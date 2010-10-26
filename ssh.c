#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <libssh2.h>
#include <libssh2_sftp.h>
//~ #include <glib-2.0/glib/gprintf.h>

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

void ssh_get_file(tKvoFile* kvo) {
	// TODO: kvo->hostname
	unsigned long hostaddr=inet_addr("192.168.9.1");
	printf("hostaddr: %08lX\n",hostaddr);

	int sock = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_port = htons(22);
	sin.sin_addr.s_addr = hostaddr;
	if (connect(sock, (struct sockaddr*)(&sin),sizeof(struct sockaddr_in)) != 0) {
		fprintf(stderr, "failed to connect!\n");
		return;
	}

	// Open an SSH session...
	LIBSSH2_SESSION* session = libssh2_session_init();
	if (libssh2_session_startup(session, sock)) {
		fprintf(stderr, "Failure establishing SSH session\n");
		close(sock);
		return;
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
			goto shutdown;
		} 
		else {
			printf("\tAuthentication by password succeeded.\n");
		}
	}
	
	// Request a password from the user
	gchar* password = dialog_request_password(NULL, "SSH server");
	if (libssh2_userauth_password(session, kvo->username, password)) {
		printf("\tAuthentication by password failed!\n");
		goto shutdown;
	} 
	else {
		printf("\tAuthentication by password succeeded.\n");
	}
	g_free(password);

	// SFTP...
	fprintf(stderr, "libssh2_sftp_init()!\n");
	LIBSSH2_SFTP* sftp_session = libssh2_sftp_init(session);

	if (!sftp_session) {
		fprintf(stderr, "Unable to init SFTP session\n");
		goto shutdown;
	}

	fprintf(stderr, "libssh2_sftp_open()!\n");
	/* Request a file via SFTP */
	LIBSSH2_SFTP_HANDLE* sftp_handle = libssh2_sftp_open(sftp_session, kvo->filename, LIBSSH2_FXF_READ, 0);

	if (!sftp_handle) {
		fprintf(stderr, "Unable to open file with SFTP\n");
		goto shutdown;
	}

	// Open the local file for writing
	FILE* local_file = fopen(kvo->local_filename,"wb");
	if (!local_file) {
		gtk_error_dialog(NULL, "Could not open local file '%s'", kvo->local_filename);
		goto shutdown;
	}

	fprintf(stderr, "libssh2_sftp_open() is done, now receive data!\n");
	
	#define BLOCK_SIZE 1024*1024
	void* buffer=g_malloc(BLOCK_SIZE);
	int rx;
	int total=0;
	while(1) {
		printf(".");
		rx=libssh2_sftp_read(sftp_handle, buffer, 1024);
		if (rx <= 0) break;
		fwrite(buffer,1,rx,local_file);
		total+=rx;
	}
	g_free(buffer);
	fclose(local_file);

	libssh2_sftp_close(sftp_handle);
	libssh2_sftp_shutdown(sftp_session);

shutdown:
	libssh2_session_disconnect(session, "Normal Shutdown, Thank you for playing");
	libssh2_session_free(session);

	close(sock);
	printf("all done!\n");
}
