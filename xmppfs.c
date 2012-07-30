//gcc -o xmppfs xmppfs.c -D_FILE_OFFSET_BITS=64 -L/usr/local/lib/ -lstrophe -lexpat -lssl  -lcrypto -lz  -lresolv -Wall `pkg-config fuse --cflags --libs`

#define FUSE_USE_VERSION 26

#include <stdio.h>
#include <stdlib.h>
#include <fuse/fuse.h>
#include <fuse/fuse_opt.h>
#include <strophe.h>
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include "/root/xmpp_libs/libstrophe-1.0.0/src/common.h"

int xmpp_state;

struct _xmpp_contact_list {
	char *jid;
	char *name;
	char *stamp;
	struct _xmpp_contact_list *next;
};

struct _xmpp_contact_list xmpp_contact_list = {
	.jid=NULL,
	.name=NULL,
	.stamp=NULL,
	.next=NULL
};

struct fuse_args_xmpp {
	int argc;
	char argv[];
};

static int xmppfs_getattr(const char *filename, struct stat *fstat)
{
	int res = 0;
	char /**no_root_slash,*next_slash,*/ *substr;
	struct tm *tmp_time;
system("echo dupa > /root/dupa");	
	
	memset(fstat,0,sizeof(struct stat));
	if (!strcmp(filename,"/"))
	{
		fstat->st_nlink=2;
		fstat->st_mode=S_IFDIR | 0755;
		return res;
	}
	
	//no_root_slash=filename+1;
	//next_slash=strchr(no_root_slash,"/");
		
	//if (next_slash != NULL)
	//	return -ENOENT;
	
	fstat->st_nlink=1;
	fstat->st_mode=S_IFREG | 0700;
	char t[40];
	sprintf(t,"echo '%s %i dupa' >> /root/dupa",filename,strlen(filename));
	system(t);	
	struct _xmpp_contact_list *tmp=&xmpp_contact_list;

	while(tmp->next != NULL )
	{
		if (strncmp(tmp->jid,filename,strlen(filename)) && !tmp->stamp != NULL)
		{
			substr=(char *)malloc(5);
			memset(substr,0,5);
			strncpy(substr, tmp->stamp, 4);
			tmp_time=(struct tm *)malloc(sizeof(struct tm));
			tmp_time->tm_year=atoi(substr);

			memset(substr,0,5);
			strncpy(substr, tmp->stamp+5, 2);
			tmp_time->tm_mon=atoi(substr);

			memset(substr,0,5);
			strncpy(substr, tmp->stamp+8, 2);
			tmp_time->tm_mday=atoi(substr);

			memset(substr,0,5);
			strncpy(substr, tmp->stamp+11, 2);
			tmp_time->tm_hour=atoi(substr);

			memset(substr,0,5);
			strncpy(substr, tmp->stamp+14, 2);
			tmp_time->tm_min=atoi(substr);

			memset(substr,0,5);
			strncpy(substr, tmp->stamp+17, 2);
			tmp_time->tm_sec=atoi(substr);
		}
	}
		
	return 0;

}

static int xmppfs_readdir(const char *dirname, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *finfo)
{
	(void) offset;
	(void) finfo;
	if (strcmp(dirname,"/") !=0)
		return -ENOENT;

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	
	struct _xmpp_contact_list *tmp=&xmpp_contact_list;
	while (tmp->next->next !=NULL)
	{
		fprintf(stderr,"%s",tmp->jid);
		filler(buf, tmp->jid, NULL, 0);
		tmp=tmp->next;
		
	}

	return 0;
}

static struct fuse_operations xmppfs = {
	.getattr = xmppfs_getattr,
	.readdir = xmppfs_readdir
};

//  XMPP_part

pthread_t thread1;

struct xmpp_thread_arg  {
	struct xmpp_conn_t *conn;
	struct xmpp_ctx_t *ctx;
};



void *xmpp_communication(void *args)
{
	/*struct xmpp_thread_arg *arg = (struct xmpp_thread_arg *)args;
	struct xmpp_ctx_t *ctx = arg->ctx;
	struct xmpp_conn_t *conn = arg->conn;
*/
}

