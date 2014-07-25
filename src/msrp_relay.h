#ifndef _MSRP_RELAY_H
#define _MSRP_RELAY_H

#include "msrp.h"
#include "msrp_session.h"


/*
	Structures
*/
typedef struct msrp_relay {
	char *address;			/* Address this relay listens on */
	unsigned short int port;	/* Port this relay listens on */
	int transport;			/* MSRP/TCP or MSRP/TCP/TLS */
	int fd;				/* File descriptor of the session */
	char *path;			/* Single path URL for this relay */
	/* TODO add authentication stuff */
	struct msrp_relay *next;	/* Next relay in the list */
} msrp_relay;


/*
	Headers
*/

#endif
