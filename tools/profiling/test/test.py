#!/usr/bin/python

import yaml
import errno
import serial
import logging
import threading
import subprocess
import shutil
import time
import copy
import os,sys
import traceback

def mkdir_p(path):
	try:
		os.makedirs(path)
	except OSError as err:
		if err.errno == errno.EEXIST:
			logging.error("Directory %s already exists, aborting", path)
		raise

class Device(object):
	"""Represents the actual device (Sky, INGA, ...) partaking in the test"""
	startpattern = "*******Booting Contiki"
	endpattern = "ContikiEND"
	profilepattern = ["PROF", "SPROF"]
	def __init__(self, config):
		self.name = config['name']
		self.id = config['id']
		self.path = config['path']
		self.logger = logging.getLogger('dev.%s'%(self.name))
	def configure(self, config):
		self.logdir = os.path.join(config['logbase'], self.name)
		mkdir_p(self.logdir)
		self.contikibase = config['contikibase']
		self.programdir = os.path.join(self.contikibase, config['programdir'])
		self.program = config['program']
		self.instrument = config['instrument']
		self.cflags = config.setdefault('cflags', "")
		self.graph_options = config.setdefault('graph_options', "")

	def build(self):
		try:
			os.chdir(self.programdir)
		except OSError as err:
			self.logger.error("Could not change to %s", self.programdir)
			raise

		try:
			self.logger.info("Cleaning %s", self.programdir)
			output = subprocess.check_output(["make", "TARGET=%s"%(self.platform), "clean"], stderr=subprocess.STDOUT)
			self.logger.debug(output)

			self.logger.info("Building %s", os.path.join(self.programdir, self.program))
			output = subprocess.check_output(["make", "TARGET=%s"%(self.platform), self.program], stderr=subprocess.STDOUT, env={'CFLAGS': self.cflags})
			self.logger.debug(output)
			time.sleep(2)
			touchcall = ["touch"]
			touchcall.extend([os.path.join(self.contikibase, instrpat) for instrpat in self.instrument])
			self.logger.debug(' '.join(touchcall))
			## XXX:Danger, Will Robinson! Do not use shell with user input
			output = subprocess.check_output(' '.join(touchcall), stderr=subprocess.STDOUT, shell=True)
			self.logger.debug(output)
			self.logger.info("Building instrumentation for %s", os.path.join(self.programdir, self.program))
			output = subprocess.check_output(["make", "TARGET=%s"%(self.platform), self.program], stderr=subprocess.STDOUT, env={'CFLAGS': '-finstrument-functions %s'%(self.cflags)})
			self.logger.debug(output)
		except subprocess.CalledProcessError as err:
			self.logger.error(err)
			self.logger.error(err.output)
			raise
	def upload(self):
		raise Exception('Unimplemented')
	def reset(self):
		raise Exception('Unimplemented')
	def create_graph(self, log):
		basename = os.path.join(self.logdir, "profile")
		svgname = "%s.svg"%(basename)
		pdfname = "%s.pdf"%(basename)

		profile = subprocess.Popen(["profile-neat.py", "-p", self.prefix, "-g", svgname, self.binary,  "-"], stdin=subprocess.PIPE, stderr=subprocess.STDOUT, stdout=subprocess.PIPE)
		(output, tmp) = profile.communicate(log)
		logging.info(output)
		output = subprocess.check_output(["inkscape", "-A", pdfname, svgname])
	def recordlog(self):
		logfile = os.path.join(self.logdir, self.name)
		self.reset()
		# Make sure the device nodes are there again
		time.sleep(1)

		self.logger.info("Recording device log to %s", logfile)
		handler = logging.FileHandler(logfile)
		formatter = logging.Formatter(fmt='%(asctime)s: %(message)s',
				datefmt='%Y%m%d %H%M%S')
		handler.setFormatter(formatter)
		handler.setLevel(logging.DEBUG)
		self.logger.addHandler(handler)

		ser = serial.Serial(port=self.path, baudrate=self.baudrate)

		resetseen = 0
		profilelines = -1
		for line in ser:
			line = line.strip()
			self.logger.debug(line)

			if resetseen > 1:
				self.logger.error("Device resetted unexpectedly, aborting test")
				break

			if line.startswith(self.startpattern):
				resetseen += 1
			for profpat in self.profilepattern:
				if line.startswith(profpat):
					profilelines = int(line.split(':')[1]) + 1
					profiledata = ""

			if profilelines > 0:
				profiledata += line
				profiledata += "\n"
				profilelines -= 1
			if profilelines == 0:
				self.create_graph(profiledata)
				profilelines = -1


		ser.close()
		self.logger.removeHandler(handler)
		self.logger.info("Finished logging")


class Sky(Device):
	pass

class SkyMonitor(Sky):
	pass

