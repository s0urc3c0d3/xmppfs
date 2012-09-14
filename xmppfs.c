//gcc -o xmppfs xmppfs.c -D_FILE_OFFSET_BITS=64 -L/usr/local/lib/ -lstrophe -lexpat -lssl  -lcrypto -lz  -lresolv -Wall `pkg-config fuse --cflags --libs`

#define FUSE_USE_VERSION 26
#define READBUF_LEN 1024
#define WRITEBUF_LEN 1024

#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>
#include <signal.h>
#include <unistd.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <fuse/fuse.h>
#include <fuse/fuse_opt.h>
#include <strophe.h>
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
//#include "/root/xmpp_libs/libstrophe-1.0.0/src/common.h"
#include "/root/libstrophe/src/common.h"

xmpp_ctx_t *ctx_new;
xmpp_conn_t *conn_new;

struct _xmppfs_args {
	char *jid;
	char *pass;
	char *mount;
	int port;
	char *host;
};

static struct fuse_server {
	pthread_t pid;
	struct fuse *fuse;
	struct fuse_chan *ch;
	int failed;
} fs;

struct _xmppfs_args xmppfs_args = {
	.jid = NULL,
	.pass = NULL,
	.mount = NULL,
	.port = 5222,
	.host = NULL,
};

struct _xmpp_contact_list {
	char *jid;
	char *name;
	char *stamp;
	struct _xmpp_contact_list *next;
	int rbuflen;
	int wbuflen;
	char *rbuf;
	char *wbuf;
};

int xmpp_status;

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
	char  *substr;
	struct tm *tmp_time;
	time_t time_of_presence;
	
	memset(fstat,0,sizeof(struct stat));
	if (!strcmp(filename,"/"))
	{
		fstat->st_nlink=2;
		fstat->st_mode=S_IFDIR | 0755;
		return res;
	}
	
	fstat->st_nlink=1;
	fstat->st_mode=S_IFREG | 0666;
	fstat->st_size=0;
	struct _xmpp_contact_list *tmp=&xmpp_contact_list;

	while(tmp->next != NULL )
	{
		if (strncmp(tmp->jid,filename+1,strlen(filename)-1) == 0 && tmp->stamp != NULL)
		{
			fstat->st_size=tmp->rbuflen;
			substr=(char *)malloc(5);
			memset(substr,0,5);
			strncpy(substr, tmp->stamp, 4);
			tmp_time=(struct tm *)malloc(sizeof(struct tm));
			tmp_time->tm_year=atoi(substr)-1900;

			memset(substr,0,5);
			strncpy(substr, tmp->stamp+4, 2);
			tmp_time->tm_mon=atoi(substr) - 1;

			memset(substr,0,5);
			strncpy(substr, tmp->stamp+6, 2);
			tmp_time->tm_mday=atoi(substr);

			memset(substr,0,5);
			strncpy(substr, tmp->stamp+9, 2);
			tmp_time->tm_hour=atoi(substr);

			memset(substr,0,5);
			strncpy(substr, tmp->stamp+12, 2);
			tmp_time->tm_min=atoi(substr);

			memset(substr,0,5);
			strncpy(substr, tmp->stamp+15, 2);
			tmp_time->tm_sec=atoi(substr);

			tmp_time->tm_isdst = -1;
			
			time_of_presence = mktime(tmp_time);
			fstat->st_atime = fstat->st_mtime = fstat->st_ctime = (unsigned long) time_of_presence;
		}
		tmp=tmp->next;
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

	while (tmp->next != NULL)
	{
		filler(buf, tmp->jid, NULL, 0);
		tmp=tmp->next;
		
	}

	return 0;
}

static int xmppfs_open(const char *filename, struct fuse_file_info *fi)
{
	//this function is puted here only for fuse to work. Always we want to allow opening files in xmpp
	return 0;
}

static int xmppfs_read(const char *filename, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	struct _xmpp_contact_list *tmp=&xmpp_contact_list;
	while (tmp->next != NULL)
	{
		if (strncmp(filename+1,tmp->jid,strlen(tmp->jid))==0)
		{
			if (tmp->rbuflen == 0) {size=0; return 0;}
			memset(buf,0,tmp->rbuflen);
			memcpy(buf,tmp->rbuf,tmp->rbuflen);
			memset(tmp->rbuf,0,READBUF_LEN);
			size=tmp->rbuflen;
			tmp->rbuflen=0;
		}
		tmp=tmp->next;
	}
	return size;
}

