#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <libssh2.h>
#include <libssh2_sftp.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "functions.h"
#include "ssh.h"
#include "gtk_dialogs.h"

// The SSH connection
struct tSsh {
	int sock;
	LIBSSH2_SESSION* session;
};

/*
 * http://www.libssh2.org
 */

int tcp_connect(const char* hostname, int port) {
	// TODO: hostname must be numerical...
	unsigned long hostaddr = inet_addr(hostname);
	if (!hostaddr)
		return 0;
	printf("hostaddr: %08lX\n",hostaddr);

	int sock = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);
	sin.sin_addr.s_addr = hostaddr;
	if (connect(sock, (struct sockaddr*)(&sin),sizeof(struct sockaddr_in)) != 0)
		return 0;
	return sock;
}

// ------------------------------------------------------------------
//
// ssh_connect() - Connect to an SSH server
//

static int ssh_connect(struct tSsh* ssh, const char* hostname, int port, void** fingerprint) {
	ssh->sock = tcp_connect(hostname, port);
	if (!ssh->sock) {
		fprintf(stderr, "failed to connect!\n");
		return -1;
	}
	
	// Open an SSH session...
	ssh->session = libssh2_session_init();
	if (libssh2_session_startup(ssh->session, ssh->sock)) {
		fprintf(stderr, "Failure establishing SSH session\n");
		return -1;
	}

	// Verify the fingerprint...
	const void* server_fingerprint = libssh2_hostkey_hash(ssh->session, LIBSSH2_HOSTKEY_HASH_MD5);
	hexdump("fingerprint", server_fingerprint, 16);printf("\n");   
	if (*fingerprint && (memcmp(server_fingerprint, *fingerprint, 16) != 0)) {
		hexdump("fingerprint",*fingerprint,16);printf("\n");   
		printf("Incorrect fingerprint\n");
		return -1;
	}
	
	// Store the new fingerprint if it was not yet defined
	if (!*fingerprint)
		*fingerprint=malloc(16);
	memcpy(*fingerprint, server_fingerprint, 16);

	return 0;
}

// ------------------------------------------------------------------
//
// ssh_login() - Login to an SSH server
//

static int ssh_login(struct tSsh* ssh, const char* username, const char* default_password) {
	char* userauthlist = libssh2_userauth_list(ssh->session, username, strlen(username));
	printf("Authentication methods: %s\n", userauthlist);

	// Is there a default password defined
	gchar* password = NULL;
	if (default_password && *default_password)
		password = strdup(default_password);

	// Stay in the loop until, we logged in, or the user cancelled
	while(1) {
		// If a password is given, try that one...
		if (password) {
			int err = libssh2_userauth_password(ssh->session, username, password);
			g_free(password);
			// We could authenticate via password
			if (err == 0) {
				printf("Authentication by password succeeded.\n");
				return 0;
			}
			printf("Authentication by password failed!\n");
		}
		password = gtk_password_dialog(NULL, "Enter password for SSH server");
		if (!password)
			return 1;
	}
}

// ------------------------------------------------------------------
//
// ssh_read() - Read data frm an ssh handle
//

static int ssh_read(struct tSsh* ssh, const char* filename, void** data, ssize_t* length) {
	// SFTP...
	fprintf(stderr, "Perform: libssh2_sftp_init()\n");
	LIBSSH2_SFTP* sftp_session = libssh2_sftp_init(ssh->session);
	if (!sftp_session) {
		fprintf(stderr, "Unable to init SFTP session\n");
		return -1;
	}
	else {
		fprintf(stderr, "SFTP session initialized\n");
	}

	// Request a file via SFTP
	fprintf(stderr, "Perform: libssh2_sftp_open('%s')\n",filename);
	LIBSSH2_SFTP_HANDLE* sftp_handle = libssh2_sftp_open(sftp_session, filename, LIBSSH2_FXF_READ, 0);
	if (!sftp_handle) {
		fprintf(stderr, "Unable to open file with SFTP\n");
		libssh2_sftp_shutdown(sftp_session);
		return -1;
	}
	else {
		fprintf(stderr, "SFTP handle openened\n");
	}

	// Stat the file
	struct _LIBSSH2_SFTP_ATTRIBUTES attrs;
	int err = libssh2_sftp_fstat(sftp_handle, &attrs);
	if (err) {
		fprintf(stderr, "Could not stat the file\n");
		libssh2_sftp_close(sftp_handle);
		libssh2_sftp_shutdown(sftp_session);
		return -1;
	}

	// Allocate memory to store the file
	*length = attrs.filesize;
	printf("filesize: %zi\n", *length);
	*data = malloc(*length);

	// Open the local file for writing
	fprintf(stderr, "libssh2_sftp_open() is done, now receive data!\n");	
	int total=0;
	while(total < attrs.filesize) {
		printf(".");
		int rx = attrs.filesize-total;
		rx=libssh2_sftp_read(sftp_handle, *data+total, rx);
		if (rx <= 0) break;
		total+=rx;
	}

	libssh2_sftp_close(sftp_handle);
	libssh2_sftp_shutdown(sftp_session);
	return 0;
}

