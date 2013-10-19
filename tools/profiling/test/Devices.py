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


class Device(object):
	"""Represents the actual device (Sky, INGA, ...) partaking in the test"""
	startpattern = "*******Booting Contiki"
	profilepattern = ["PROF", "SPROF"]
	def __init__(self, config,devcfg):
		self.name = config['name']
		self.id = config['id']
		self.path = config['path']
		self.devcfg=devcfg
		
	def configure(self, config, testname):
		self.logger = logging.getLogger("test.%s.%s"%(testname, self.name))
		self.logdir = config['logbase']
		self.contikibase = config['contikibase']
		self.programdir = os.path.join(self.contikibase, config['programdir'])
		self.program = config['program']
		self.binary = os.path.join(self.logdir, "%s-%s"%(self.name, self.program))
		self.instrument = config['instrument']
		self.debug = config['debug']
		self.cflags = config.setdefault('cflags', "")
		self.graph_options = config.setdefault('graph_options', "")

	def build(self,options):
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

			self.logger.info("Generating file lists")
			just_instrument= list(set(self.instrument) - set(self.debug))
			just_debug= list(set(self.debug) - set(self.instrument))
			instrument_and_debug= list(set(self.debug) & set(self.instrument))
			#self.logger.info("inst:\n %s \n debug:\n %s \n both: %s",str(just_instrument), str(just_debug), str(instrument_and_debug))
			self.logger.info("Building %s", os.path.join(self.programdir, self.program))
			myenv = os.environ.copy()
			myenv['CFLAGS'] = self.cflags
			for dev in self.devcfg:
				myenv['CFLAGS']=myenv['CFLAGS'].replace(str("-DCONF_DEST_NODE=$"+dev['name'].upper()),str("-DCONF_DEST_NODE="+str(dev['id'])))
				myenv['CFLAGS']=myenv['CFLAGS'].replace(str("-DCONF_SEND_TO_NODE=$"+dev['name'].upper()),str("-DCONF_SEND_TO_NODE="+str(dev['id'])))
			# always add the nodeid of this node as define
			myenv['CFLAGS']+=str(" -DNODEID="+str(self.id))
			self.myenv = myenv
			output = subprocess.check_output(["make", "TARGET=%s"%(self.platform), self.program], stderr=subprocess.STDOUT, env=myenv)
			self.logger.debug(output)
			time.sleep(2)
			if len(just_instrument) > 0: #build file with just instrumentation again
				touchcall = ["touch"]
				touchcall.extend([os.path.join(self.contikibase, instrpat) for instrpat in just_instrument])
				self.logger.debug(' '.join(touchcall))
				## XXX:Danger, Will Robinson! Do not use shell with user input
				output = subprocess.check_output(' '.join(touchcall), stderr=subprocess.STDOUT, shell=True)
				self.logger.debug(output)
				self.logger.info("Building instrumentation for %s", os.path.join(self.programdir, self.program))
				myenv = os.environ.copy()
				myenv['CFLAGS'] = '-finstrument-functions %s'%(self.cflags)
				for dev in self.devcfg:
					myenv['CFLAGS']=myenv['CFLAGS'].replace(str("-DCONF_DEST_NODE=$"+dev['name'].upper()),str("-DCONF_DEST_NODE="+str(dev['id'])))
					myenv['CFLAGS']=myenv['CFLAGS'].replace(str("-DCONF_SEND_TO_NODE=$"+dev['name'].upper()),str("-DCONF_SEND_TO_NODE="+str(dev['id'])))
				output = subprocess.check_output(["make", "TARGET=%s"%(self.platform), self.program], stderr=subprocess.STDOUT, env=myenv)
				self.logger.debug(output)


			if len(just_debug) > 0: #build file witch just debug again
				touchcall = ["touch"]
				touchcall.extend([os.path.join(self.contikibase, instrpat) for instrpat in just_debug])
				self.logger.debug(' '.join(touchcall))
				## XXX:Danger, Will Robinson! Do not use shell with user input
				output = subprocess.check_output(' '.join(touchcall), stderr=subprocess.STDOUT, shell=True)
				self.logger.debug(output)
				self.logger.info("Building logging for %s", os.path.join(self.programdir, self.program))
				myenv = os.environ.copy()
				myenv['CFLAGS'] = '-DENABLE_LOGGING %s'%(self.cflags)
				for dev in self.devcfg:
					myenv['CFLAGS']=myenv['CFLAGS'].replace(str("-DCONF_DEST_NODE=$"+dev['name'].upper()),str("-DCONF_DEST_NODE="+str(dev['id'])))
					myenv['CFLAGS']=myenv['CFLAGS'].replace(str("-DCONF_SEND_TO_NODE=$"+dev['name'].upper()),str("-DCONF_SEND_TO_NODE="+str(dev['id'])))
				output = subprocess.check_output(["make", "TARGET=%s"%(self.platform), self.program], stderr=subprocess.STDOUT, env=myenv)
				self.logger.debug(output)


			if len(instrument_and_debug) > 0: #build file with instrumentation and debug
				touchcall = ["touch"]
				touchcall.extend([os.path.join(self.contikibase, instrpat) for instrpat in instrument_and_debug])
				self.logger.debug(' '.join(touchcall))
				## XXX:Danger, Will Robinson! Do not use shell with user input
				output = subprocess.check_output(' '.join(touchcall), stderr=subprocess.STDOUT, shell=True)
				self.logger.debug(output)
				self.logger.info("Building instrumentation and logging for %s", os.path.join(self.programdir, self.program))
				myenv = os.environ.copy()
				myenv['CFLAGS'] = '-finstrument-functions -DENABLE_LOGGING %s'%(self.cflags)
				for dev in self.devcfg:
					myenv['CFLAGS']=myenv['CFLAGS'].replace(str("-DCONF_DEST_NODE=$"+dev['name'].upper()),str("-DCONF_DEST_NODE="+str(dev['id'])))
					myenv['CFLAGS']=myenv['CFLAGS'].replace(str("-DCONF_SEND_TO_NODE=$"+dev['name'].upper()),str("-DCONF_SEND_TO_NODE="+str(dev['id'])))
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
	def dummy(self,testname):
		raise Exception('Unimplemented')
	def reset_occurred(self):
		self.abort_by_reset = True
	def recordlog(self, callgraphqueue, queue, controlqueue):
		logfile = os.path.join(self.logdir, "%s.log"%(self.name))

		self.logger.info("Recording device log to %s", logfile)
		handler = logging.FileHandler(logfile)
		formatter = logging.Formatter(fmt='%(asctime)s: %(message)s',
				datefmt='%Y%m%d %H%M%S')
		handler.setFormatter(formatter)
		handler.setLevel(logging.DEBUG)
		self.logger.addHandler(handler)

		resetseen = 0
		resettimer = threading.Timer(5, self.reset_occurred)
		resettimer.daemon = True
		self.abort_by_reset = False

		# Some bug somewhere in the system prevents the baud rate to be set correctly after a USB reset
		# Setting the baud rate twice works around that
		ser = serial.Serial(port=self.path, baudrate=1200, timeout=0.5)
		ser.baudrate = self.baudrate

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
				self.logger.debug("%s %s"%(self.name, line))

				if line.startswith(self.startpattern):
					resetseen += 1
					if resetseen == 2:
						# Delay aborting the test to record valuable log data
						resettimer.start()

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
					if not callgraphqueue is None:
						workitem = (profilename, profiledata, self.logdir, self.prefix, self.binary)
						callgraphqueue.put(workitem)
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

			if self.abort_by_reset:
				self.logger.error("Device resetted unexpectedly, aborting test")
				queue.put({'status': 'Aborted', 'reason': 'Node restart detected', 'name': self.name})
				break
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

		resettimer.cancel()
		ser.close()
		self.logger.removeHandler(handler)
		self.logger.info("Finished logging")

