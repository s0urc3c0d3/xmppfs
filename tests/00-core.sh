#!/bin/bash 

JID1=tester@example.jabber.com
RES1=/psi
PASS1=tester
HOST1=example.jabber.com
MNT_POINT1=/mnt/xmppfs1/

JID2=tester2@example.jabber.com
RES2=/psi2
PASS2=tester2
HOST2=example.jabber.com
MNT_POINT2=/mnt/xmppfs2/

if ! [ -d $MNT_POINT1 ]; then mkdir $MNT_POINT1; fi
if ! [ -d $MNT_POINT2 ]; then mkdir $MNT_POINT2; fi

#$XMPPFS_PATH -j ${JID1}${RES1} -p $PASS1 -o $HOST1 -m $MNT_POINT1
#$XMPPFS_PATH -j ${JID2}${RES2} -p $PASS2 -o $HOST2 -m $MNT_POINT2

for i in `seq 1 100`; do
	echo "Send. and recv. message nr $i..."
	send=$i
	echo $send >> $MNT_POINT1/$JID2
	sleep 1
	recv=$(cat $MNT_POINT2/$JID1)
	if ! [ $send == $recv ];then
		echo "Test failed on $i message"
		umount $MNT_POINT1
		umount $MNT_POINT2
		exit 1
	fi
	sleep 1
done
umount $MNT_POINT1
umount $MNT_POINT2
