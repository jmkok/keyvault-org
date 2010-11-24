#ifndef _xml_h_
#define _xml_h_

xmlNode* xmlFindNode(xmlNode* root_node, const xmlChar* name, int depth);

xmlChar* xmlGetTextContents(xmlNode* root_node, const xmlChar* name);
int xmlGetIntegerContents(xmlNode* root_node, const xmlChar* name);
xmlChar* xmlGetBase64Contents(xmlNode* root_node, const xmlChar* name);

xmlNode* xmlNewIntegerChild(xmlNodePtr parent, xmlNs* ns, const xmlChar* name, const int value);
xmlNode* xmlNewBase64Child(xmlNodePtr parent, xmlNs* ns, const xmlChar* name, const void* data, const int size);

xmlAttr* xmlNewIntegerProp(xmlNodePtr node, const xmlChar* name, const int value);
xmlAttr* xmlNewBase64Prop(xmlNodePtr node, const xmlChar* name, const void* data, const int size);

xmlNode* xmlNodeEncrypt(xmlNode* node, const unsigned char passphrase_key[32], const char* protocols);
xmlNode* xmlNodeEncryptAndReplace(xmlNode* node, const unsigned char passphrase_key[32], const char* protocols);
xmlNode* xmlNodeDecrypt(xmlNode* node, const unsigned char passphrase_key[32]);
xmlNode* xmlNodeDecryptAndReplace(xmlNode* node, const unsigned char passphrase_key[32]);
int xmlIsNodeEncrypted(xmlNode* root);

#endif
