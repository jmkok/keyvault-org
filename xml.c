#include <libxml/parser.h>

// -------------------------------------------------------------------------------
//
// Locate a specific node
//

xmlNode* xmlFindNode(xmlNode* root_node, const xmlChar* name, int depth) {
	//~ debugf("soapFindNode('%s',%u) in '%s' \n", name, depth, root_node->name);
	xmlNode* node = root_node->children;
	while (node) {
		//~ debugf("found> %s\n", node->name);fflush(stdout);
		if (node->type == XML_ELEMENT_NODE) {
			if (xmlStrEqual(node->name, name)) {
				return node;
			}
			if (depth) {
				xmlNode* sub_node = xmlFindNode(node, name, depth-1);
				if (sub_node)
					return sub_node;
			}
		}
		node = node->next;
	}
	return NULL;
}

// -------------------------------------------------------------------------------
//
// helper
//

static int xmlCharFreeAndReturnInteger(xmlChar* text) {
	if (text) {
		int retval = atol((char*)text);
		xmlFree(text);
		return retval;
	}
	return 0;
}

// -------------------------------------------------------------------------------
//
// Extracte and duplicate data from a node
//

xmlChar* xmlGetContents(xmlNode* root_node, const xmlChar* name) {
	xmlNode* node = xmlFindNode(root_node, BAD_CAST name, 0);
	if (node)
		return xmlNodeGetContent(node);
	return NULL;
}

int xmlGetContentsInteger(xmlNode* root_node, const xmlChar* name) {
	xmlNode* node = xmlFindNode(root_node, BAD_CAST name, 0);
	if (node) {
	xmlChar* text = xmlNodeGetContent(node);
		return xmlCharFreeAndReturnInteger(text);
	}
	return 0;
}

// OLD

/*
xmlNode* xmlFindNode(xmlNode* root_node, xmlChar* name, int depth) {
	//~ printf("soapFindNode('%s',%u) in '%s' \n", name, depth, root_node->name);
	xmlNode* node = root_node->children;
	while (node) {
		//~ printf("found> %s\n", node->name);fflush(stdout);
		if (node->type == XML_ELEMENT_NODE) {
			if (xmlStrEqual(node->name, name)) {
				return node;
			}
			if (depth) {
				xmlNode* sub_node = xmlFindNode(node, name, depth-1);
				if (sub_node)
					return sub_node;
			}
		}

		node = node->next;
	}
	return NULL;
}

char* xmlNodeContentsText(xmlNode* root_node, xmlChar* name) {
	xmlNode* node = xmlFindNode(root_node, BAD_CAST name, 0);
	if (node && node->children && node->children->content)
		return strdup((char*)node->children->content);
	return NULL;
}

int xmlNodeContentsInteger(xmlNode* root_node, xmlChar* name) {
	xmlNode* node = xmlFindNode(root_node, BAD_CAST name, 0);
	if (node && node->children && node->children->content)
		return atol((char*)node->children->content);
	return 0;
}
*/
