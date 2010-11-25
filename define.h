#ifndef _define_h_
#define _define_h_

#define trace() printf("<%s:%u>\n",__FILE__,__LINE__)
#define todo() printf("<%s:%u> TODO\n",__FILE__,__LINE__)
#define die(args...) do {printf("<%s:%u> ERROR: ",__FILE__,__LINE__);printf(args);printf("\n");exit(1);} while(0)
#define debugf(args...) printf(args)
#define UNUSED(x) UNUSED_ ## x __attribute__((unused))

#endif
