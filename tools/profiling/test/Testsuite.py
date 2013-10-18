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
from Testcase import *
from helper import *


class Testsuite(object):
	def __init__(self, suitecfg, devcfg, testcfg,options):
		self.config = suitecfg
		self.devcfg=devcfg
		# Set PATH to contain inga_tool and profile-neat.py
		path_inga_tool_dir = os.path.join(self.config['contikibase'], "tools", "inga", "inga_tool")
		path_profile_neat_py_dir = os.path.join(self.config['contikibase'], "tools", "profiling")
		os.environ['PATH'] += ':%s:%s' % (path_inga_tool_dir, path_profile_neat_py_dir)

		self.options = options
		if 'contikiscm' in self.config:
			if self.config['contikiscm'] == 'git':
				self.contikiversion = subprocess.check_output(["git", "describe", "--tags", "--always", "--dirty", "--long"], stderr=subprocess.STDOUT, cwd=self.config['contikibase']).rstrip()
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

		# create logdir
		self.logdir = os.path.join(self.config['logbase'], logdir)
		mkdir_p(self.logdir)

		# Set up logging to file
		filehandler = logging.FileHandler(os.path.join(self.logdir, "build.log"))
		filehandler.setLevel(logging.DEBUG)
		fileformat = logging.Formatter(fmt='%(asctime)s %(name)s %(levelname)s: %(message)s',
				datefmt='%Y%m%d %H:%M:%S')
		filehandler.setFormatter(fileformat)
		logging.getLogger('').addHandler(filehandler)

		logging.info("Logs are located under %s", self.logdir)

		with open(os.path.join(self.logdir, 'contikiversion'), 'w') as verfile:
			verfile.write(self.contikiversion)
			verfile.write('\n')

		shutil.copyfile(options.node_configfile, os.path.join(self.logdir, 'node_config.yaml'))
		shutil.copyfile(options.test_configfile, os.path.join(self.logdir, 'test_config.yaml'))

		# create symlink to logdir
		try:
			self.logdir_last = os.path.join(self.config['logbase'], "lastlog")
			if os.path.islink(self.logdir_last):
				os.unlink(self.logdir_last)
			os.symlink(self.logdir, self.logdir_last)
			logging.info("Symlink to Logs under %s", self.logdir_last)
		#FIXME be more specific...
		except Exception:
			self.logdir_last = "FAILED"

		self.devices = {}
		self.build_inga_tool = False
		for devicecfg in devcfg:
			# Build inga_tool if needed
			if not self.build_inga_tool:
				if devicecfg['class'] == 'INGA':
					self.build_inga_tool = True
					self.inga_tool_dir = os.path.join(self.config['contikibase'], "tools", "inga", "inga_tool")
					logging.info("Building inga_tool %s", self.inga_tool_dir)
					if not os.path.exists(self.inga_tool_dir):
						logging.error("Could not find %s", self.inga_tool_dir)
						raise Exception
					try:
						output = subprocess.check_output(["make", "-C", self.inga_tool_dir], stderr=subprocess.STDOUT)
						logging.debug(output)
					except subprocess.CalledProcessError as err:
						logging.error(err)
						logging.error(err.output)
						raise
			deviceclass = globals()[devicecfg['class']]
			deviceinst = deviceclass(devicecfg,devcfg)
			self.devices[devicecfg['name']] = deviceinst

		self.tests = {}
		for testcfg in testcfg:
			testcfg['logbase'] = self.logdir
			testcfg['contikibase'] = self.config['contikibase']
			#if testcfg['name'] in self.config['testcases']:
			testcase = Testcase(testcfg, self.devices, testcfg['devices'],devcfg,options)
			self.tests[testcfg['name']] = testcase


		# Info
		logging.info("Profiling suite - initialized")
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
		if not 'testcases' in self.config:
		  self.config['testcases'] = [x.name for x in self.tests.values()]
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

		if self.options.xmlreport:
			with open(os.path.join(self.options.xmlreport, 'build.xml'), 'w') as xmlfile:
				xmlfile.write('<testsuite errors="0" failures="%d" name="" tests="%d" time="0">\n'
						%(len(failure), len(failure)+len(success)))
				for test in success:
					xmlfile.write('<testcase classname="contiki" name="%s" time="0"/>\n'
							%(test.name))
				for test in failure:
					xmlfile.write('<testcase classname="contiki" name="%s" time="0">\n'
							%(test.name))
					xmlfile.write('<failure type="testfailure">%s</failure>\n'
							%(test.failure))
					xmlfile.write('</testcase>\n')

				xmlfile.write('<system-out><![CDATA[')
				for test in success:
					for item in test.result:
						xmlfile.write("<measurement><name>%s %s-%s (%s)</name><value>%f</value></measurement>\n"
								%(test.name, item['name'], item['desc'], item['unit'], float(item['data'])/item['scale']))
				xmlfile.write(']]></system-out>\n')

				xmlfile.write('</testsuite>\n')

		# make clean inga_tool_dir
		if self.build_inga_tool:
			logging.info("Cleaning inga_tool sourcedir %s", self.inga_tool_dir)
			try:
				output = subprocess.check_output(["make", "-C", self.inga_tool_dir, "clean"], stderr=subprocess.STDOUT)
				logging.debug(output)
			except subprocess.CalledProcessError as err:
				logging.error(err)
				logging.error(err.output)
				raise

		return len(failure)
