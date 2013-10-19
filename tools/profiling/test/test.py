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

node_configfile = file(options.node_configfile, 'r')
node_config = yaml.load(node_configfile)

test_configfile = file(options.test_configfile, 'r')
test_config = yaml.load(test_configfile)


# Override with commandline tests if specified
if len(options.only_tests) > 0:
	node_config['suite']['testcases'] = options.only_tests

suite = Testsuite(node_config['suite'], node_config['devices'], test_config['tests'],options)

if (options.list_tests):
	sys.exit(0)

ret = suite.run()

sys.exit(ret)
