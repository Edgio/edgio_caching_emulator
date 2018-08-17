#!/bin/bash

# A convenience script to build and run
# the simulator in one step.

CPP=g++ make -j $(nproc) && ./run_em.sh input_request_sequence ./bin/lru_2hc