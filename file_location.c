#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>

#include "functions.h"
#include "file_location.h"
#include "configuration.h"
#include "ssh.h"
#include "xml.h"

// ---------------------------------------------------------------------
//
// Convert a text into an enum
//

enum FL_PROTO text_to_proto(const char* text) {
	if (strcmp(text,"local") == 0)
		return PROTO_FILE;
	if (strcmp(text,"ssh") == 0)
		return PROTO_SSH;
	return PROTO_UNKNOWN;
}

const char* proto_to_text(enum FL_PROTO protocol) {
	switch(protocol) {
		case PROTO_UNKNOWN:
			return "unknown";
		case PROTO_FILE:
			return "local";
		case PROTO_SSH:
			return "ssh";
		default:
			return "unknown";
	};
}

// ---------------------------------------------------------------------
//
// Local file handlers
//

static int read_local_file(struct FILE_LOCATION* loc, void** data, ssize_t* length) {
	/* Get the stat for the file
	 * We need to allocate data before read */
	struct stat buf;
	int err = stat(loc->filename, &buf);
	if (err)
		return -1;

	/* Open the file */
	FILE* fp = fopen(loc->filename, "rb");
	if (!fp)
		return -1;

	*data = malloc(buf.st_size+1);
	*length = fread(*data, 1, buf.st_size, fp);
	fclose(fp);
	if (*length != buf.st_size)
		return -1;
	return 0;
}

static int write_local_file(struct FILE_LOCATION* loc, void* data, ssize_t length) {
	FILE* fp = fopen(loc->filename, "wb");
	if (!fp)
		return -1;
	ssize_t tx = fwrite(data, 1, length, fp);
	fclose(fp);
	if (tx != length)
		return -1;
	return 0;
}

// ---------------------------------------------------------------------
//
// Read the data from the FILE LOCATION
//

extern int read_data(struct FILE_LOCATION* loc, void** data, ssize_t* length) {
	switch(loc->protocol) {
		case PROTO_UNKNOWN:
			assert(0);
		case PROTO_FILE:
			return read_local_file(loc, data, length);
		case PROTO_SSH:
			return ssh_get_file(loc, data, length);
		default:
			return -1;
	};
}

// ---------------------------------------------------------------------
//
// Write the data to the FILE LOCATION
//

extern int write_data(struct FILE_LOCATION* loc, void* data, ssize_t length) {
	switch(loc->protocol) {
		case PROTO_UNKNOWN:
			assert(0);
		case PROTO_FILE:
			return write_local_file(loc, data, length);
		case PROTO_SSH:
			return ssh_put_file(loc, data, length);
	};
	return -1;
}

// ---------------------------------------------------------------------
//
// Destroy a file location structure
//

void destroy_file_location(struct FILE_LOCATION* loc) {
	free_if_defined(loc->title);
	free_if_defined(loc->filename);
	free_if_defined(loc->hostname);
	free_if_defined(loc->username);
	free_if_defined(loc->password);
	free_if_defined(loc->fingerprint);
	free(loc);
}

// ---------------------------------------------------------------------
//
// Create a file location structure
//

struct FILE_LOCATION* create_file_location(void) {
	printf("%s()\n", __FUNCTION__);
	struct FILE_LOCATION* loc = calloc(1,sizeof(struct FILE_LOCATION));
	return loc;
}

// ---------------------------------------------------------------------
//
// Create a file location structure from an url
//

struct FILE_LOCATION* create_file_location_from_uri(const char* url) {
	printf("%s('%s')\n", __FUNCTION__, url);
	struct FILE_LOCATION* loc = calloc(1,sizeof(struct FILE_LOCATION));
	char* tmp = strdup(url);
	loc->title = strdup(url);
	loc->filename = strdup(url);

	/* Extract the protocol */
	char* next = strstr(tmp,"://");
	if (next) {
		*next = 0; next += 3;
		loc->protocol = text_to_proto(tmp);

		/* next points to the potential username
		 * extract the "username:password@hostname" */
		char* cur = next;
		next = strchr(next, '/');
		if (next) {
			*next = 0;
			next++;
		}

		/* Extract the username */
		char* ptr = strchr(cur,'@');
		if (ptr) {
			*ptr = 0;
			loc->username = strdup(cur);
			/* TODO: loc->password */
			cur = ptr+1;
		}
		else if (getenv("USER")) {
			loc->username = strdup(getenv("USER"));
		}

		/* Split the hostname:port */
		ptr = strchr(cur,':');
		if (ptr) {
			*ptr = 0;
			loc->port = atoi(ptr+1);
		}
		loc->hostname = strdup(cur);

		/* The filename is the remaining text */
		free_if_defined(loc->filename);
		loc->filename = strdup(next);
	}
	else if (*url == '/') {
		loc->protocol = PROTO_FILE;
	}
	else {
		loc->protocol = PROTO_UNKNOWN;
	}

	/* cleanup */
	free(tmp);
	return loc;
}

// ---------------------------------------------------------------------
//
// This code is a reminder we need to make more changes
//