class SKY(Device):
	"""TMote Sky"""
	prefix="msp430"
	platform="sky"
	baudrate="115200"

	def upload(self):
		try:
			os.chdir(self.programdir)
		except OSError as err:
			self.logger.error("Could not change to %s", self.programdir)
			raise

		try:
			self.logger.info("Uploading %s", os.path.join(self.programdir, self.program))
			output = subprocess.check_output(["make", "TARGET=sky", "MOTES=%s"%(self.path), "%s.upload"%(self.program)], stderr=subprocess.STDOUT,env=self.myenv)
			self.binary = os.path.join(self.logdir, "%s-%s"%(self.name, self.program))
			shutil.copyfile("%s.sky"%(self.program), self.binary)
			self.logger.debug(output)
		except subprocess.CalledProcessError as err:
			self.logger.error(err)
			self.logger.error(err.output)
			raise
	def reset(self):
		try:
			self.logger.info( "Resetting")
			output = subprocess.check_output(["make", "TARGET=sky", "MOTES=%s"%(self.path), "sky-reset-sequence"], stderr=subprocess.STDOUT)
		except subprocess.CalledProcessError as err:
			self.logger.error(err)
			self.logger.error(err.output)
			raise
	def dummy(self,testname):
		self.logger = logging.getLogger("test.%s.%s"%(testname, self.name))
		output = subprocess.check_output(["msp430-bsl-linux", "--telosb","-c", self.path, "-r"], stderr=subprocess.STDOUT)
		try:
			self.logger.info( "Uploading DUMMY to %s"%(self.name))
			output = subprocess.check_output(["msp430-bsl-linux", "--telosb","-c", self.path, "-e"], stderr=subprocess.STDOUT)
		except subprocess.CalledProcessError as err:
			self.logger.error(err)
			self.logger.error(err.output)
			raise

class SkyMonitor(SKY):
	pass

class INGA(Device):
	"""INGA node"""
	prefix="avr"
	platform="inga"
	baudrate=38400

	def upload(self):
		try:
			os.chdir(self.programdir)
		except OSError as err:
			self.logger.error("Could not change to %s", self.programdir)
			raise

		try:
			self.logger.info("Uploading %s", os.path.join(self.programdir, self.program))
			output = subprocess.check_output(["make", "TARGET=inga", "MOTES=%s"%(self.path), "%s.upload"%(self.program)], stderr=subprocess.STDOUT, env=self.myenv)
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
	def dummy(self,testname):
		self.logger = logging.getLogger("test.%s.%s"%(testname, self.name))
		try:
			self.reset()
		except:
			self.logger.warning("Did not find dummy device. Ignoring it.")
			return
		try:
			self.logger.info( "Uploading DUMMY to %s"%(self.name))
			output = subprocess.check_output(["avrdude", "-b","230400", "-P", self.path, "-c", "avr109" ,"-p","atmega1284p","-e" ], stderr=subprocess.STDOUT)
		except subprocess.CalledProcessError as err:
			self.logger.error(err)
			self.logger.error(err.output)
			raise