static int xmppfs_write(const char *filename, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	int s = strlen(filename)-1;
	char *replytext, *jid = (char *)malloc(s);
	strncpy(jid,filename+1,s);
	struct _xmpp_contact_list *tmp=&xmpp_contact_list;
	while(tmp->next != NULL)
	{
		if (strncmp(jid,tmp->jid,s) == 0)
		{
			xmpp_stanza_t *reply, *body, *text;
			reply = xmpp_stanza_new(ctx_new);
			xmpp_stanza_set_name(reply, "message");
			xmpp_stanza_set_type(reply, "chat");
			xmpp_stanza_set_attribute(reply, "to", tmp->jid);

			body = xmpp_stanza_new(ctx_new);
			xmpp_stanza_set_name(body, "body");

			replytext = malloc(size);
			memset(replytext,'\0',size);
			strncpy(replytext, buf,size-1);

			text = xmpp_stanza_new(ctx_new);
			xmpp_stanza_set_text(text, replytext);
			xmpp_stanza_add_child(body, text);
			xmpp_stanza_add_child(reply, body);

			xmpp_send(conn_new, reply);
			xmpp_stanza_release(reply);
			free(replytext);
			free(jid);
			return size;
		}
		tmp=tmp->next;
	}
	free(jid);
	return 0;
}

static int xmppfs_release(const char *filename, struct fuse_file_info *fi)
{
	//this function is required by VFS. But it always in xmppfs return 0
	return 0;
}

static int xmppfs_truncate (const char *filename, off_t size)
{
	//just for echo test > /path/jid to work
	return 0;
}

static struct fuse_operations xmppfs = {
	.getattr = xmppfs_getattr,
	.readdir = xmppfs_readdir,
	.open = xmppfs_open,
	.read = xmppfs_read,
	.write = xmppfs_write,
	.release = xmppfs_release,
	.truncate = xmppfs_truncate,
};

//  XMPP_part

pthread_t xmpp_thread;
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
	return 0;
}

int xmpp_connection_handle_reply(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza, void * const userdata)
{
	xmpp_stanza_t *query, *item;
	char *type, *name, *jid;

	xmpp_ctx_t *ctx = (xmpp_ctx_t *)userdata;
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
				memset(tmp->next,0,sizeof (struct _xmpp_contact_list));
				jid = xmpp_stanza_get_attribute(item, "jid");
				tmp->jid = (char *)malloc(strlen(jid)+1);
				strncpy(tmp->jid,jid,strlen(jid)+1);
				tmp->rbuflen=0;
				tmp->wbuflen=0;
				tmp->rbuf=(char *)malloc(READBUF_LEN);
				tmp->wbuf=(char *)malloc(WRITEBUF_LEN);
				if ((name = xmpp_stanza_get_attribute(item, "name")))
				{	
					tmp->name = (char *)malloc(strlen(name));
					strncpy(tmp->name,name,strlen(name));
					
				}
				
				tmp=tmp->next;
		}
		xmpp_stanza_t *pres;
		pres = xmpp_stanza_new(ctx);
		xmpp_stanza_set_name(pres, "presence");
		xmpp_send(conn, pres);
		xmpp_stanza_release(pres);
	}

	struct xmpp_thread_arg *args = (struct xmpp_thread_arg *) malloc(sizeof(struct xmpp_thread_arg));
	args->ctx=(struct xmpp_ctx_t *)userdata;
	args->conn=(struct xmpp_conn_t *)malloc(sizeof(xmpp_conn_t));
	memcpy(args->conn,conn,sizeof(xmpp_conn_t));

	pthread_create( &thread1, NULL, xmpp_communication, args);
	pthread_join( thread1, NULL);

//	xmpp_disconnect(conn);

	return 1;
}
					
int presence_handler(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza, void * const userdata)
{
	xmpp_stanza_t *x;
	char *from, *stamp;
	struct _xmpp_contact_list *tmp=&xmpp_contact_list;

	if(!xmpp_stanza_get_child_by_name(stanza, "x")) return 1;
	if(!xmpp_stanza_get_attribute(stanza, "from")) return 1;

	x = xmpp_stanza_get_child_by_name(stanza, "x");
	stamp = xmpp_stanza_get_attribute(x, "stamp");
	from = xmpp_stanza_get_attribute(stanza, "from");
	while (tmp->next != NULL)
	{
		
		if (strncmp(tmp->jid,from,strlen(from)))
		{
			tmp->stamp=(char *)malloc(strlen(stamp));
			strncpy(tmp->stamp,stamp,strlen(stamp));
		}
		tmp=tmp->next;
	}
	return 1;
}

int message_handler(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza, void * const userdata)
{
	char *from, *msgt=(char *)malloc(1024);
	xmpp_stanza_t *msg;
	int mlen;
	struct _xmpp_contact_list *tmp=&xmpp_contact_list;

	if(!xmpp_stanza_get_child_by_name(stanza, "body")) return 1;
	if(!xmpp_stanza_get_attribute(stanza, "from")) return 1;

	from = xmpp_stanza_get_attribute(stanza, "from");
	msg = xmpp_stanza_get_child_by_name(stanza ,"body");

	while(tmp->next != NULL)
	{
		if (strncmp(from,tmp->jid,strlen(tmp->jid)) == 0)
		{
			//zwiekszanie bufora
			msgt=xmpp_stanza_get_text(msg);
			mlen = strlen(msgt);
			memset(tmp->rbuf,0,READBUF_LEN);
			strncpy(tmp->rbuf,msgt,mlen);
			tmp->rbuflen=mlen;
		}
		tmp=tmp->next;
	}
	free(msgt);
	return 1;
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

		xmpp_handler_add(conn, presence_handler, NULL, "presence", NULL, ctx);
		xmpp_handler_add(conn, message_handler, NULL, "message", NULL, ctx);
		xmpp_handler_add(conn, xmpp_connection_handle_reply, "get:iq:roster1", "iq",NULL, ctx);
		xmpp_id_handler_add(conn, xmpp_connection_handle_reply, "roster1", ctx);

		xmpp_send(conn, iq);

		xmpp_stanza_release(iq);
	} else {
		fprintf(stderr, "DEBUG: disconnected\n");
		xmpp_stop(ctx);
	}
}