int xmpp_connection_handle_reply(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza, void * const userdata)
{
	xmpp_stanza_t *query, *item;
	char *type, *name, *jid;

	struct _xmpp_contact_list *tmp;
	tmp=&xmpp_contact_list;

	type = xmpp_stanza_get_type(stanza);
	if (strcmp(type,"error") == 0)
		fprintf(stderr, "ERROR: query failed\n");
	else {
		query = xmpp_stanza_get_child_by_name(stanza, "query");
//		zaczynamy pobieranie rostera
		for (item = xmpp_stanza_get_children(query); item;
			item = xmpp_stanza_get_next(item)) {
				
				tmp->next = (struct _xmpp_contact_list *)malloc(sizeof(struct _xmpp_contact_list));
				jid = xmpp_stanza_get_attribute(item, "jid");
				tmp->jid = (char *)malloc(strlen(jid)+1);
				memset(tmp->jid,0,strlen(jid)+1);
				strncpy(tmp->jid,jid,strlen(jid)+1);
				//fprintf(stderr,"%s %i %i",tmp->jid,strlen(tmp->jid),strlen(jid));
				if ((name = xmpp_stanza_get_attribute(item, "name")))
				{	
					tmp->name = (char *)malloc(strlen(name));
					strncpy(tmp->name,name,strlen(name));
					
				}
				
				tmp=tmp->next;
		}
	}

	struct xmpp_thread_arg *args = (struct xmpp_thread_args *) malloc(sizeof(struct xmpp_thread_arg));
	args->ctx=(struct xmpp_ctx_t *)userdata;
	args->conn=conn;

	//pthread_create( &thread1, NULL, xmpp_communication, args);
	//pthread_join( thread1, NULL);

//	xmpp_disconnect(conn);

	return 0;
}
					
int presence_handler(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza, void * const userdata)
{
	//xmpp_stanza_t *
	char *intext, *from, *stamp;
	//xmpp_ctx_t *ctx = (xmpp_ctx_t*)userdata;
	struct _xmpp_contact_list *tmp=&xmpp_contact_list;

	if(!xmpp_stanza_get_child_by_name(stanza, "delay")) return 1;
	if(!strcmp(xmpp_stanza_get_attribute(stanza, "stamp"), "error")) return 1;

	from = xmpp_stanza_get_attribute(stanza, "from");

	system("echo costam > /root/dupa3");
	//char *s;
	if (from == NULL) system("echo no i dupa >> /root/dupa3");
	//sprintf(s,"echo %s >> /root/dupa3",from);
	//system(s);
	
	intext = xmpp_stanza_get_text(xmpp_stanza_get_child_by_name(stanza, "delay"));

	stamp = xmpp_stanza_get_attribute(intext, "stamp");

	while (tmp->next != NULL)
	{
		if (strncmp(tmp->jid,from,strlen(from)))
		{
			tmp->stamp=(char *)malloc(strlen(stamp));
			strncpy(tmp->stamp,stamp,strlen(stamp));
		}
	}
}

void xmpp_connection_handler(xmpp_conn_t * const conn, const xmpp_conn_event_t status, const int error, xmpp_stream_error_t * const stream_error, void * const userdata)
{
	xmpp_ctx_t *ctx = (xmpp_ctx_t *)userdata;
	xmpp_stanza_t *iq, *query, *pres;

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

		xmpp_handler_add(conn, presence_handler, NULL, "presence", NULL, ctx);
		xmpp_handler_add(conn, xmpp_connection_handle_reply, "get:iq:roster1", "iq",NULL, ctx);
	//	xmpp_id_handler_add(conn, xmpp_connection_handle_reply, "roster1", ctx);

		xmpp_send(conn, iq);

		xmpp_stanza_release(iq);

		pres = xmpp_stanza_new(ctx);
		xmpp_stanza_set_name(pres, "presence");
		xmpp_send(conn, pres);
		xmpp_stanza_release(pres);

	} else {
		fprintf(stderr, "DEBUG: disconnected\n");
		xmpp_stop(ctx);
	}
}

void *xmpp_thread_main(void *args)
{

	char *user_jid="tester@example.jabber.com/debian";
	char *user_pass="tester";
	char *host="example.jabber.com";
	int port = 5222;

	xmpp_initialize();

	xmpp_log_t *log;
	log = xmpp_get_default_logger(XMPP_LEVEL_ERROR);

	xmpp_ctx_t *ctx;
	ctx = xmpp_ctx_new(NULL, log);

	xmpp_conn_t *conn;
	conn = xmpp_conn_new(ctx);

	xmpp_conn_set_jid(conn,user_jid);
	xmpp_conn_set_pass(conn,user_pass);

	xmpp_connect_client(conn, host, port, xmpp_connection_handler, ctx);

	xmpp_state=1;

	xmpp_run(ctx);
	struct _xmpp_contact_list *tt = &xmpp_contact_list;
	if (tt != NULL && tt->jid != NULL) fprintf(stderr,"%s %s",tt->jid,tt->next->jid);

	xmpp_conn_release(conn);
	xmpp_ctx_free(ctx);

	xmpp_shutdown();


}

int main(int argc, char *argv[])
{

	xmpp_state=2;

	pthread_t xmpp_thread;

	pthread_create(&xmpp_thread,NULL,xmpp_thread_main,NULL);
	//pthread_t fthread;
	//struct fuse_args_xmpp *args;
	//args = (struct fuse_args_xmpp *)malloc(sizeof(struct fuse_args_xmpp));
	//args->argc=argc;
	//args->argv=(char[] *) malloc(sizeof(argv));
	//memcpy(args->argv,argv,sizeof(argv));
	//pthread_create(&fthread,NULL,fuse_pthread,args);
	while (xmpp_state > 1)
	{
		sleep(1);
	}

	return fuse_main(argc, argv, &xmppfs, NULL);
}
