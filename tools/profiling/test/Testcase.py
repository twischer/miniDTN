
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

from Devices import *
from helper import *

class Testcase(object):
	"""Contiki testcase"""
	def __init__(self, config, devicelist, devicecfg, devcfg,options):
		self.name = config['name']
		self.logger = logging.getLogger('test.%s'%(self.name))
		self.logbase = os.path.join(config['logbase'], self.name)
		self.contikibase = config['contikibase']
		self.timeout = int(config.setdefault('timeout', "300"))
		self.devices = []
		self.unused_devices = []
		self.timedout = False
		self.result = []
		self.options= options
		for unused in devcfg:
			found=0
			for dev in devicecfg:
				if unused['name'] == dev['name']:
					found=1
			if found==0 :
				device = devicelist[unused['name']]
				device = copy.copy(device)
				self.unused_devices.append(device)
				device.dummy(self.name)
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

	def create_graph(self, name, log, logdir, prefix, binary):
		basename = os.path.join(logdir, self.name)
		svgname = "%s-%s.svg"%(basename, name)
		pdfname = "%s-%s.pdf"%(basename, name)

		self.logger.info("Writing Call Graph to %s and %s", svgname, pdfname)
		profile = subprocess.Popen(["profile-neat.py", "-p", prefix, "-g", svgname, binary,  "-"], stdin=subprocess.PIPE, stderr=subprocess.STDOUT, stdout=subprocess.PIPE)
		(output, tmp) = profile.communicate(log)
		logging.info(output)
		output = subprocess.check_output(["inkscape", "-A", pdfname, svgname])

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

		# Initialize call graph worker queue
		self.callgraphqueue = Queue.Queue()

		try:
			self.logger.info("Starting test %s", self.name)
			for device in self.devices:
				try:
					self.logger.info("Setting up %s for device %s", device.programdir, device.name)
					device.build(self.options)
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
					thread = threading.Thread(target=device.recordlog, args=(self.callgraphqueue,queue,control))
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

		# Create all call graphs
		self.logger.info("Creating call graphs...")
		while self.callgraphqueue.qsize() > 0:
			workitem = self.callgraphqueue.get()
			self.create_graph(workitem[0], workitem[1], workitem[2], workitem[3], workitem[4])
			self.callgraphqueue.task_done()
