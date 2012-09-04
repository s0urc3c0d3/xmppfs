#!/bin/sh

JID1=tester@example.jabber.com/psi
PASS1=tester
HOST1=example.jabber.com
MNT_POINT2=/mnt/xmppfs1

JID2=tester3@example.jabber.com/psi2
PASS2=tester3
HOST2=example.jabber.com
MNT_POINT2=/mnt/xmppfs2

if ! [ -d $MNT_POINT1 ]; then mkdir $MNT_POINT1; fi
if ! [ -d $MNT_POINT2 ]; then mkdir $MNT_POINT2; fi

$XMPPFS_PATH -j $JID1 -p $PASS1 -o $HOST1 -m $MNT_POINT1
$XMPPFS_PATH -j $JID2 -p $PASS2 -o $HOST2 -m $MNT_POINT2


#poczatek testu
