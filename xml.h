#ifndef _xml_h_
#define _xml_h_

xmlNode* xmlFindNode(xmlNode* root_node, const xmlChar* name, int depth);
xmlChar* xmlGetContents(xmlNode* root_node, const xmlChar* name);
int xmlGetContentsInteger(xmlNode* root_node, const xmlChar* name);

xmlAttr* xmlNewPropInteger(xmlNodePtr node, const xmlChar* name, const int value);
xmlAttr* xmlNewPropBase64(xmlNodePtr node, const xmlChar* name, const void* data, const int size);

#endif
