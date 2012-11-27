#!/bin/sh

# example wrapper-skript for using test.py with INGA

# first argument passed has to be the path to a correct config.yaml, will be passed with --config-Option to test.py
CONF="${1}"

# arguments 2-n are passed to test.py, see test.py -h
shift

# get contiki base-path from config.yaml
CONTIKI="$(sed -n 's/^.*contikibase: //p' ${CONF})"

# build inga-tool
make -C "${CONTIKI}"/tools/inga/inga_tool

# set path to contain inga-tool and profile-neat.py , use config.yaml , pass on arguments to test.py
PATH="${PATH}":"${CONTIKI}"/tools/inga/inga_tool:"${CONTIKI}"/tools/profiling "${CONTIKI}"/tools/profiling/test/test.py --config "${CONF}" "$@"

# cleanup inga-tool
make -C "${CONTIKI}"/tools/inga/inga_tool clean
