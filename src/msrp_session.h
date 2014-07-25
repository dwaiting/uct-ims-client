#ifndef _MSRP_SESSION_H
#define _MSRP_SESSION_H

#include "msrp.h"
#include "msrp_utils.h"


/*
	Definitions
*/
enum {	/* Possible type flags for sessions */
	MSRP_ENDPOINT,		/* An MSRP user agent (i.e. client only) */
	MSRP_RELAY,		/* An MSRP relay */
	MSRP_SWITCH,		/* An MSRP switcher (e.g. a conference room) */
};

/* Pool of messages each private session will take care of simultaneously */
#define MSRP_MSG_BUFFER	20


/*
	Structures
*/
typedef struct msrp_context {
	char *name;			/* Friendly reference name for this context */
	struct msrp_context *through;	/* Next hop for this context */
	struct msrp_peer *relay;	/* Reference external relay for this context */
	int authenticated;		/* Whether this context is already authenticated */
	struct msrp_context *next;	/* Next in the list */
} msrp_context;

typedef struct msrp_session {
	unsigned long int ID;		/* Unique local identifier for this session */
	char *callid;			/* The Session Initiation Protocol (SIP) Call-ID */
	msrp_context *context;		/* The context (relays and so on) for this session */
	int type;			/* The public reference session type (endpoint/relay/switch) */
	void *session;			/* The public reference session pointer (endpoint/relay/switch) */
	struct msrp_peer *from;		/* The local peer ("From") */
	struct msrp_peer *to;		/* The remote peer ("To") */
	int fd;				/* File descriptor of the server */
	/* List of incoming and outgoing messages this session is handling FIXME */
	struct msrp_message *in_msg[MSRP_MSG_BUFFER];
	struct msrp_message *out_msg[MSRP_MSG_BUFFER];
	struct msrp_session *next;	/* Sessions are handled as a list */
} msrp_session;

typedef struct msrp_peer {
	msrp_session *session;		/* Session this peer belongs to */
	char *path;			/* Full MSRP path */
	char *address;			/* Address this peer listens on */
	unsigned short int port;	/* Port this peer listens on */
	struct sockaddr_in *sockaddr;	/* Address structure */
	int fd;				/* File descriptor (server/client) */
	char *sessionid;		/* Session identifier (if in a session) */
	int flags;			/* Flags for this peer */
	int rights;			/* Read/write access rights for this peer */
	int content;			/* Supported Content-Types */
	void *opaque;			/* An opaque pointer to store something */
} msrp_peer;


/*
	Headers
*/
/* Contexts management */
char *msrp_context_build_path(msrp_context *context);
int msrp_context_setup_relay(msrp_context *context, char *address, unsigned short int port);
int msrp_context_free(msrp_context *context);

/* Sessions management */
msrp_session *msrp_session_new(unsigned long int ID);
int msrp_session_set_from(msrp_session *session, msrp_peer *from);
int msrp_session_set_to(msrp_session *session, msrp_peer *to);
char *msrp_session_get_address(msrp_session *session, char *whose);
unsigned short int msrp_session_get_port(msrp_session *session, char *whose);
char *msrp_session_get_sessionid(msrp_session *session, char *whose);
char *msrp_session_get_fullpath(msrp_session *session, char *whose);
char *msrp_session_get_accepttypes(msrp_session *session, char *whose);
msrp_session *msrp_session_get(int fd);
int msrp_session_connect(msrp_session *session);
int msrp_session_destroy(msrp_session *session);

/* Peers management */
msrp_peer *msrp_peer_new(char *sessionid);
int msrp_peer_set_address(msrp_peer *peer, char *address, unsigned short int port);
int msrp_peer_set_options(msrp_peer *peer, int content, int flags);
int msrp_peer_set_rights(msrp_peer *peer, int rights);
char *msrp_peer_get_path(msrp_peer *peer);
int msrp_peer_bind(msrp_peer *peer);
int msrp_peer_listen(msrp_peer *peer);
int msrp_peer_connect(msrp_peer *peer, msrp_peer* dst);
int msrp_peer_destroy(msrp_peer *peer);


#endif
