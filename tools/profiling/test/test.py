#!/usr/bin/python

import yaml
import errno
import serial
import logging
import threading
import Queue
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
		ser = serial.Serial(port=self.path, baudrate=1200, timeout=0.5)
		ser.close()
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
			if len(self.instrument) > 0:
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
	def create_graph(self, name, log):
		basename = os.path.join(self.logdir, "profile")
		svgname = "%s-%s.svg"%(basename, name)
		pdfname = "%s-%s.pdf"%(basename, name)

		profile = subprocess.Popen(["profile-neat.py", "-p", self.prefix, "-g", svgname, self.binary,  "-"], stdin=subprocess.PIPE, stderr=subprocess.STDOUT, stdout=subprocess.PIPE)
		(output, tmp) = profile.communicate(log)
		logging.info(output)
		output = subprocess.check_output(["inkscape", "-A", pdfname, svgname])
	def recordlog(self, queue, controlqueue):
		logfile = os.path.join(self.logdir, "%s.log"%(self.name))
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

		# Some bug somewhere in the system prevents the baud rate to be set correctly after a USB reset
		# Setting the baud rate twice works around that
		ser = serial.Serial(port=self.path, baudrate=1200, timeout=0.5)
		ser.baudrate = self.baudrate

		resetseen = 0
		profilelines = -1
		line = ""
		while True:
			try:
				line += ser.readline()
			except serial.SerialException as err:
				queue.put({'status': 'Aborted', 'reason': err, 'name': self.name})
				break
			if (line.endswith('\n') or line.endswith('\r')):
				line = line.strip()
				self.logger.debug(line)

				if line.startswith(self.startpattern):
					resetseen += 1
				if resetseen > 1:
					self.logger.error("Device resetted unexpectedly, aborting test")
					queue.put({'status': 'Aborted', 'reason': 'Node restart detected', 'name': self.name})
					break

				for profpat in self.profilepattern:
					if line.startswith(profpat):
						profilelines = int(line.split(':')[2]) + 1
						profilename = line.split(':')[1]
						profiledata = ""

				if profilelines > 0:
					profiledata += line
					profiledata += "\n"
					profilelines -= 1
				if profilelines == 0:
					self.create_graph(profilename, profiledata)
					profilelines = -1

				if line.startswith("TEST"):
					testdata = line.split(':')
					# TEST:FAIL/PASS:<value>:<unit>
					queue.put({'status': 'Completed', 'reason': testdata[1], 'name': self.name, 'data': int(testdata[2]), 'scale': int(testdata[3]), 'unit': testdata[4]})
					break
				line = ""

			try:
				item = controlqueue.get_nowait()
				controlqueue.task_done()
				if item == "Exit":
					self.logger.info("Aborted by test")
					break
				else:
					self.logger.debug("Received unknown control item: %s", item)
			except Queue.Empty:
				pass

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
		self.timeout = int(config.setdefault('timeout', "300"))
		self.devices = []
		self.timedout = False

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

	def timeout_occured(self):
		self.timedout = True

	def run(self):
		logfile = os.path.join(self.logbase, "test.log")
		handler = logging.FileHandler(logfile)
		formatter = logging.Formatter(fmt='%(asctime)s: %(message)s',
				datefmt='%Y%m%d %H%M%S')
		handler.setFormatter(formatter)
		handler.setLevel(logging.DEBUG)
		self.logger.addHandler(handler)

		timeouttimer = threading.Timer(self.timeout+30, self.timeout_occured)
		timeouttimer.daemon = True
		self.timedout = False
		result = []

		try:
			self.logger.info("Starting test %s", self.name)
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

			try:
				threads = {}
				queue = Queue.Queue()
				for device in self.devices:
					control = Queue.Queue()
					thread = threading.Thread(target=device.recordlog, args=(queue,control))
					threads[device.name] = (thread, control)
					thread.start()

				self.logger.info("Starting timeout (%is)", self.timeout)
				timeouttimer.start()

				while not self.timedout:
					# Exit if all threads have finished
					alldead = True
					for thread in threads.values():
						if thread[0].isAlive():
							alldead = False
							break
					if alldead:
						self.logger.info("All devices completed")
						break

					try:
						item = queue.get(True, 2)
						self.logger.debug("Got item %s"%(item))
						queue.task_done()
						if item['status'] == "Aborted":
							err = Exception("Test failed: Device %s aborted (%s)"%(item['name'], item['reason']))
							for thread in threads.values():
								thread[1].put("Exit")
							time.sleep(2)
							raise err
						elif item['status'] == "Completed":
							self.logger.info("Device %s completed test (%s) - %f %s", item['name'], item['reason'], float(item['data'])/item['scale'], item['unit'])
							result.append(item)
					except Queue.Empty:
						pass

				if self.timedout:
					self.logger.error("Timeout while running test %s", self.name)
					for thread in threads.values():
						thread[1].put("Exit")
					time.sleep(2)

					err = Exception("Timeout")
					raise err
			except KeyboardInterrupt:
				self.logger.error("Test aborted by user")
				for thread in threads.values():
					thread[1].put("Exit")
				time.sleep(2)
				raise

		except Exception:
			# Catch all exceptions and remove logger before passing the exception on
			self.logger.removeHandler(handler)
			timeouttimer.cancel()
			raise

		timeouttimer.cancel()
		self.logger.info("Test %s completed", self.name)
		logfile = os.path.join(self.logbase, "result.log")
		resulthandler = logging.FileHandler(logfile)
		formatter = logging.Formatter(fmt='%(message)s')
		resulthandler.setFormatter(formatter)
		resulthandler.setLevel(logging.DEBUG)
		self.logger.addHandler(resulthandler)
		for item in result:

			self.logger.info("%s:%s:%f:%s", item['name'], item['reason'], float(item['data'])/item['scale'], item['unit'])
			if item['reason'] != "PASS":
				err = Exception("Device %s reported failure"%(item['name'])
				self.logger.removeHandler(resulthandler)
				self.logger.removeHandler(handler)
				raise err

		self.logger.removeHandler(resulthandler)
		self.logger.removeHandler(handler)

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