void fl_todo(struct FILE_LOCATION* loc) {
	printf("%s(%s)\n", __FUNCTION__, loc->title);
#if 0
	/* FROM: menu_open_profile_file() */
	assert(node);
	//~ tConfigDescription* config = node_to_kvo;
	xmlNodeShow(node);

	// First we have to decrypt the node...
	xmlNode* config_node = node;
	while (xmlIsNodeEncrypted(node)) {
		if (passkey->valid) {
			xmlNode* tmp = xmlNodeDecrypt(node, passkey->config);
			if (tmp) {
				config_node = tmp;
				break;
			}
		}
		if (gtk_request_passphrase() != 0)
			return;
	}

	// Convert the node into an config object
	struct FILE_LOCATION* kvo = node_to_kvo(config_node);
	if (!kvo)
		return;

	//~ if (!dialog_request_kvo(main_window, kvo))
		//~ return;
	g_printf("hostname: %s\n",loc->hostname);
	g_printf("username: %s\n",loc->username);
	g_printf("password: %s\n",loc->password);
	g_printf("filename: %s\n",loc->filename);

	/* FROM: menu_save_profile_file() */
	assert(node);
	xmlNodeShow(node);

	// First we have to decrypt the node...
	xmlNode* config_node = node;
	while (xmlIsNodeEncrypted(node)) {
		if (passkey->valid) {
			xmlNode* tmp = xmlNodeDecrypt(node, passkey->config);
			if (tmp) {
				config_node = tmp;
				break;
			}
		}
		if (gtk_request_passphrase() != 0)
			return;
	}

	// Convert the node into an config object
	struct FILE_LOCATION* kvo = node_to_kvo(config_node);
	if (!kvo)
		return;

	/* FROM: menu_edit_profile() */
	// First we have to decrypt the node...
	xmlNode* config_node = node;
	while (xmlIsNodeEncrypted(node)) {
		if (passkey->valid) {
			xmlNode* tmp = xmlNodeDecrypt(node, passkey->config);
			if (tmp) {
				config_node = tmp;
				break;
			}
		}
		if (gtk_request_passphrase() != 0)
			return;
	}

	// Convert the node into an config object
	struct FILE_LOCATION* kvo = node_to_kvo(config_node);
	if (!kvo)
		return;

#endif
}

// -----------------------------------------------------------
//
// get_configuration
//

static xmlNode* get_config_node(xmlDoc* doc, int idx) {
	printf("%s(%p,%i)\n", __FUNCTION__, doc, idx);
	assert(doc);

	// Get the root
	xmlNode* root = xmlDocGetRootElement(doc);
	assert(root);

	// Walk all children
	xmlNode* node = root->children;
	while(node) {
		trace();
		if (node->type == XML_ELEMENT_NODE) {
			if (idx-- == 0)
				return node;
		}
		node = node->next;
	}
	return NULL;
}

extern struct FILE_LOCATION* get_file_location_by_index(struct CONFIG* config, int idx) {
	printf("%s(%p,%i)\n", __FUNCTION__, config, idx);
	xmlNode* node = get_config_node(config->doc, idx);
	if (!node)
		return NULL;
	xmlNodeShow(node);

	/* Convert the xmlNode into a struct FILE_LOCATION */
	struct FILE_LOCATION* loc = calloc(1,sizeof(struct FILE_LOCATION));
	loc->title = (char*)xmlGetTextContents(node, CONST_BAD_CAST "title");
	char* p = (char*)xmlGetTextContents(node, CONST_BAD_CAST "protocol");
	if (p) {
		loc->protocol = text_to_proto(p);
		free(p);
	}
	loc->hostname = (char*)xmlGetTextContents(node, CONST_BAD_CAST "hostname");
	loc->port = xmlGetIntegerContents(node, CONST_BAD_CAST "port");
	loc->username = (char*)xmlGetTextContents(node, CONST_BAD_CAST "username");
	loc->password = (char*)xmlGetTextContents(node, CONST_BAD_CAST "password");
	loc->filename = (char*)xmlGetTextContents(node, CONST_BAD_CAST "filename");
	loc->fingerprint = (char*)xmlGetBase64Contents(node, CONST_BAD_CAST "fingerprint");
	return loc;
}

// -----------------------------------------------------------
//
// put_configuration
//

static xmlNode* file_location_to_xml_node(struct FILE_LOCATION* kvo) {
	printf("kvo_to_node(%p)\n", kvo);
	assert(kvo);
	assert(kvo->title);
	xmlNode* root = xmlNewNode(NULL, CONST_BAD_CAST "kvo_file");
	assert(root);
	xmlNewTextChild(root, NULL, CONST_BAD_CAST "title", BAD_CAST kvo->title);
	if (kvo->protocol)
		xmlNewTextChild(root, NULL, CONST_BAD_CAST "protocol", BAD_CAST proto_to_text(kvo->protocol));
	if (kvo->hostname)
		xmlNewTextChild(root, NULL, CONST_BAD_CAST "hostname", BAD_CAST kvo->hostname);
	if (kvo->port)
		xmlNewIntegerChild(root, NULL, CONST_BAD_CAST "port", kvo->port);
	if (kvo->username)
		xmlNewTextChild(root, NULL, CONST_BAD_CAST "username", BAD_CAST kvo->username);
	if (kvo->password)
		xmlNewTextChild(root, NULL, CONST_BAD_CAST "password", BAD_CAST kvo->password);
	if (kvo->filename)
		xmlNewTextChild(root, NULL, CONST_BAD_CAST "filename", BAD_CAST kvo->filename);
	if (kvo->fingerprint)
		xmlNewBase64Child(root, NULL, CONST_BAD_CAST "fingerprint", BAD_CAST kvo->fingerprint, 16);
	return root;
}

void store_file_location(struct CONFIG* config, struct FILE_LOCATION* loc) {
	//~ xmlNode* node = xmlNewDocNode(doc, NULL, CONST_BAD_CAST "kvo_file", NULL);
	xmlNode* node = file_location_to_xml_node(loc);
	xmlNode* root = xmlDocGetRootElement(config->doc);
	xmlAddChild(root, node);
}
