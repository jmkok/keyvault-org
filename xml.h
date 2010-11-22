#ifndef _xml_h_
#define _xml_h_

xmlNode* xmlFindNode(xmlNode* root_node, const xmlChar* name, int depth);
xmlChar* xmlGetContents(xmlNode* root_node, const xmlChar* name);
int xmlGetContentsInteger(xmlNode* root_node, const xmlChar* name);
xmlChar* xmlGetContentsEncrypted(xmlNode* root_node, const xmlChar* name, const unsigned char passphrase_key[32]);

xmlNode* xmlNewChildEncrypted(xmlNodePtr parent, xmlNs* ns, const xmlChar* name, const xmlChar* value, const unsigned char passphrase_key[32]);
xmlNode* xmlNewChildInteger(xmlNodePtr parent, xmlNs* ns, const xmlChar* name, const int value);
xmlNode* xmlNewChildBase64(xmlNodePtr parent, xmlNs* ns, const xmlChar* name, const void* data, const int size);

xmlAttr* xmlNewPropInteger(xmlNodePtr node, const xmlChar* name, const int value);
xmlAttr* xmlNewPropBase64(xmlNodePtr node, const xmlChar* name, const void* data, const int size);

void xmlNodeEncrypt(xmlNode* node, const unsigned char passphrase_key[32], const char* protocols);
void xmlNodeDecrypt(xmlNode* node, const unsigned char passphrase_key[32], const char* protocols);

#endif
