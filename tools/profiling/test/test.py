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
import argparse

class MakeList(argparse.Action):
	def __call__(self, parser, namespace, values, option_string=None):
		setattr(namespace, self.dest, values.split(','))

parser = argparse.ArgumentParser(description="Test tool for Contiki")

parser.add_argument("-l", "--list", dest="list_tests", action="store_true", default=False,
		help="only list what tests/devices are defined")
parser.add_argument("-d", "--dirty", dest="dirty", action="store_true", default=False,
		help="don't clean the projects")
parser.add_argument("-c", "--config", dest="configfile", default="config.yaml",
		help="where to read the config from")
parser.add_argument("--only-tests",
		dest="only_tests", default=[],
                action=MakeList,
		help="only run the tests defined on the commandline")

options = parser.parse_args()

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
	profilepattern = ["PROF", "SPROF"]
	def __init__(self, config):
		self.name = config['name']
		self.id = config['id']
		self.path = config['path']
	def configure(self, config, testname):
		self.logger = logging.getLogger("test.%s.%s"%(testname, self.name))
		self.logdir = config['logbase']
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
			if not options.dirty:
				self.logger.info("Cleaning %s", self.programdir)
				output = subprocess.check_output(["make", "TARGET=%s"%(self.platform), "clean"], stderr=subprocess.STDOUT)
				self.logger.debug(output)

			self.logger.info("Building %s", os.path.join(self.programdir, self.program))
			myenv = os.environ.copy()
			myenv['CFLAGS'] = self.cflags
			output = subprocess.check_output(["make", "TARGET=%s"%(self.platform), self.program], stderr=subprocess.STDOUT, env=myenv)
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
				myenv = os.environ.copy()
				myenv['CFLAGS'] = '-finstrument-functions %s'%(self.cflags)
				output = subprocess.check_output(["make", "TARGET=%s"%(self.platform), self.program], stderr=subprocess.STDOUT, env=myenv)
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
		basename = os.path.join(self.logdir, self.name)
		svgname = "%s-%s.svg"%(basename, name)
		pdfname = "%s-%s.pdf"%(basename, name)

		profile = subprocess.Popen(["profile-neat.py", "-p", self.prefix, "-g", svgname, self.binary,  "-"], stdin=subprocess.PIPE, stderr=subprocess.STDOUT, stdout=subprocess.PIPE)
		(output, tmp) = profile.communicate(log)
		logging.info(output)
		output = subprocess.check_output(["inkscape", "-A", pdfname, svgname])
	def recordlog(self, queue, controlqueue):
		logfile = os.path.join(self.logdir, "%s.log"%(self.name))

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
					# Handle the following outputs
					# TEST:FAIL:reason
					# TEST:PASS
					# TEST:REPORT:desc:value:scale:unit
					if testdata[1] == "REPORT":
						queue.put({'status': 'Report', 'desc': testdata[2], 'name': self.name, 'data': int(testdata[3]), 'scale': int(testdata[4]), 'unit': testdata[5]})
					elif testdata[1] == "PASS":
						queue.put({'status': 'Completed', 'name': self.name})
						break
					elif testdata[1] == "FAIL":
						queue.put({'status': 'Failed', 'name': self.name, 'reason': testdata[2]})
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
			self.binary = os.path.join(self.logdir, "%s-%s"%(self.name, self.program))
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
		self.result = []

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
			device.configure(cfgdevice, self.name)
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
		self.result = []

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
					device.reset()
				# Make sure the device nodes are there again
				time.sleep(1)

				for device in self.devices:
					control = Queue.Queue()
					thread = threading.Thread(target=device.recordlog, args=(queue,control))
					threads[device.name] = (thread, control)
					thread.start()

				self.logger.info("Starting timeout (%is)", self.timeout)
				timeouttimer.start()

				alldead = False
				while not self.timedout and not alldead:
					# Exit if all threads have finished, but handle queue items first
					alldead = True
					for thread in threads.values():
						if thread[0].isAlive():
							alldead = False
							break
					if alldead:
						self.logger.info("All devices completed")
						break

					try:
						while True:
							item = queue.get(True, 2)
							self.logger.debug("Got item %s"%(item))
							queue.task_done()
							if item['status'] == "Aborted" or item['status'] == "Failed":
								err = Exception("Test failed: Device %s aborted (%s)"%(item['name'], item['reason']))
								for thread in threads.values():
									thread[1].put("Exit")
								time.sleep(2)
								raise err
							elif item['status'] == "Completed":
								self.logger.info("Device %s completed test successfully", item['name'])
							elif item['status'] == "Report":
								self.logger.info("Device %s reported metric: %s is %f %s", item['name'], item['desc'], float(item['data'])/item['scale'], item['unit'])
								self.result.append(item)
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
				err = Exception("Aborted by user")
				raise err

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
		self.logger.info("Test %s report:", self.name)
		for item in self.result:
			self.logger.info("%s:%s:%f:%s", item['name'], item['desc'], float(item['data'])/item['scale'], item['unit'])

		self.logger.removeHandler(resulthandler)
		self.logger.removeHandler(handler)

