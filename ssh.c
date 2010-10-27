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

/*
 * http://www.libssh2.org
 */

int tcp_connect(const char* hostname, int port) {
	// TODO: kvo->hostname must be numerical...
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
// ssh_get_file()
// Connect to an SSH server and get the specified file
//

int ssh_get_file(tFileDescription* kvo, void** data, ssize_t* length) {
	// Connect to the server
	int sock = tcp_connect(kvo->hostname, kvo->port);
	if (!sock) {
		fprintf(stderr, "failed to connect!\n");
		return 0;
	}

	// Open an SSH session...
	LIBSSH2_SESSION* session = libssh2_session_init();
	if (libssh2_session_startup(session, sock)) {
		fprintf(stderr, "Failure establishing SSH session\n");
		close(sock);
		return 0;
	}

	// Verify the fingerprint... (TODO: store it in the kvo_file)
	const char* fingerprint = libssh2_hostkey_hash(session, LIBSSH2_HOSTKEY_HASH_MD5);
	hexdump("fingerprint",fingerprint,16);printf("\n");   
	if (kvo->fingerprint && (memcmp(fingerprint,(char*)kvo->fingerprint,16) != 0)) {
		hexdump("fingerprint",kvo->fingerprint,16);printf("\n");   
		printf("Incorrect fingerprint\n");
		goto shutdown;
	}
	if (!kvo->fingerprint)
		kvo->fingerprint=malloc(16);
	memcpy(kvo->fingerprint,fingerprint,16);

	/* check what authentication methods are available */
	char* userauthlist = libssh2_userauth_list(session, kvo->username, strlen(kvo->username));
	printf("Authentication methods: %s\n", userauthlist);

	// If a password is given, try that one...
	if (kvo->password && *kvo->password) {
		/* We could authenticate via password */
		if (libssh2_userauth_password(session, kvo->username, kvo->password)) {
			printf("\tAuthentication by password failed!\n");
		} 
		else {
			printf("\tAuthentication by password succeeded.\n");
			goto ssh_password_succeeded;
		}
	}
	
	// Request a password from the user
ssh_request_user_password:;
	gchar* password = dialog_request_password(NULL, "SSH server");
	if (!password)
		goto shutdown;
	if (libssh2_userauth_password(session, kvo->username, password)) {
		printf("\tAuthentication by password failed!\n");
		g_free(password);
		goto ssh_request_user_password;
	} 
	else {
		printf("\tAuthentication by password succeeded.\n");
	}
	g_free(password);

ssh_password_succeeded:
	// SFTP...
	fprintf(stderr, "Perform: libssh2_sftp_init()\n");
	LIBSSH2_SFTP* sftp_session = libssh2_sftp_init(session);
	if (!sftp_session) {
		fprintf(stderr, "Unable to init SFTP session\n");
		goto shutdown;
	}
	else {
		fprintf(stderr, "SFTP session initialized\n");
	}

	// Request a file via SFTP
	fprintf(stderr, "Perform: libssh2_sftp_open('%s')\n",kvo->filename);
	LIBSSH2_SFTP_HANDLE* sftp_handle = libssh2_sftp_open(sftp_session, kvo->filename, LIBSSH2_FXF_READ, 0);
	if (!sftp_handle) {
		fprintf(stderr, "Unable to open file with SFTP\n");
		goto shutdown;
	}
	else {
		fprintf(stderr, "SFTP handle openened\n");
	}

	// Stat the file
	struct _LIBSSH2_SFTP_ATTRIBUTES attrs;
	int err = libssh2_sftp_fstat(sftp_handle, &attrs);
	if (err) {
		fprintf(stderr, "Could not stat the file\n");
		goto shutdown;
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

shutdown:
	libssh2_session_disconnect(session, "Normal Shutdown, Thank you for playing");
	libssh2_session_free(session);

	close(sock);

	return *length;
}