// ------------------------------------------------------------------
//
// ssh_write() - Write data frm an ssh handle
//

static int ssh_write(struct tSsh* ssh, const char* filename, void* data, ssize_t length) {
	// SFTP...
	fprintf(stderr, "Perform: libssh2_sftp_init()\n");
	LIBSSH2_SFTP* sftp_session = libssh2_sftp_init(ssh->session);
	if (!sftp_session) {
		fprintf(stderr, "Unable to init SFTP session\n");
		return -1;
	}
	else {
		fprintf(stderr, "SFTP session initialized\n");
	}

	// Request a file via SFTP
	fprintf(stderr, "Perform: libssh2_sftp_open('%s')\n",filename);
	LIBSSH2_SFTP_HANDLE* sftp_handle = libssh2_sftp_open(sftp_session, filename, LIBSSH2_FXF_WRITE, 0);
	if (!sftp_handle) {
		fprintf(stderr, "Unable to open file with SFTP\n");
		libssh2_sftp_shutdown(sftp_session);
		return -1;
	}
	else {
		fprintf(stderr, "SFTP handle openened\n");
	}
	
	// TODO: libssh2_sftp_rename

	// Start writing
	fprintf(stderr, "Sending data\n");	
	int total=0;
	while(total < length) {
		printf(".");
		int tx = tx-total;
		tx=libssh2_sftp_write(sftp_handle, data+total, tx);
		if (tx <= 0) break;
		total+=tx;
	}

	libssh2_sftp_close(sftp_handle);
	libssh2_sftp_shutdown(sftp_session);
	return 0;
}

// ------------------------------------------------------------------
//
// ssh_get_file() - Read a file from a SSH server
//

int ssh_get_file(tFileDescription* kvo, void** data, ssize_t* length) {
	*length=0;
	int err=0;

	struct tSsh* ssh = mallocz(sizeof(struct tSsh));

	// Connect to the server
	err = ssh_connect(ssh, kvo->hostname, kvo->port, &kvo->fingerprint);
	if (err) goto shutdown;

	// check what authentication methods are available
	err = ssh_login(ssh, kvo->username, kvo->password);
	if (err) goto shutdown;

	// Read the file...
	err = ssh_read(ssh, kvo->filename, data, length);
	if (err) goto shutdown;

shutdown:
	if (ssh->session) {
		libssh2_session_disconnect(ssh->session, "Normal Shutdown, Thank you for playing");
		libssh2_session_free(ssh->session);
	}

	if (ssh->sock)
		close(ssh->sock);

	return *length;
}

// ------------------------------------------------------------------
//
// ssh_put_file() - Write a file from a SSH server
//

int ssh_put_file(tFileDescription* kvo, void* data, ssize_t length) {
	int err=0;
	struct tSsh* ssh = mallocz(sizeof(struct tSsh));

	// Connect to the server
	err = ssh_connect(ssh, kvo->hostname, kvo->port, &kvo->fingerprint);
	if (err) goto shutdown;

	// check what authentication methods are available
	err = ssh_login(ssh, kvo->username, kvo->password);
	if (err) goto shutdown;

	// Read the file...
	err = ssh_write(ssh, kvo->filename, data, length);
	if (err) goto shutdown;

shutdown:
	if (ssh->session) {
		libssh2_session_disconnect(ssh->session, "Normal Shutdown, Thank you for playing");
		libssh2_session_free(ssh->session);
	}

	if (ssh->sock)
		close(ssh->sock);

	return err;
}
