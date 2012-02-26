#if ! defined ( NET_H_ )
#define NET_H_

#include <stdlib.h>

#define FAIL(x) if(x == -1) {\
	perror(#x);exit(-1);}
	
#define ERROR -1

/**
 * Initialisation of the Network
 **/
void initNetwork();

/**
 * Desactivation of Network and free related memory
 **/
void closeNetwork();

/**
 * Send Message toward server
 **/
int networkSendTrame(const char * msg_ptr, size_t msg_len);

#endif /*NET*/
