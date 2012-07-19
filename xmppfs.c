#include <stdio.h>
#include <stdlib.h>
#include <fuse/fuse.h>
#include <fuse/fuse_opt.h>
#include <strophe.h>


static int xmppfs_getattr(const char *filename, struct stat *fstat)
{

	return 0;

}

//static int xmppfs_readdir(const char *dirname, void *buf, fuse_fill_dir_t filler, off_t 		offset, struct fuse_file_info *finfo);

static struct fuse_operations xmppfs = {
	.getattr = xmppfs_getattr,
	//.readdir = xmppfs_readdir
};

int main(int argc, char *argv[])
{
	char *user_jid="testet@example.jabber.com/debian";
	char *user_pass="tester";

	xmpp_initialize();

	xmpp_log_t *log;
	log = xmpp_get_default_logger(XMPP_LEVEL_DEBUG);

	xmpp_ctx_t *ctx;
	ctx = xmpp_ctx_new(NULL, log);

	xmpp_conn_t *conn;
	conn = xmpp_conn_new(ctx);

	xmpp_conn_set_jid(conn,user_jid);
	xmpp_conn_set_pass(conn,user_pass);

	return 0;
}
