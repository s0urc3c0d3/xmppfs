#!/bin/bash

export XMPPFS_PATH=`pwd`/../xmppfs
export TEST_LOG=`pwd`/tests-`date +%F-%T`.log


if ! [ -x $XMPPFS_PATH ];
then
	echo "Please build xmppfs first!"
	exit 1
fi

for i in `seq -w 0 99`; 
do
	for j in ${i}*;
	do
		if [ -x $j ]; 
		then
			echo "Starting: $j" >> $TEST_LOG
			./$j
			echo "Finished: $j" >> $TEST_LOG
			echo >> $TEST_LOG
		fi
	done
done
