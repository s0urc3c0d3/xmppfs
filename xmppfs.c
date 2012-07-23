//gcc -o xmppfs xmppfs.c -D_FILE_OFFSET_BITS=64 -L/usr/local/lib/ -lstrophe -lexpat -lssl  -lcrypto -lz  -lresolv -Wall `pkg-config fuse --cflags --libs`

#include <stdio.h>
#include <stdlib.h>
#include <fuse.h>
#include <strophe.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

struct fuse_args_xmpp {
	int argc;
	char *argv[];
};

static int xmppfs_getattr(const char *filename, struct stat *fstat)
{

	return 0;

}

//static int xmppfs_readdir(const char *dirname, void *buf, fuse_fill_dir_t filler, off_t 		offset, struct fuse_file_info *finfo);

static struct fuse_operations xmppfs = {
	.getattr = xmppfs_getattr,
	//.readdir = xmppfs_readdir
};


static void fuse_thread(void *args)
{
	struct fuse_args_xmpp *m = (struct fuse_args_xmpp *)args;
	int t;
	t = fuse_main(m->argc, m->argv, &xmppfs, NULL);
//return fuse_main(argc, argv, &hello_oper, NULL)
}

//  XMPP_part

pthread_t thread1;

struct xmpp_thread_arg  {
	struct xmpp_conn_t *conn;
	struct xmpp_ctx_t *ctx;
};

struct _xmpp_contact_list {
	char *jid;
	char *name;
	struct xmpp_contact_list *next;
};

struct _xmpp_contact_list xmpp_contact_list = {
	.jid=NULL,
	.name=NULL,
	.next=NULL
};

static void *xmpp_communication(void *args)
{
	struct xmpp_thread_arg *arg = (struct xmpp_thread_arg *)args;
	struct xmpp_ctx_t *ctx = arg->ctx;
	struct xmpp_conn_t *conn = arg->conn;

	return ;
}

int xmpp_connection_handle_reply(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza, void * const userdata)
{
	struct xmpp_stanza_t *query, *item;
	char *type, *name,*jid;

	struct _xmpp_contact_list *tmp;
	tmp=&xmpp_contact_list;
	memcpy(tmp,"\0",sizeof(tmp));
	
	type = xmpp_stanza_get_type(stanza);
	if (strcmp(type,"error") == 0)
		fprintf(stderr, "ERROR: query failed\n");
	else {
		query = xmpp_stanza_get_child_by_name(stanza, "query");
//		zaczynamy pobieranie rostera
		for (item = xmpp_stanza_get_children(query); item;
			item = xmpp_stanza_get_next(item)) {
				tmp=tmp->next;
				tmp = (struct _xmpp_contact_list *)malloc(sizeof(struct _xmpp_contact_list));
				jid = xmpp_stanza_get_attribute(item, "jid");
				tmp->jid = (struct _xmpp_contact_list *)malloc(sizeof(jid));
				memcpy(tmp->jid,jid,sizeof(jid));
				if ((name = xmpp_stanza_get_attribute(item, "name")))
				{	
					tmp->name = (struct _xmpp_contact_list *)malloc(sizeof(name));
					strncpy(tmp->name,name,sizeof(name));
					
				}
		}
	}

	struct xmpp_thread_arg *args = (struct xmpp_thread_args *) malloc(sizeof(struct xmpp_thread_arg));
	args->ctx=(struct xmpp_ctx_t *)userdata;
	args->conn=conn;

	pthread_create( &thread1, NULL, xmpp_communication, args);
	pthread_join( thread1, NULL);

	xmpp_disconnect(conn);

	return 0;
}
					

void xmpp_connection_handler(xmpp_conn_t * const conn, const xmpp_conn_event_t status, const int error, xmpp_stream_error_t * const stream_error, void * const userdata)
{
	xmpp_ctx_t *ctx = (xmpp_ctx_t *)userdata;
	xmpp_stanza_t *iq, *query;

	if (status == XMPP_CONN_CONNECT) {
		fprintf(stderr, "DEBUG: connected\n");

		iq = xmpp_stanza_new(ctx);

		xmpp_stanza_set_name(iq, "iq");
		xmpp_stanza_set_type(iq, "get");
		xmpp_stanza_set_id(iq, "roster1");

		query = xmpp_stanza_new(ctx);
		xmpp_stanza_set_name(query, "query");
		xmpp_stanza_set_ns(query, XMPP_NS_ROSTER);

		xmpp_stanza_add_child(iq, query);

		xmpp_stanza_release(query);

		xmpp_id_handler_add(conn, xmpp_connection_handle_reply, "roster1", ctx);

		xmpp_send(conn, iq);

		xmpp_stanza_release(iq);
	} else {
		fprintf(stderr, "DEBUG: disconnected\n");
		xmpp_stop(ctx);
	}
}

int main(int argc, char *argv[])
{
	pthread_t fthread;
	struct fuse_args_xmpp *args;
	args = (struct fuse_args_xmpp *)malloc(sizeof(fuse_args));
	args->argc=argc;
	args->argv=argv;
	pthread_create(&fthread,NULL,fuse_thread,args);

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
	xmpp_ctx_free(ctx);

	xmpp_shutdown();

	return 0;
}
