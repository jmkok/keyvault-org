#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <libssh2.h>
#include <libssh2_sftp.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "functions.h"
#include "ssh.h"
#include "gtk_dialogs.h"

#define min(a,b) (a<b?a:b)

// The SSH connection
struct tSsh {
	int sock;
	LIBSSH2_SESSION* session;
};

/*
 * http://www.libssh2.org
 */

static void tcp_socket_timeout(int sock, int timeout) {
	struct timeval tv;
	tv.tv_sec = timeout;
	tv.tv_usec = 0;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
}


int tcp_connect(const char* hostname, int port) {
	printf("tcp_connect('%s',%i)\n",hostname, port);
	if (!port)
		port=22;

	// Resolve the hostname
	struct hostent* host = gethostbyname(hostname);
	printf("\thostaddr: %u.%u.%u.%u (%u)\n", 
		(uint8_t)host->h_addr_list[0][0], 
		(uint8_t)host->h_addr_list[0][1], 
		(uint8_t)host->h_addr_list[0][2], 
		(uint8_t)host->h_addr_list[0][3], 
		host->h_length);

	// Create the TCP socket
	int sock = socket(AF_INET, SOCK_STREAM, 0);

	// Set the timeout to 5 seconds
	tcp_socket_timeout(sock,5);

	// Connect to the server
	struct sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);
	memcpy(&sin.sin_addr.s_addr, host->h_addr_list[0], 4);
	if (connect(sock, (struct sockaddr*)(&sin),sizeof(struct sockaddr_in)) != 0)
		return 0;
	return sock;
}

// ------------------------------------------------------------------
//
// ssh_connect() - Connect to an SSH server
//

