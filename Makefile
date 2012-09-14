all:
	gcc -o xmppfs xmppfs.c -D_FILE_OFFSET_BITS=64 -L/usr/local/lib/ -lstrophe -lexpat -lssl  -lcrypto -lz  -lresolv -Wall `pkg-config fuse --cflags --libs` -ggdb
clean:
	rm xmppfs
install:
	cp xmppfs /usr/local/bin
