xmppfs
======

simple FUSE xmpp client

Depedencies: libstrophe, fuse-utils libssl libexpat1 libz libresolve libcrypto libfuse-dev


Author

xmppfs is oryginaly created and still maintained by Grzegorz Dwornicki
visit http://c0d3.pl for more info on xmppfs and my other projects. Report a problem with this software soo I can fix it.


License
This software is released under GLPv2 (http://www.gnu.org/licenses/old-licenses/gpl-2.0.html). A copy of GLPv2 is located in LICENSE file


How to build

First install depedencies. Next go to main directory and type:
make

if all goes well you should have working binary in main directory now. If you wish to install this binary you may copy it to other directory. Makefile provides install target soo typing:
make install

will copy xmppfs to /usr/local/bin

