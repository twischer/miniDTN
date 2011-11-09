#!/usr/bin/python2.7

import os
from optparse import OptionParser
import subprocess
#import pydot


parser = OptionParser()
parser.add_option("-g", "--graph", dest="graph",
		help="generate a graph")
parser.add_option("-c", "--clusters",
		action="store_true", dest="clusters", default=False,
		help="arrange the functions by filename")
parser.add_option("-i", "--individual",
		action="store_true", dest="individual", default=False,
		help="show individual callsites in functions")
parser.add_option("-l", "--log", dest="log",
		help="log file to open")
parser.add_option("-x", "--binary", dest="bin",
		help="binary file")
parser.add_option("-p", "--prefix", dest="prefix",
		help="prefix needed for the addr2line tool")
parser.add_option("-s", "--sort", dest="sort", default="count",
		help="sort according to criteria (from|to|count|time)")
parser.add_option("-r", "--reverse", action="store_false", dest="reverse",
		default=True, help="revert sort order")
parser.add_option("-q", "--quiet",
		action="store_false", dest="verbose", default=True,
		help="don't print status messages to stdout")

(options, args) = parser.parse_args()

func_table = {}
file_table = {}

def lookup_symbol(symbol, funcptr=False):
	# AVR stores the address in words, correct that
	if options.prefix == "avr":
		symbol = 2*symbol

	if symbol in func_table:
		return func_table[symbol]

	element = {}

	output = subprocess.check_output(["%s-addr2line"%(options.prefix), "-afe", options.bin, "0x%x"%(symbol)]).strip().split('\n')

	element['func'] = funcptr
	element['addr'] = symbol #int(output[0], 16)
	element['name'] = output[1]
	element['file'], element['line'] = output[2].split(':')

	file_el = file_table.setdefault(element['file'], [])
	if not element['name'] in file_el:
		file_el.append(element['name'])

	func_table[symbol] = element
	return element

def plural(i):
	if i == 1:
		return ""
	else:
		return "s"

def graph_function(func, callsite, label=None):
	if not label:
		label = func
	if not callsite:
#		return pydot.Node(func, label=label)
		out.write("  %s [label=\"%s\"]\n"%(func, label))
	else:
		out.write("  subgraph cluster_fn_%s {\n  label=\"%s\";shape=ellipse;\n\n"%(func, label))
		for site in func_table.values():
			if site['name'] == func:
				if site['func']:
					out.write("    mem_%x [label=\"\" style=\"invis\" height=0 width=0 fixedsize=true]\n"%(site['addr']))
				else:
					out.write("    mem_%x [label=\"line %s\"]\n"%(site['addr'], site['line']))
		out.write("  }\n")

def graph_functions(out, byfile=False, callsites=False):
	if (byfile):
		i = 0
		for file_el in file_table.keys():
			out.write("  subgraph cluster_file_%i {\n"%(i))
			out.write("    label=\"%s\";\n"%(file_el.split('/')[-1]))
			i += 1
			for func in file_table[file_el]:
				graph_function(out, func, callsites, label="%s()"%(func))

			out.write("  }\n")
	else:
		for file_el in file_table.keys():
			for func in file_table[file_el]:
				graph_function(out, func, callsites, label="%s\\n%s()"%(file_el.split('/')[-1], func))


def graph_edges():
	pass

def graph_files():
	pass

def generate_callgraph(calls, outfile):
	cumulative = {}
	allcount = 0
	alltime = 0

	if (options.individual):
		for call in calls:
			cumel = cumulative.setdefault(("mem_%x"%(call['from']['addr']), "mem_%x"%(call['to']['addr'])), {'count':0, 'time':0, 'site':0})
			cumel['from'] = call['from']
			cumel['to'] = call['to']
			cumel['count'] = call['count']
			cumel['time'] = call['time']
			cumel['site'] = 1
			allcount += call['count']
			alltime += call['time']
	else:
		for call in calls:
			cumel = cumulative.setdefault((call['from']['name'], call['to']['name']), {'count':0, 'time':0, 'site':0})
			cumel['count'] += call['count']
			cumel['time'] += call['time']
			cumel['site'] += 1
			allcount += call['count']
			alltime += call['time']

	out = open(outfile, 'w')

	out.write("digraph \"%s\" {\n"%(options.bin))
	out.write("label=\"%s callgraph\\ndotted: #calls < 0.1%% / \dashed: 0.1%% <= #calls < 1%% / solid: #calls >= 1%%\"\n"%(options.bin));
	out.write("splines=\"spline\";\nnodesep=0.4;\ncompound=true;\n");
	out.write("node [shape=ellipse fontsize=10];\n");
	out.write("edge [fontsize=9];\n");

	graph_functions(out, options.clusters, options.individual)

	avgtime = float(alltime)/allcount

	for fpair in cumulative.keys():
		attr = ""
		reltime = float(cumulative[fpair]['time'])/cumulative[fpair]['count']

		if cumulative[fpair]['count'] < 0.001* allcount:
			attr += " style=\"dotted\""
		elif cumulative[fpair]['count'] < 0.01* allcount:
			attr += " style=\"dashed\""
		else:
			attr += " style=\"bold\""

		frac = reltime/(avgtime*2)
		if frac > 1:
			frac = 1
		attr += " color=\"#%02x00%02x\""%(frac*255, (1-frac)*255)

		if options.individual:
			attr += "lhead=\"cluster_fn_%s\""% cumulative[fpair]['to']['name']

		out.write("  %s -> %s [label=\"%i site%s\\n%.3fms\\n%i call%s\" %s]\n"%(fpair[0], fpair[1],
			cumulative[fpair]['site'], plural(cumulative[fpair]['site']),
			reltime/128.0*1000, cumulative[fpair]['count'], plural(cumulative[fpair]['count']), attr))

	out.write("}\n")
	out.close()



def handle_prof(logfile):
	calls = []
	print "Profiling for %s"%(options.bin)
	for line in logfile:
		elements = line.split(':')
		call = {}
		symfrom = int(elements[0], 16)
		symto = int(elements[1], 16)
		from_el = lookup_symbol(symfrom)
		to_el = lookup_symbol(symto, funcptr=True)
		call['from'] = from_el
		call['to'] = to_el
		call['count'] = int(elements[2])
		call['time'] = int(elements[3])
		calls.append(call)

	calls  = sorted(calls, key=lambda call: call[options.sort], reverse=options.reverse)
	if options.graph:
		generate_callgraph(calls, options.graph)

	for call in calls:
		print "%s -> %s %i times, %.3fms"%(call['from']['name'], call['to']['name'], call['count'], call['time']/128.0*1000)


def handle_sprof(logfile):
	print "Error, unimplemented!"
	os.exit(1)

logfile = open(options.log)

# Read the header
header = logfile.readline()
if (header.startswith("PROF")):
	handle_prof(logfile)
elif (header.startswith("SPROF")):
	handle_sprof(logfile)