class INGA(Device):
	"""INGA node"""
	prefix="avr"
	platform="inga"
	baudrate=19200

	def upload(self):
		try:
			os.chdir(self.programdir)
		except OSError as err:
			self.logger.error("Could not change to %s", self.programdir)
			raise

		try:
			self.logger.info("Uploading %s", os.path.join(self.programdir, self.program))
			output = subprocess.check_output(["make", "TARGET=inga", "MOTES=%s"%(self.path), "%s.upload"%(self.program)], stderr=subprocess.STDOUT)
			self.binary = os.path.join(self.logdir, self.program)
			shutil.copyfile("%s.inga"%(self.program), self.binary)
			self.logger.debug(output)
		except subprocess.CalledProcessError as err:
			self.logger.error(err)
			self.logger.error(err.output)
			raise
	def reset(self):
		try:
			self.logger.info( "Resetting")
			output = subprocess.check_output(["inga_tool", "-r", "-d", self.path], stderr=subprocess.STDOUT)
		except subprocess.CalledProcessError as err:
			self.logger.error(err)
			self.logger.error(err.output)
			raise


class Testcase(object):
	"""Contiki testcase"""
	def __init__(self, config, devicelist, devicecfg):
		self.name = config['name']
		self.logger = logging.getLogger('test.%s'%(self.name))
		self.logbase = os.path.join(config['logbase'], self.name)
		self.contikibase = config['contikibase']
		self.devices = []

		mkdir_p(self.logbase)
		for cfgdevice in devicecfg:
			try:
				device = devicelist[cfgdevice['name']]
			except KeyError as err:
				self.logger.error("Could not find device %s, unable to complete test", cfgdevice['name'])
				raise

			device = copy.copy(device)
			cfgdevice['logbase'] = self.logbase
			cfgdevice['contikibase'] = self.contikibase
			device.configure(cfgdevice)
			self.devices.append(device)


	def run(self):
		for device in self.devices:
			try:
				self.logger.info("Setting up %s for device %s", device.programdir, device.name)
				device.build()
				device.upload()
			except Exception as err:
				self.logger.error("Test could not complete")
				self.logger.error(err)
				raise

		self.logger.info("All devices configured, resetting and starting test")

		threads = []
		for device in self.devices:
			thread = threading.Thread(target=device.recordlog)
			threads.append(thread)
			thread.start()

		# Wait until all threads have finished
		timeout = True
		while timeout:
			timeout = False
			for thread in threads:
				thread.join(1)
				if thread.isAlive():
					timeout = True

class Testsuite(object):
	def __init__(self, suitecfg, devcfg, testcfg):
		self.config = suitecfg

		if (self.config['logpattern'] == 'date'):
			logdir = time.strftime('%Y%m%d%H%M%S')
		elif (self.config['logpattern'] == 'tag'):
			pass
		self.logdir = os.path.join(self.config['logbase'], logdir)

		mkdir_p(self.logdir)

		self.devices = {}
		for devicecfg in devcfg:
			deviceclass = globals()[devicecfg['class']]
			deviceinst = deviceclass(devicecfg)
			self.devices[devicecfg['name']] = deviceinst

		self.tests = {}
		for testcfg in testcfg:
			testcfg['logbase'] = self.logdir
			testcfg['contikibase'] = self.config['contikibase']
			testcase = Testcase(testcfg, self.devices, testcfg['devices'])
			self.tests[testcfg['name']] = testcase

		# Set up logging to file
		filehandler = logging.FileHandler(os.path.join(self.logdir, "build.log"))
		filehandler.setLevel(logging.DEBUG)
		fileformat = logging.Formatter(fmt='%(asctime)s %(name)s %(levelname)s: %(message)s',
				datefmt='%Y%m%d %H:%M:%S')
		filehandler.setFormatter(fileformat)
		logging.getLogger('').addHandler(filehandler)

		# Info
		logging.info("Profiling suite - initialized")
		logging.info("Logs are located under %s", self.logdir)
		logging.info("Contiki base path is %s", self.config['contikibase'])
		logging.info("The following devices are defined:")
		for device in self.devices.values():
			logging.info("* %s (%s platform) identified through %s", device.name,
					device.platform, device.path)
		logging.info("The following tests are defined:")
		for test in self.tests.values():
			logging.info("* %s with devices:", test.name)
			for device in test.devices:
				logging.info("  * %s configured with program %s", device.name,
						os.path.join(device.programdir, device.program))


	def run(self):
		failure = []
		success = []
		for testname in self.config['testcases']:
			logging.info("Running test %s", testname)
			try:
				test = self.tests[testname]
			except KeyError as err:
				logging.error("Unable to find configured test %s, skipping", testname)
				continue

			try:
				test.run()
			except Exception as err:
				logging.error("Test %s failed, traceback follows", testname)
				logging.error(traceback.format_exc())
				failure.append(test)
				continue

			success.append(test)

		logging.info("Test summary:")
		for test in success:
			logging.info("OK  - %s", test.name)
		for test in failure:
			logging.info("ERR - %s", test.name)


# Create a logger for the console
logging.basicConfig(level=logging.DEBUG,
		format='%(name)s %(levelname)s: %(message)s',
		datefmt='%y%m%d %H:%M:%S')

logging.getLogger().handlers[0].setLevel(logging.INFO)

configfile = file('config.yaml', 'r')
config = yaml.load(configfile)

suite = Testsuite(config['suite'], config['devices'], config['tests'])

suite.run()
