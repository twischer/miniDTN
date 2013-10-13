
import logging
import errno
import os,sys

def mkdir_p(path):
	try:
		os.makedirs(path)
	except OSError as err:
		if err.errno == errno.EEXIST:
			logging.error("Directory %s already exists, aborting", path)
