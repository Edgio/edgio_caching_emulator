#!/bin/bash

# A convenience script to build and run
# the simulator in one step.

# Use nproc on Linux, else use a Mac OS X sysctl
NCPUS=$(uname | grep -q Linux && nproc ||  sysctl -n hw.ncpu)

CPP=g++ make -j ${NCPUS} && ./run_em.sh input_request_sequence ./bin/lru_2hc
