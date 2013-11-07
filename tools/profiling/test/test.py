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

from Devices import *
from Testcase import *
from Testsuite import *

class MakeList(argparse.Action):
	def __call__(self, parser, namespace, values, option_string=None):
		setattr(namespace, self.dest, values.split(','))

parser = argparse.ArgumentParser(description="Test tool for Contiki")

parser.add_argument("-l", "--list", dest="list_tests", action="store_true", default=False,
		help="only list what tests/devices are defined")
parser.add_argument("-d", "--dirty", dest="dirty", action="store_true", default=False,
		help="don't clean the projects")
parser.add_argument("-r", "--reset-only", dest="resetonly", action="store_true", default=False,
		help="don't build and upload")
parser.add_argument("-s", "--suite-config", dest="suite_configfile", default="suite_config.yaml",
		help="where to read the test suite config from")
parser.add_argument("-t", "--test-config", dest="test_configfile", default="test_config.yaml",
		help="where to read the test config from")
parser.add_argument("-n", "--node-config", dest="node_configfile", default="node_config.yaml",
		help="where to read the executing nodes config from")
parser.add_argument("-x", "--xml", dest="xmlreport", default=False,
		help="output the test reports in XML (for easy parsing with jenkins), takes path to XML-report-dir as argument")
parser.add_argument("--only-tests",
		dest="only_tests", default=[],
                action=MakeList,
		help="only run the tests defined on the commandline")

options = parser.parse_args()




# Create a logger for the console
logging.basicConfig(level=logging.DEBUG,
		format='%(name)s %(levelname)s: %(message)s',
		datefmt='%y%m%d %H:%M:%S')

logging.getLogger().handlers[0].setLevel(logging.INFO)

# Load test suite if available
try:
        suite_configfile = open(options.suite_configfile, 'r')
        suite_config = yaml.load(suite_configfile)
except IOError:
        logging.error("Failed to load suite configuration '%s'.", options.suite_configfile)
        sys.exit(1)

# Load nodes
try:
        node_configfile = open(options.node_configfile, 'r')
        node_config = yaml.load(node_configfile)
except IOError:
        logging.error("Failed to load nodes configuration '%s'.", options.node_configfile)
        sys.exit(1)

# Directory of suite_config.yaml is base directory unless 'workdir' property is set
if 'workdir' in suite_config['suite']:
	workdir = suite_config['suite']['workdir']
else:
	workdir = os.path.dirname(os.path.abspath(options.suite_configfile))
logging.info("Working directory is '%s'", workdir)
suite_config['suite']['contikibase'] = os.path.join(workdir, suite_config['suite']['contikibase'])
suite_config['suite']['logbase'] = os.path.join(workdir, suite_config['suite']['logbase'])

# Load test from dirs selected with suite option 'testdirs'
# If this option is not set try to load single test config file
test_config = {'tests' : []}
if 'testdirs' in suite_config['suite']:
	for testdir in suite_config['suite']['testdirs']:
		loadfile = os.path.join(workdir, testdir, "test_config.yaml")
		try:
			test_configfile = open(loadfile, 'r')
			new_tests = yaml.load(test_configfile)['tests']
			# add directory information for each test (currently not used)
			for test in new_tests:
				test['testdir'] = testdir
			test_config['tests'] += new_tests #yaml.load(test_configfile)['tests']
		except IOError:
			logging.warning("Test configuration file '%s' was not found. Ignoring...", loadfile)
else:
	try:
		test_configfile = open(options.test_configfile, 'r')
		test_config = yaml.load(test_configfile)
	except IOError:
		logging.error("Test configuration file '%s' was not found. Aborting...", options.test_configfile)
		sys.exit(1)

# Override with commandline tests if specified
if len(options.only_tests) > 0:
	suite_config['suite']['testcases'] = options.only_tests

suite = Testsuite(suite_config['suite'], node_config['devices'], test_config['tests'], options)

if (options.list_tests):
	sys.exit(0)

ret = suite.run()

sys.exit(ret)
