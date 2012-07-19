//gcc -o xmppfs xmppfs.c -D_FILE_OFFSET_BITS=64 -L/usr/local/lib/ -lstrophe -lexpat -lssl  -lcrypto -lz  -lresolv -Wall `pkg-config fuse --cflags --libs`

#include <stdio.h>
#include <stdlib.h>
#include <fuse/fuse.h>
#include <fuse/fuse_opt.h>
#include <strophe.h>
#include <pthread.h>


static int xmppfs_getattr(const char *filename, struct stat *fstat)
{

	return 0;

}

//static int xmppfs_readdir(const char *dirname, void *buf, fuse_fill_dir_t filler, off_t 		offset, struct fuse_file_info *finfo);

static struct fuse_operations xmppfs = {
	.getattr = xmppfs_getattr,
	//.readdir = xmppfs_readdir
};

//  XMPP_part

int xmpp_connection_handle_reply(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza, void * const userdata)
{
	xmpp_stanza_t *query, *item;
	char *type, *name;

	type = xmpp_stanza_get_type(stanza);
	if (strcmp(type,"error") == 0)
		fprintf(stderr, "ERROR: query failed\n");
	else {
		query = xmpp_stanza_get_child_by_name(stanza, "query");
		//zaczynamy pobieranie rostera
		for (item = xmpp_stanza_get_children(query); item;
			item = xmpp_stanza_get_next(item))
				if ((name = xmpp_stanza_get_attribute(item, "name")))
					//xmpp_stanza_get_attribute(item, "jid")
					//name - zawiera nazwe wyswietana
				else
					//xmpp_stanza_get_attribute(item, "jid")
	}
	//wskakujemy do watku

	xmpp_disconnect(conn);

	return 0;
}
					

void xmpp_connection_handler(xmpp_conn_t * const conn, const xmpp_conn_event_t status, const int error, xmpp_stream_error_t * const stream_error, void * const userdata)
{
	xmpp_ctx_t *ctx = (xmpp_ctx_t *)userdata;
	xmpp_stanza_t *iq, *query;

	if (status == XMPP_CONN_CONNECT) {
		fprintf(stderr, "DEBUG: connected\n");

		xmpp_stanza_set_name(iq,"iq");
		xmpp_stanza_set_type(iq, "get");
		xmpp_stanza_set_id(iq, "roster1");

		query = xmpp_stanza_new(ctx);
		xmpp_stanza_set_name(query, "query");
		xmpp_stanza_set_ns(query, XMPP_NS_ROSTER);

		xmpp_stanza_add_child(iq, query);

		xmpp_stanza_release(query)l

		xmpp_id_handler_add(conn, xmpp_connection_handle_reply, "roster1", ctx);

		xmpp_send(conn, iq);

		xmpp_stanza_release(iq);
	} else {
		fprint(stderr, "DEBUG: disconnected\n");
		xmpp_stop(ctx);
	}
}

int main(int argc, char *argv[])
{
	char *user_jid="tester@example.jabber.com/debian";
	char *user_pass="tester";
	char *host="example.jabber.com";
	int port = 5222;

	xmpp_initialize();

	xmpp_log_t *log;
	log = xmpp_get_default_logger(XMPP_LEVEL_DEBUG);

	xmpp_ctx_t *ctx;
	ctx = xmpp_ctx_new(NULL, log);

	xmpp_conn_t *conn;
	conn = xmpp_conn_new(ctx);

	xmpp_conn_set_jid(conn,user_jid);
	xmpp_conn_set_pass(conn,user_pass);

	xmpp_connect_client(conn, host, port, xmpp_connection_handler, ctx);

	xmpp_run(ctx);

	xmpp_conn_release(conn);
	xmpp_ctx_release(ctx);

	xmpp_shutdown();

	return 0;
}
