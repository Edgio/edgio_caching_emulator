#!/bin/bash
# Copyright 2021 Edgecast Inc
# Licensed under the terms of the Apache 2.0 open source license
# See LICENSE file for terms.


function run_test()
{
    local executable=$1
    BINNAME=$(basename ${executable})
    local LOGDIR=$2
    
    # Get the list of timestamps from the log file names in log directory 'LOGDIR'
    LIST_OF_TS=`ls $LOGDIR/*.gz | awk -F "/" '{print $NF}' | awk -F'_' '{print $2}' | awk -F'.' '{print $1}' | sort -n | uniq`
    
    echo "Adding run of $executable to $RUNDIR/$BINNAME.dat"
    
    for i in $LIST_OF_TS; do
        gunzip -c $LOGDIR/*$i*.gz | sort -n
    done | $executable > $RUNDIR/$BINNAME.dat
    
    echo "Finished $executable"
}



if [[ $# -lt 2 ]] ; then
    echo -e 'Usage:\n\trun_sim.sh <log dir> <exp_bin...>'
    echo -e "\nTo use the provided sample logs and default Least Recently Used, Second Hit Caching simulator, \n use 'input_request_sequence' as the log_dir and '$(dirname $0)/bin/lru_2hc' as the experiment binary."
    exit 0
fi

if [ ! -f "$(dirname $0)/bin/lru_2hc" ]; then
    echo -e 'ERROR: lru_2hc not found.\nPlease run "make" to build the binary before running this script.'
    exit 1
fi

DATE=`date +%d_%b_%y-%H_%M`
OUTDIR="out"
LOGDIR=$1
RUNDIR=$OUTDIR/$DATE
echo "Output will be saved in: $RUNDIR"
mkdir -p out;
mkdir -p $RUNDIR;

shift

for executable in "$@"
do
    run_test $executable $LOGDIR
done

echo "Output was saved in: $RUNDIR"

# Convenience, set a symlink so I can easily find these results
rm $OUTDIR/latest
ln -s $DATE $OUTDIR/latest

