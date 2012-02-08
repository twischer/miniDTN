#!/usr/bin/python2.7

import os
import argparse
import subprocess
import pydot

class MakeList(argparse.Action):
	def __call__(self, parser, namespace, values, option_string=None):
		setattr(namespace, self.dest, values.split(','))

def split_list(option, opt, value, parser):
	setattr(parser.values, option.dest, value.split(','))

parser = argparse.ArgumentParser(description="Process profiling data")

display = parser.add_argument_group('display')
# Cluster functions together
display.add_argument("-c", "--clusters",
		action="store_true", dest="clusters", default=False,
		help="arrange the functions by filename")
display.add_argument("--cluster-files",
		dest="cluster_files", default=[],
                action=MakeList,
		help="only cluster functions in the following filenames")

# Show individual callsites
display.add_argument("-i", "--individual",
		action="store_true", dest="individual", default=False,
		help="show individual callsites in functions")
##TODO: Implement
##display.add_argument("--individual-source",
##		action="store_true", dest="individual_source", default=False,
##		help="show source line of the callsites")
##display.add_argument("--individual-functions",
##		dest="individual_functions", default=[],
##                action=MakeList,
##		help="show individual callsites in the specified functions")
display.add_argument("--cumulative",
		action="store_true", dest="cumulative", default=False,
		help="accumulate time")


filtering = parser.add_argument_group('filtering')
#Filtering
filtering.add_argument("--ignore-functions",
		dest="ignore_functions", default=[],
                action=MakeList,
		help="ignore the following functions")
filtering.add_argument("--only-functions",
		dest="only_functions", default=[],
                action=MakeList,
		help="only consider the following functions")
filtering.add_argument("--ignore-files",
		dest="ignore_files", default=[],
                action=MakeList,
		help="ignore functions in the following files")
filtering.add_argument("--only-files",
		dest="only_files", default=[],
                action=MakeList,
		help="only consider functions in the following files")

graph = parser.add_argument_group('graph options')
graph.add_argument("-g", "--graph", dest="graph",
		help="generate a graph. The pattern %%n will be replaced by the name of the profiling result")
graph.add_argument("--highlight-functions",
		dest="highlight_functions", default=[],
                action=MakeList,
		help="highlight the following functions")
graph.add_argument("--highlight-color", dest="highlight_color", default="#00FF00",
		help="Highlight color")

parser.add_argument("-s", "--sort", dest="sort", default="count",
		help="sort according to criteria (from|to|count|time)")
parser.add_argument("-r", "--reverse", action="store_false", dest="reverse",
		default=True, help="revert sort order")
parser.add_argument("-q", "--quiet",
		action="store_false", dest="verbose", default=True,
		help="don't print status messages to stdout")
parser.add_argument("-p", "--prefix", dest="prefix",
		help="prefix needed for the addr2line tool")
parser.add_argument("bin",
		help="binary file")
parser.add_argument("log", type=argparse.FileType('r'),
		help="log file with the profiling data")

options = parser.parse_args()


opts = {}
func_table = {}
file_table = {}