void *xmpp_thread_main(void *args)
{
	xmpp_initialize();

	xmpp_log_t *log;
	log = xmpp_get_default_logger(XMPP_LEVEL_DEBUG);

	xmpp_ctx_t *ctx;
	ctx = xmpp_ctx_new(NULL, log);

	ctx_new=ctx;

	xmpp_conn_t *conn;
	conn = xmpp_conn_new(ctx);

	conn_new=conn;

	xmpp_conn_set_jid(conn,xmppfs_args.jid);
	xmpp_conn_set_pass(conn,xmppfs_args.pass);

	xmpp_connect_client(conn, xmppfs_args.host, xmppfs_args.port, xmpp_connection_handler, ctx);


	xmpp_run(ctx);

	xmpp_conn_release(conn);
	xmpp_ctx_free(ctx);

	xmpp_shutdown();

	return 0;
}

void *fuse_thread(void *arg)
{
	if(arg) {}

	if(fuse_loop(fs.fuse) < 0) {
		perror("fuse_loop");
	}
	fs.failed = 1;
	return NULL;
}

void usage()
{
	printf("xmppfs [-h] [-r PORT] -j JID -p PASS help -m MOUNTPOINT -o HOST\n");
	printf("--help  -h	          This message\n");
	printf("--jid   -j JID            Set JID in format: login@FQDN/RESOURCE\n");
	printf("--pass  -p PASS           Set password\n");
	printf("--mount -m MOUNTPOINT     Set mount point\n");
	printf("--host  -h HOST           Set FQDN of server (this can be different from FQDN in JID\n");
	printf("--port  -p PORT           Set server port (default 5222)\n");
}

int main(int argc, char *argv[])
{
	int next_option=0;
	const char* const short_options="hj:p:m:o:r:";
	const struct option long_options[]={
		{"help",        0,NULL,'h'},
		{"jid",		1,NULL,'j'},
		{"pass",	1,NULL,'p'},
		{"mount",	1,NULL,'m'},
		{"host",	1,NULL,'o'},
		{"port",	1,NULL,'r'},
		{NULL,          0,NULL,0}
	};
	while (next_option !=-1) {
		next_option=getopt_long(argc,argv,short_options, long_options,NULL);
		switch (next_option)
		{
			case 'h':
				usage();
				break;
			
			case 'j':
				xmppfs_args.jid = (char *)malloc(strlen(optarg));
				strcpy(xmppfs_args.jid,optarg);
				break;

			case 'p':
				xmppfs_args.pass = (char *)malloc(strlen(optarg));
				strcpy(xmppfs_args.pass,optarg);
				break;
			case 'm':
				xmppfs_args.mount = (char *)malloc(strlen(optarg));
				strcpy(xmppfs_args.mount,optarg);
				break;
			case 'o':
				xmppfs_args.host = (char *)malloc(strlen(optarg));
				strncpy(xmppfs_args.host,optarg,strlen(optarg));
				break;
			case 'r':
				xmppfs_args.port=(int) optarg;
				break;
		}
	}

/*	pid_t pid, sid;
	pid = fork();
	if (pid < 0) {
		exit(EXIT_FAILURE);
	}
	if (pid > 0) {
		exit(EXIT_SUCCESS);
	}

	umask(0);
                
                
	sid = setsid();
	if (sid < 0) {
		exit(EXIT_FAILURE);
	}
        
	if ((chdir("/")) < 0) {
		exit(EXIT_FAILURE);
	}
        
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);
*/

	xmpp_status=0;
	pthread_create(&xmpp_thread,NULL,xmpp_thread_main,NULL);

	struct fuse_args args = FUSE_ARGS_INIT(0, NULL);

	if (xmppfs_args.mount == NULL || xmppfs_args.jid == NULL || xmppfs_args.pass == NULL || xmppfs_args.host == NULL) 
	{
		usage();
		return 1;
	}
	
	//fprintf(stderr,"%s \n",xmppfs_args.mount);
	fs.ch = fuse_mount(xmppfs_args.mount, &args);

	fs.fuse = fuse_new(fs.ch, &args, &xmppfs, sizeof(xmppfs), NULL);
	pthread_create(&fs.pid, NULL, fuse_thread, NULL);

	while(!fs.failed) {
		sleep(1);
	}

	return 0;
}
