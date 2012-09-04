#!/bin/bash

export XMPPFS_PATH=`pwd`/../xmppfs
export TEST_LOG=`pwd`/tests-`data +%F-%T`.log


if ! [ -x $XMPPFS_PATH ];
then
	echo "Please build xmppfs first!"
	exit 1
fi

for i in `seq -w 1 99`; 
do
	for j in ${i}*;
	do
		echo "Starting: $j" >> $TEST_LOG
		./$j
		echo "Finished: $j" >> $TEST_LOG
		echo >> $TEST_LOG
	done
done