def ignore_site(site, onlyfile=False, onlyname=False):
	name = None
	filename = None
	if onlyname:
		name = site
	elif onlyfile:
		filename = site
	else:
		name = site['name']
		filename = site['file']

	if name and name in options.ignore_functions:
		return True
	if filename and filename.split('/')[-1] in options.ignore_files:
		return True
	if name and len(options.only_functions) and not (name in options.only_functions):
		return True
	if filename and len(options.only_files) and not filename.split('/')[-1] in options.only_files:
		return True
	return False


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
	if ignore_site(func, onlyname=True):
		return

	if not label:
		label = func

	color="#dddddd00"

	if func in options.highlight_functions:
		color = options.highlight_color

	time_spent = 0
	invocations = 0
	time_max = 0
	time_min = 0xffff
	for site in func_table.values():
		if site['name'] == func:
			time_spent += site['time_spent']
			invocations += site['invocations']
			if 'time_max' in site:
				time_max = max(site['time_max'], time_max)
			if 'time_min' in site:
				time_min = min(site['time_min'], time_min)

	if invocations > 0:
		fnlabel = "%s\nmin: %.3fms max: %.3fms\n%.3fms\n%i calls"%(label, float(time_min)/opts['ticks_per_sec']*1000,
				float(time_max)/opts['ticks_per_sec']*1000, float(time_spent)/opts['ticks_per_sec']*1000, invocations)
	else:
		fnlabel = "%s\n(unprofiled)"%(label)
	if not callsite:
		graph.add_node(pydot.Node(func, label=fnlabel, fillcolor=color, style="filled"))
	else:
		subgr = pydot.Subgraph("cluster_fn_%s"%func, label=fnlabel, style="filled", fillcolor=color)
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
		if ignore_site(file_el, onlyfile=True):
			continue
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

	for site in func_table.values():
		if site['invocations'] > 0:
			alltime += site['time_spent']
	opts['time_fns'] = alltime

	alltime = 0

	if (options.individual):
		for call in calls:
			if ignore_site(call['to']) or ignore_site(call['from']):
				continue
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
			if ignore_site(call['to']) or ignore_site(call['from']):
				continue
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
		if len(cumulative) == 0:
			avgtime = 0
		else:
			avgtime = float(alltime)/len(cumulative)
	else:
		if allcount == 0:
			avgtime = 0
		else:
			avgtime = float(alltime)/allcount

	for fpair in cumulative.keys():
		if options.cumulative:
			reltime = float(cumulative[fpair]['time'])/opts['time_run']
			duration = "%.3f%%"%(reltime*100)
		else:
			try:
				reltime = float(cumulative[fpair]['time'])/cumulative[fpair]['count']
			except ZeroDivisionError:
				reltime = 0
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
	opts = dict(zip(('num_sites', 'max_sites', 'time_run', 'ticks_per_sec'), [int(i) for i in tempopts[1:]]))
	opts['name'] = tempopts[0]

	i = 0

	for line in logfile:
		if i == opts['num_sites']:
			break


		i += 1
		elements = line.split(':')
		call = {}
		symfrom = int(elements[0], 16)
		symto = int(elements[1], 16)
		from_el = lookup_symbol(symfrom)
		to_el = lookup_symbol(symto, funcptr=True)

		from_el.setdefault('time_spent', 0)
		from_el.setdefault('invocations', 0)
		to_el.setdefault('time_spent', 0)
		to_el.setdefault('invocations', 0)
		if ignore_site(from_el['name'], onlyname=True) or ignore_site(to_el['name'], onlyname=True):
			continue

		call['from'] = from_el
		call['to'] = to_el
		call['count'] = int(elements[2])
		call['time'] = int(elements[3])

		call['time_min'] = int(elements[4])
		call['time_max'] = int(elements[5])

		if 'time_min' in to_el:
			to_el['time_min'] = min(to_el['time_min'], int(elements[4]))
		else:
			to_el['time_min'] = int(elements[4])

		if 'time_max' in to_el:
			to_el['time_max'] = max(to_el['time_max'], int(elements[5]))
		else:
			to_el['time_max'] = int(elements[5])

		calls.append(call)

		# Keep track of how much time we're actually spending in here
		from_el['time_spent'] -= call['time']
		to_el['time_spent'] += call['time']
		to_el['invocations'] += call['count']


	if options.graph:
		graphfile = options.graph.replace("%n", opts['name'])
		generate_callgraph(calls, graphfile)

	calls  = sorted(calls, key=lambda call: call[options.sort], reverse=options.reverse)
	for call in calls:
		print "%s -> %s %i times, %.3fms"%(call['from']['name'], call['to']['name'], call['count'], float(call['time'])/opts['ticks_per_sec']*1000)

	funcs = {}
	for func in func_table.values():
		fn = funcs.setdefault(func['name'], {'invocations': 0, 'time': 0})
		fn['invocations'] += func['invocations']
		fn['time'] += func['time_spent']

	combtime = 0
	for fn in funcs.keys():
		if funcs[fn]['invocations'] > 0:
			combtime += funcs[fn]['time']
			print "%s %i times %.3fs"%(fn, funcs[fn]['invocations'], float(funcs[fn]['time'])/opts['ticks_per_sec'])

	print "Function time combined: %.3fs, Profiling time: %.3fs"%(float(combtime)/opts['ticks_per_sec'], float(opts['time_run'])/opts['ticks_per_sec'])


def handle_sprof(logfile, header):
	sites = []
	print "Statistical profiling for %s"%(options.bin)
	tempopts = header.split(':')[1:]
	global opts
	opts = dict(zip(('num_sites', 'max_sites', 'num_samples') , [int(i) for i in tempopts[1:]]))
	opts['name'] = tempopts[0]

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

	if options.individual:
		sites = sorted(sites, key=lambda site: site['count'], reverse=options.reverse)
		for site in sites:
			if ignore_site(site['addr']):
				continue
			print "(%s) %s:%s (0x%x) %i times"%(site['addr']['file'], site['addr']['name'], site['addr']['line'], site['addr']['addr'], site['count'])
	else:
		filtered_sites = {}
		for site in sites:
			if ignore_site(site['addr']):
				continue
			if options.cumulative:
				fsite = filtered_sites.setdefault((site['addr']['file'], site['addr']['name']), {'name': site['addr']['name'], 'file': site['addr']['file'], 'line': '()', 'count': 0})
				fsite['count'] += site['count']
			else:
				fsite = filtered_sites.setdefault((site['addr']['file'], site['addr']['name'], site['addr']['line']), {'name': site['addr']['name'], 'file': site['addr']['file'], 'line': site['addr']['line'], 'count': 0})
				fsite['count'] += site['count']

		filtered_sites = sorted(filtered_sites.values(), key=lambda site: site['count'], reverse=options.reverse)
		for site in filtered_sites:
			print "(%s) %s:%s %i times"%(site['file'], site['name'], site['line'], site['count'])


# Read the header
header = options.log.readline()
while (header):
	if (header.startswith("PROF")):
		handle_prof(options.log, header)
		break
	elif (header.startswith("SPROF")):
		handle_sprof(options.log, header)
		break
	header = options.log.readline()
