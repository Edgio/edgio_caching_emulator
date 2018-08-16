#!/bin/bash
# Copyright 2018, Oath Inc.
# Licensed under the terms of the Apache 2.0 open source license
# See LICENSE file for terms.

if [[ $# -eq 0 ]] ; then
    echo -e 'Usage:\n\trun_sim.sh <log dir> <exp_bin...>'
    exit 0
fi

DATE=`date +%d_%b_%y-%H_%M`
OUTDIR="out"
LOGDIR=$1
RUNDIR=$OUTDIR/$DATE
BINNAME=${2##*/}

echo "Output will be saved in: $RUNDIR"

# Make the outdir
if [ ! -d out ]; then
  mkdir out;
fi

# Make the rundir
if [ ! -d $RUNDIR ]; then
  mkdir $RUNDIR;
fi

# Get the list of timestamps from the log file names in log directory 'LOGDIR'
LIST_OF_TS=`ls $LOGDIR/*.gz | awk -F "/" '{print $NF}' | awk -F'_' '{print $2}' | awk -F'.' '{print $1}' | sort -n | uniq`

# loop over the time stamps from the filenames, and
# cat all files from that timestamp into tee (to pass to multiple instance of caching emulator binaries)
COMMAND="for i in $LIST_OF_TS; do zcat $LOGDIR/*\$i*.gz | sort -n; done | tee "

echo "Adding $2"

NEW_CMD=">(/usr/bin/time --quiet -f "%e" $2 > $RUNDIR/$BINNAME.dat) "

# final command
COMMAND=$COMMAND$NEW_CMD

COMMAND="$COMMAND > /dev/null"
eval $COMMAND

echo "Output was saved in: $RUNDIR"

# Convenience, set a symlink so I can easily find these results
rm $OUTDIR/latest
ln -s $DATE $OUTDIR/latest
