#include <mxml.h>

// -----------------------------------------------------------
//
// XML format callback
//

const char* whitespace_cb(mxml_node_t* node,int where) {
	if (!node->parent)
		return NULL;
	static int close_child=0;
	static int open_count=0;
	static char retval[64];
	retval[0]=0;
	if ((where == MXML_WS_BEFORE_CLOSE) && (node->child) && (close_child)) {
		strcat(retval,"\n");
		int i;
		for (i=0;i<open_count-1;i++)
			strcat(retval,"\t");
	}
	if (where == MXML_WS_BEFORE_OPEN) {
		strcat(retval,"\n");
		int i;
		for (i=0;i<open_count;i++)
			strcat(retval,"\t");
	}
	// Determine next state
	close_child=0;
	if ((where == MXML_WS_BEFORE_OPEN) && (node->child))
		open_count++;
	if ((where == MXML_WS_AFTER_CLOSE) && (node->child)) {
		open_count--;
		close_child=1;
	}
	return retval;
}
