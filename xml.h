xmlDoc* xml_doc_encrypt(xmlDoc* doc, const gchar* passphrase);
xmlDoc* xml_doc_decrypt(xmlDoc* doc, const gchar* passphrase);

xmlNode* xmlFindNode(xmlNode* root_node, const xmlChar* name, int depth);
xmlChar* xmlGetContents(xmlNode* root_node, const xmlChar* name);
int xmlGetContentsInteger(xmlNode* root_node, const xmlChar* name);

xmlAttr* xmlNewPropInteger(xmlNodePtr node, const xmlChar* name, const int value);
xmlAttr* xmlNewPropBase64(xmlNodePtr node, const xmlChar* name, const void* data, const int size);
