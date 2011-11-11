#!/usr/bin/python2.7

import os
from optparse import OptionParser
import subprocess
import pydot

def split_list(option, opt, value, parser):
	setattr(parser.values, option.dest, value.split(','))

parser = OptionParser()
parser.add_option("-g", "--graph", dest="graph",
		help="generate a graph")


# Cluster functions together
parser.add_option("-c", "--clusters",
		action="store_true", dest="clusters", default=False,
		help="arrange the functions by filename")
parser.add_option("--cluster-files",
		dest="cluster_files", default=[],
                type='string', action='callback', callback=split_list,
		help="cluster functions in the following filenames")

# Show individual callsites
parser.add_option("-i", "--individual",
		action="store_true", dest="individual", default=False,
		help="show individual callsites in functions")
parser.add_option("--cumulative",
		action="store_true", dest="cumulative", default=False,
		help="accumulate time")
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

opts = {}
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

	# Does the address point to the start of a function or not?
	element['func'] = funcptr
	# Address of the call site
	element['addr'] = symbol #int(output[0], 16)
	# Name of the function
	element['name'] = output[1]
	# Filename and line number of the call site
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

def graph_function(graph, func, callsite, label=None):
	if not label:
		label = func

	time_spent = 0
	invocations = 0
	for site in func_table.values():
		if site['name'] == func:
			time_spent += site['time_spent']
			invocations += site['invocations']

	if invocations > 0:
		fnlabel = "%s\n%.3fms\n%i calls"%(label, float(time_spent)/opts['ticks_per_sec']*1000, invocations)
	else:
		fnlabel = "%s\n(unprofiled)"%(label)
	if not callsite:
		graph.add_node(pydot.Node(func, label=fnlabel))
	else:
		subgr = pydot.Subgraph("cluster_fn_%s"%func, label=fnlabel, style="filled")
		for site in func_table.values():
			if site['name'] == func:
				if site['func']:
					subgr.add_node(pydot.Node("mem_%x"%(site['addr']), label="", shape="plaintext", style="invis"))
				else:
					subgr.add_node(pydot.Node("mem_%x"%(site['addr']), label="line %s"%(site['line']), shape="plaintext"))
		graph.add_subgraph(subgr)

def graph_functions(graph, byfile=False, callsites=False):
	i = 0
	for file_el in file_table.keys():
		if byfile or (file_el.split('/')[-1] in options.cluster_files):
			subgr = pydot.Subgraph("cluster_file_%i"%(i), label=file_el.split('/')[-1])
			i += 1
			for func in file_table[file_el]:
				graph_function(subgr, func, callsites, label="%s()"%(func))
			graph.add_subgraph(subgr)
		else:
			for func in file_table[file_el]:
				graph_function(graph, func, callsites, label="%s\\n%s()"%(file_el.split('/')[-1], func))


def graph_edges():
	pass

def generate_callgraph(calls, outfile):
	cumulative = {}
	allcount = 0
	alltime = 0

	for i in func_table.values():
		if i['time_spent'] > 0:
			alltime += i['time_spent']
	opts['time_fns'] = alltime

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


	opts['time_all'] = alltime
	opts['count_all'] = allcount
	graph = pydot.Dot(graph_name='G', graph_type='digraph', label="%s callgraph\\ndotted: #calls < 0.1%% / \dashed: 0.1%% <= #calls < 1%% / solid: #calls >= 1%%"%(options.bin),
			splines="spline", nodesep=0.4, compound=True);

	graph.set_node_defaults(shape="ellipse", fontsize=10);
	graph.set_edge_defaults(fontsize=9);

	graph_functions(graph, options.clusters, options.individual)

	if options.cumulative:
		avgtime = float(alltime)/len(cumulative)
	else:
		avgtime = float(alltime)/allcount

	for fpair in cumulative.keys():
		if options.cumulative:
			reltime = float(cumulative[fpair]['time'])/opts['time_run']
			duration = "%.3f%%"%(reltime*100)
		else:
			reltime = float(cumulative[fpair]['time'])/cumulative[fpair]['count']
			duration = "%.3fms/call"%(reltime/opts['ticks_per_sec']*1000)

		edge = pydot.Edge(fpair[0], fpair[1], label="%i site%s\n%s\n%i call%s"%(cumulative[fpair]['site'], plural(cumulative[fpair]['site']),
							 duration, cumulative[fpair]['count'], plural(cumulative[fpair]['count'])))

		if cumulative[fpair]['count'] < 0.001* allcount:
			style = "dotted"
		elif cumulative[fpair]['count'] < 0.01* allcount:
			style = "dashed"
		else:
			style = "bold"

		edge.set_style(style)


		if options.cumulative:
			frac = reltime*10
		else:
			frac = reltime/(avgtime*2)

		if frac > 1:
			frac = 1


		edge.set_color("#%02x00%02x"%(frac*255, (1-frac)*255))

		if options.individual:
			edge.set_lhead("cluster_fn_%s"%cumulative[fpair]['to']['name'])

		graph.add_edge(edge);

	graph.write_svg(outfile)



def handle_prof(logfile, header):
	calls = []
	print "Profiling for %s"%(options.bin)
	tempopts = header.split(':')[1:]
	global opts
	opts = dict(zip(('num_sites', 'max_sites', 'time_run', 'ticks_per_sec'), [int(i) for i in tempopts]))

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

		# Keep track of how much time we're actually spending in here
		from_el.setdefault('time_spent', 0)
		from_el['time_spent'] -= call['time']
		from_el.setdefault('invocations', 0)
		to_el.setdefault('time_spent', 0)
		to_el['time_spent'] += call['time']
		to_el.setdefault('invocations', 0)
		to_el['invocations'] += call['count']

		if len(calls) == opts['num_sites']:
			break



	calls  = sorted(calls, key=lambda call: call[options.sort], reverse=options.reverse)
	if options.graph:
		generate_callgraph(calls, options.graph)

	for call in calls:
		print "%s -> %s %i times, %.3fms"%(call['from']['name'], call['to']['name'], call['count'], float(call['time'])/opts['ticks_per_sec']*1000)

	print "Function time combined: %.3fs, Profiling time: %.3fs"%(float(opts['time_fns'])/opts['ticks_per_sec'], float(opts['time_run'])/opts['ticks_per_sec'])


def handle_sprof(logfile, header):
	sites = []
	print "Statistical profiling for %s"%(options.bin)
	tempopts = header.split(':')[1:]
	global opts
	opts = dict(zip(('num_sites', 'max_sites', 'num_samples') , [int(i) for i in tempopts]))

	for line in logfile:
		elements = line.split(':')
		site = {}
		symaddr = int(elements[0], 16)
		symbol = lookup_symbol(symaddr)
		site['addr'] = symbol
		site['count'] = int(elements[1])
		sites.append(site)

		if len(sites) == opts['num_sites']:
			break

	sites = sorted(sites, key=lambda site: site['count'], reverse=options.reverse)

	for site in sites:
		print "%s:%s %i times"%(site['addr']['name'], site['addr']['line'], site['count'])


logfile = open(options.log)

# Read the header
header = logfile.readline()
if (header.startswith("PROF")):
	handle_prof(logfile, header)
elif (header.startswith("SPROF")):
	handle_sprof(logfile, header)