class Testsuite(object):
	def __init__(self, suitecfg, devcfg, testcfg):
		self.config = suitecfg

		if 'contikiscm' in self.config:
			if self.config['contikiscm'] == 'git':
				self.contikiversion = subprocess.check_output(["git", "describe", "--tags", "--always", "--dirty"], stderr=subprocess.STDOUT, cwd=self.config['contikibase']).rstrip()
			elif self.config['contikiscm'] == 'none':
				self.contikiversion = "NA"
			else:
				self.contikiversion = "unknown"

		if self.config['logpattern'] == 'tag':
			logdir = self.contikiversion
		elif self.config['logpattern'] == 'date-tag':
			logdir = "%s-%s"%(time.strftime('%Y%m%d%H%M%S'), self.contikiversion)
		else: # Date based is the default
			logdir = time.strftime('%Y%m%d%H%M%S')

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
			if testcfg['name'] in self.config['testcases']:
				testcase = Testcase(testcfg, self.devices, testcfg['devices'])
				self.tests[testcfg['name']] = testcase

		# Set up logging to file
		filehandler = logging.FileHandler(os.path.join(self.logdir, "build.log"))
		filehandler.setLevel(logging.DEBUG)
		fileformat = logging.Formatter(fmt='%(asctime)s %(name)s %(levelname)s: %(message)s',
				datefmt='%Y%m%d %H:%M:%S')
		filehandler.setFormatter(fileformat)
		logging.getLogger('').addHandler(filehandler)

		with open(os.path.join(self.logdir, 'contikiversion'), 'w') as verfile:
			verfile.write(self.contikiversion)
			verfile.write('\n')

		shutil.copyfile(options.configfile, os.path.join(self.logdir, 'config.yaml'))

		# Info
		logging.info("Profiling suite - initialized")
		logging.info("Logs are located under %s", self.logdir)
		logging.info("Contiki base path is %s", self.config['contikibase'])
		logging.info("Contiki version is %s", self.contikiversion)
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
				test.failure = err
				failure.append(test)
				continue

			success.append(test)

		logging.info("Test summary (%i/%i succeeded):", len(success), len(success)+len(failure))
		for test in success:
			logging.info("%s [OK]", test.name)
			for item in test.result:
				logging.info("* %s:%s:%f:%s", item['name'], item['desc'], float(item['data'])/item['scale'], item['unit'])
		for test in failure:
			logging.info("%s [ERR]", test.name)
			logging.info(" -> %s", test.failure)

		return len(failure)


# Create a logger for the console
logging.basicConfig(level=logging.DEBUG,
		format='%(name)s %(levelname)s: %(message)s',
		datefmt='%y%m%d %H:%M:%S')

logging.getLogger().handlers[0].setLevel(logging.INFO)

configfile = file(options.configfile, 'r')
config = yaml.load(configfile)

# Override with commandline tests if specified
if len(options.only_tests) > 0:
	config['suite']['testcases'] = options.only_tests

suite = Testsuite(config['suite'], config['devices'], config['tests'])

if (options.list_tests):
	sys.exit(0)

ret = suite.run()

sys.exit(ret)