static int ssh_connect(struct tSsh* ssh, const char* hostname, int port, void** fingerprint) {
	printf("ssh_connect(%p, '%s', %u, %p)\n", ssh, hostname, port, fingerprint);
	ssh->sock = tcp_connect(hostname, port);
	if (!ssh->sock) {
		gtk_error("Failed to connect to the SSH server");
		return -1;
	}
	
	// Open an SSH session...
	ssh->session = libssh2_session_init();
	if (libssh2_session_startup(ssh->session, ssh->sock)) {
		gtk_error("Failed establishing an SSH session");
		return -1;
	}

	// Verify the fingerprint...
	const void* server_fingerprint = libssh2_hostkey_hash(ssh->session, LIBSSH2_HOSTKEY_HASH_MD5);
	hexdump(server_fingerprint, 16);
	if (*fingerprint && (memcmp(server_fingerprint, *fingerprint, 16) != 0)) {
		hexdump(*fingerprint,16);
		gtk_error("Incorrect fingerprint for remote SSH server\nMore info: http://en.wikipedia.org/wiki/Public_key_fingerprint");
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

static int ssh_login(struct tSsh* ssh, struct FILE_LOCATION* kvo, const char* username, const char* default_password) {
	printf("ssh_login(%p, %p, '%s', '%s')\n", ssh, kvo, username, default_password);
	char* userauthlist = libssh2_userauth_list(ssh->session, username, strlen(username));
	printf("\tAuthentication methods: %s\n", userauthlist);

	// Is there a default password defined
	gchar* password = NULL;
	if (default_password && *default_password)
		password = strdup(default_password);

	// Stay in the loop until, we logged in, or the user cancelled
	while(1) {
		// If a password is given, try that one...
		if (password) {
			int err = libssh2_userauth_password(ssh->session, username, password);
			// We could authenticate via password
			if (err == 0) {
				if (!kvo->password)
					kvo->password = password;
				else if (strcmp(kvo->password, password) != 0) {
					free(kvo->password);
					kvo->password = password;
				}
				printf("\tAuthentication by password succeeded.\n");
				return 0;
			}
			g_free(password);
			printf("\tAuthentication by password failed!\n");
		}
		password = gtk_dialog_password(NULL, "Enter password for SSH server");
		if (!password)
			return 1;
	}
}

// ------------------------------------------------------------------
//
// ssh_read() - Read data frm an ssh handle
//

static int ssh_read(struct tSsh* ssh, const char* filename, void** data, ssize_t* length) {
	printf("ssh_read(%p, '%s', ...)", ssh, filename);
	// SFTP...
	printf("\tPerform: libssh2_sftp_init()\n");
	LIBSSH2_SFTP* sftp_session = libssh2_sftp_init(ssh->session);
	if (!sftp_session) {
		gtk_error("Unable to initialize the SFTP session");
		return -1;
	}
	else {
		printf("\tSFTP session initialized\n");
	}

	// Request a file via SFTP
	printf("\tPerform: libssh2_sftp_open('%s')\n",filename);
	LIBSSH2_SFTP_HANDLE* sftp_handle = libssh2_sftp_open(sftp_session, filename, LIBSSH2_FXF_READ, 0);
	if (!sftp_handle) {
		gtk_error("Unable to open the requested file using SFTP");
		libssh2_sftp_shutdown(sftp_session);
		return -1;
	}
	else {
		printf("\tSFTP handle openened\n");
	}

	// Stat the file
	struct _LIBSSH2_SFTP_ATTRIBUTES attrs;
	int err = libssh2_sftp_fstat(sftp_handle, &attrs);
	if (err) {
		gtk_error("Could not stat the file on the SSH server");
		libssh2_sftp_close(sftp_handle);
		libssh2_sftp_shutdown(sftp_session);
		return -1;
	}

	// Allocate memory to store the file
	*length = attrs.filesize;
	*data = malloc(*length);

	// Open the local file for writing
	printf("\tReceive %zu bytes!\n",*length);
	int total=0;
	while(total < *length) {
		int rx = *length-total;
		rx=libssh2_sftp_read(sftp_handle, *data+total, rx);
		printf("\trx: %u (%u / %zu)\n",rx,total,*length);	
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
	printf("ssh_write(%p, '%s', %p, %zu)\n", ssh, filename, data, length);

	// SFTP...
	printf("\tPerform: libssh2_sftp_init()\n");
	LIBSSH2_SFTP* sftp_session = libssh2_sftp_init(ssh->session);
	if (!sftp_session) {
		gtk_error("Unable to initialize the SFTP session");
		return -1;
	}
	else {
		printf("\tSFTP session initialized\n");
	}

	// Request a file via SFTP
	printf("\tPerform: libssh2_sftp_open('temp.kvo')\n");
	LIBSSH2_SFTP_HANDLE* sftp_handle = libssh2_sftp_open(
			sftp_session,
			"temp.kvo",
			LIBSSH2_FXF_WRITE | LIBSSH2_FXF_CREAT | LIBSSH2_FXF_TRUNC,
			LIBSSH2_SFTP_S_IRUSR | LIBSSH2_SFTP_S_IWUSR
		);
	if (!sftp_handle) {
		gtk_error("Unable to open the requested file using SFTP");
		libssh2_sftp_shutdown(sftp_session);
		return -1;
	}
	else {
		printf("\tSFTP handle openened\n");
	}
	
	// Start writing (only 16kB blocks - 32kB and/or larger blocks fail...)
	printf("\tSending %zu bytes\n", length);	
	int total=0;
	while(total < length) {
		int tx = libssh2_sftp_write(sftp_handle, data+total, (min((16*1024),length-total)));
		printf("\ttx: %u (%u / %zu)\n", tx, total, length);	
		if (tx == 0)
			continue;
		if (tx <= 0) {
			gtk_error("Writing failed");
			break;
		}
		total += tx;
	}

	// Close the file
	int retval = libssh2_sftp_close(sftp_handle);
	printf("retval: %i\n",retval);
	
	// libssh2_sftp_rename (it seems I am not able to overwrite files...)
	if ((retval == 0) && (total == length)) {
		// remove the current backup.kvo
		libssh2_sftp_unlink(sftp_session, "backup.kvo");
		// rename the current file to backup.kvo
		retval = libssh2_sftp_rename(sftp_session, filename, "backup.kvo");
		if (retval != 0) {
			gtk_error("Failed to rename file");
			return -1;
		}
		// rename the new file to the filename
		retval = libssh2_sftp_rename(sftp_session, "temp.kvo", filename);
		if (retval != 0) {
			gtk_error("Failed to rename file");
			return -1;
		}
		gtk_info("File successfully written");
	}
	else {
		libssh2_sftp_unlink(sftp_session, "temp.kvo");
		gtk_error("Removing temporary file");
	}

	// Close the session
	libssh2_sftp_shutdown(sftp_session);
	return 0;
}

// ------------------------------------------------------------------
//
// ssh_agent_login()
// http://www.libssh2.org/examples/ssh2_agent.html
//

int ssh_agent_login(struct tSsh* ssh, const char* username) {
	/* Connect to the ssh-agent */ 
	LIBSSH2_AGENT* agent = libssh2_agent_init(ssh->session);
	if (!agent) {
		fprintf(stderr, "Failure initializing ssh-agent support\n");
		return -1;
	}

	if (libssh2_agent_connect(agent)) {
		fprintf(stderr, "Failure connecting to ssh-agent\n");
		return -1;
	}

	if (libssh2_agent_list_identities(agent)) {
		fprintf(stderr, "Failure requesting identities to ssh-agent\n");
		return -1;
	}

	struct libssh2_agent_publickey *identity, *prev_identity = NULL;
	while (1) {
		int rc = libssh2_agent_get_identity(agent, &identity, prev_identity);
		if (rc == 1) {
			fprintf(stderr, "Couldn't continue authentication\n");
			return -1;
		}
		if (rc < 0) {
			fprintf(stderr, "Failure obtaining identity from ssh-agent support\n");
			return -1;
		}
		if (libssh2_agent_userauth(agent, username, identity)) {
			printf("\tAuthentication with username %s and public key %s failed!\n", username, identity->comment);
		}
		else {
			printf("\tAuthentication with username %s and public key %s succeeded!\n", username, identity->comment);
			return 0;
		}
		prev_identity = identity;
	}
} 

// ------------------------------------------------------------------
//
// ssh_get_file() - Read a file from a SSH server
//

int ssh_get_file(struct FILE_LOCATION* kvo, void** data, ssize_t* length) {
	*length=0;
	int err=0;

	struct tSsh* ssh = mallocz(sizeof(struct tSsh));

	// Connect to the server
	err = ssh_connect(ssh, kvo->hostname, kvo->port, &kvo->fingerprint);
	if (err) goto shutdown;

	// Use the SSH agent to connect
	err = ssh_agent_login(ssh, kvo->username);
	if (err) {
		// check what authentication methods are available
		err = ssh_login(ssh, kvo, kvo->username, kvo->password);
		if (err) goto shutdown;
	}

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

	return err;
}

// ------------------------------------------------------------------
//
// ssh_put_file() - Write a file from a SSH server
//

int ssh_put_file(struct FILE_LOCATION* kvo, void* data, ssize_t length) {
	printf("ssh_put_file(%p, %p, %zu)", kvo, data, length);
	int err=0;
	struct tSsh* ssh = mallocz(sizeof(struct tSsh));

	// Connect to the server
	err = ssh_connect(ssh, kvo->hostname, kvo->port, &kvo->fingerprint);
	if (err) goto shutdown;

	// check what authentication methods are available
	err = ssh_login(ssh, kvo, kvo->username, kvo->password);
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
