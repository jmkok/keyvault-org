#ifndef _define_h_
#define _define_h_

/* Always love these defines... */
#define trace() printf("<%s:%u>\n",__FILE__,__LINE__)
#define todo() printf("<%s:%u> TODO\n",__FILE__,__LINE__)
#define die(args...) do {printf("<%s:%u> ERROR: ",__FILE__,__LINE__);printf(args);printf("\n");exit(1);} while(0)
#define debugf(args...) printf(args)

/* UNUSED items are not allowed to be used
 * This keeps the code rather strict
 */
#define _UNUSED_ __attribute__((unused deprecated))
#define _DEPRECATED_ __attribute__((deprecated))

/* The encryption variables
 * The keys are built from the same passphrase, but with different salts and rounds
 * The salts and rounds for the keys (not secret, they just need to be different)
 */
#define KEYVAULT_DATA   "5NewDdGpLQ0W-keyvault-data"
#define KEYVAULT_CONFIG "n41JFWQAdmcf-keyvault-config"
#define KEYVAULT_LOGIN  "S3ftaw7kgXlc-keyvault-login"
#define KEYVAULT_DATA_ROUNDS   (4*1024)
#define KEYVAULT_CONFIG_ROUNDS (5*1024)
#define KEYVAULT_LOGIN_ROUNDS  (6*1024)

/* The file we use to inidicate that the user has read the warning about
 * the beta stage of the code
 */
#define FILE_CONFIG ".config/keyvault/config.xml"
#define FILE_WARN_ONCE ".config/keyvault/warning.once"

#endif
