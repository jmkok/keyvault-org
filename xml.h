#ifndef _xml_h_
#define _xml_h_

xmlNode* xmlFindNode(xmlNode* root_node, const xmlChar* name, int depth);

xmlChar* xmlGetContents(xmlNode* root_node, const xmlChar* name);
int xmlGetContentsInteger(xmlNode* root_node, const xmlChar* name);
xmlChar* xmlGetContentsBase64(xmlNode* root_node, const xmlChar* name);

xmlNode* xmlNewChildInteger(xmlNodePtr parent, xmlNs* ns, const xmlChar* name, const int value);
xmlNode* xmlNewChildBase64(xmlNodePtr parent, xmlNs* ns, const xmlChar* name, const void* data, const int size);

xmlAttr* xmlNewPropInteger(xmlNodePtr node, const xmlChar* name, const int value);
xmlAttr* xmlNewPropBase64(xmlNodePtr node, const xmlChar* name, const void* data, const int size);

xmlNode* xmlNodeEncrypt(xmlNode* node, const unsigned char passphrase_key[32], const char* protocols);
xmlNode* xmlNodeEncryptAndReplace(xmlNode* node, const unsigned char passphrase_key[32], const char* protocols);
xmlNode* xmlNodeDecrypt(xmlNode* node, const unsigned char passphrase_key[32]);
xmlNode* xmlNodeDecryptAndReplace(xmlNode* node, const unsigned char passphrase_key[32]);
int xmlIsNodeEncrypted(xmlNode* root);

#endif
