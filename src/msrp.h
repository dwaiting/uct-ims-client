#ifndef _MSRP_H
#define _MSRP_H

/*
	Definitions
*/
enum {	/* Available flags for endpoint sessions */
	MSRP_PASSIVE = (1 << 0),	/* Server-mode (receiving connection) */
	MSRP_ACTIVE = (1 << 1),		/* Client-mode (initiating connection) */
	MSRP_OVER_TCP = (1 << 2),	/* MSRP/TCP */
	MSRP_OVER_TLS = (1 << 3),	/* MSRP/TCP/TLS */
};

enum {	/* Read/write rights */
	MSRP_SENDRECV,			/* Can send and receive */
	MSRP_RECVONLY,			/* Can only receive */
	MSRP_SENDONLY,			/* Can only send */
	MSRP_INACTIVE,			/* Can neither send nor receive */
};

enum {	/* Content-types */
	MSRP_TEXT_PLAIN = (1 << 0),	/* text/plain */
	MSRP_TEXT_HTML = (1 << 1),	/* text/html */
};

enum {	/* Reports (Success-Report and Failure-Report) */
	MSRP_SUCCESS_REPORT = (1 << 0),		/* Success-Report = yes */
	MSRP_SUCCESS_REPORT_PARTIAL = (1 << 1),	/* Success-Report = partial */
	MSRP_FAILURE_REPORT = (1 << 2),		/* Failure-Report = yes */
	MSRP_FAILURE_REPORT_PARTIAL = (1 << 3),	/* Failure-Report = partial */
};

enum {	/* Callback methods and events, to notify application */
	MSRP_NONE,
	MSRP_INCOMING_SEND,	/* A received SEND (message attached) */
	MSRP_INCOMING_REPORT,	/* A received REPORT (message attached) */
	MSRP_INCOMING_AUTH,	/* A received AUTH (message attached) */
	MSRP_LOCAL_CONNECT,	/* A local peer disconnected (endpoint attached) */
	MSRP_REMOTE_CONNECT,	/* A remote peer disconnected (endpoint attached) */
	MSRP_LOCAL_DISCONNECT,	/* A local peer disconnected (endpoint attached) */
	MSRP_REMOTE_DISCONNECT,	/* A remote peer disconnected (endpoint attached) */
	MSRP_ENDPOINT_DUMPED,	/* An endpoint connection has been dumped (endpoint attached) */
	MSRP_CONFUSER_JOIN,	/* A conference user just joined (switch attached) */
	MSRP_CONFUSER_LEAVE,	/* A conference user just left (switch attached) */
	MSRP_CONFUSER_SEND,	/* A message from a conference user has been switched (switch attached) */
	MSRP_LOG,		/* Logger for debug purposes */
	MSRP_ERROR,		/* An error happened (explain text attached) */
};


/*
	Structures
*/
typedef struct MsrpEndpoint {
	unsigned long int ID;	/* Local endpoint identifier */
	char *callid;		/* The Session Initiation Protocol (SIP) Call-ID */
	int label;		/* The Session Initiation Protocol (SIP) media label */
	char *context;		/* The reference context for this endpoint */
	int flags;		/* Flags for this endpoint's session */
	void *endpoint;		/* Placeholder for the private endpoint data (DON'T TOUCH!) */
	void *opaque;		/* A pointer to store some application data, if wanted */
} MsrpEndpoint;

typedef struct MsrpRelay {
	unsigned long int ID;	/* Local relay identifier */
	void *relay;		/* Placeholder for the private relay data (DON'T TOUCH!) */
	void *opaque;		/* A pointer to store some application data, if wanted */
} MsrpRelay;

typedef struct MsrpSwitch {
	unsigned long int ID;	/* Local switch identifier */
	void *switcher;		/* Placeholder for the private switch data (DON'T TOUCH!) */
	void *opaque;		/* A pointer to store some application data, if wanted */
} MsrpSwitch;


#if defined __cplusplus
	extern "C" {
#endif

/*
	Headers
*/
/* Initialization and deallocation */
int msrp_init(void(*app_events)(int event, void *info));
void msrp_ep_callback(void(*app_ep_callback)(int event, MsrpEndpoint *endpoint, int content, void *data, int bytes));
void msrp_rl_callback(void(*app_rl_callback)(MsrpRelay *relay));
void msrp_sw_callback(void(*app_sw_callback)(int event, MsrpSwitch *switcher, unsigned short int user, void *data, int bytes));
int msrp_quit(void);


/* Contexts/External Relays */
int msrp_context_new(char *name);
int msrp_context_set_relay(char *context, char *address, unsigned short int port);
int msrp_context_pass_through(char *context, char *through);
char *msrp_context_get_full_path(char *context);
int msrp_context_destroy(char *context);

/* Endpoint Sessions */
MsrpEndpoint *msrp_endpoint_new(void);
MsrpEndpoint *msrp_endpoint_get(unsigned long int ID);
int msrp_endpoint_set_callid(MsrpEndpoint *endpoint, char *callid);
char *msrp_endpoint_get_callid(MsrpEndpoint *endpoint);
int msrp_endpoint_set_label(MsrpEndpoint *endpoint, int label);
int msrp_endpoint_get_label(MsrpEndpoint *endpoint);
int msrp_endpoint_set_context(MsrpEndpoint *endpoint, char *context);
char *msrp_endpoint_get_context(MsrpEndpoint *endpoint);
int msrp_endpoint_authenticate(MsrpEndpoint *endpoint);	/* FIXME */
int msrp_endpoint_set_from(MsrpEndpoint *endpoint, char *address, unsigned short int port, int content, int flags, int rights);
char *msrp_endpoint_get_from_address(MsrpEndpoint *endpoint);
unsigned short int msrp_endpoint_get_from_port(MsrpEndpoint *endpoint);
char *msrp_endpoint_get_from_sessionid(MsrpEndpoint *endpoint);
char *msrp_endpoint_get_from_fullpath(MsrpEndpoint *endpoint);
char *msrp_endpoint_get_from_accepttypes(MsrpEndpoint *endpoint);
int msrp_endpoint_set_to(MsrpEndpoint *endpoint, const char *path, int content, int flags, int rights);
char *msrp_endpoint_get_to_address(MsrpEndpoint *endpoint);
unsigned short int msrp_endpoint_get_to_port(MsrpEndpoint *endpoint);
char *msrp_endpoint_get_to_sessionid(MsrpEndpoint *endpoint);
char *msrp_endpoint_get_to_fullpath(MsrpEndpoint *endpoint);
char *msrp_endpoint_get_to_accepttypes(MsrpEndpoint *endpoint);
int msrp_endpoint_destroy(MsrpEndpoint *endpoint);

/* Relay Sessions */
MsrpRelay *msrp_relay_new(void);
MsrpRelay *msrp_relay_get(unsigned long int ID);
int msrp_relay_set_address(char *address, unsigned short int port);
int msrp_relay_destroy(MsrpRelay *relay);

/* Switch Sessions */
MsrpSwitch *msrp_switch_new(void);
int msrp_switch_set_from(MsrpSwitch *switcher, char *address, unsigned short int port);
char *msrp_switch_get_address(MsrpSwitch *switcher);
unsigned short int msrp_switch_get_port(MsrpSwitch *switcher);
char *msrp_switch_get_sessionid(MsrpSwitch *switcher);
char *msrp_switch_get_fullpath(MsrpSwitch *switcher);
MsrpSwitch *msrp_switch_get(unsigned long int ID);
unsigned short int msrp_switch_add_user(MsrpSwitch *switcher, char *display, const char *path, int content, int flags, int rights);
int msrp_switch_update_user_rights(MsrpSwitch *switcher, unsigned short int ID, int rights);
int msrp_switch_remove_user(MsrpSwitch *switcher, unsigned short int ID);
int msrp_switch_message(MsrpSwitch *switcher, unsigned short int ID, char *text);
int msrp_switch_announcement(MsrpSwitch *switcher, unsigned short int ID, char *text);
int msrp_switch_destroy(MsrpSwitch *switcher);


/* Message wrappers (for endpoint sessions only) */
int msrp_send_text(MsrpEndpoint *endpoint, char *text, int reports);
int msrp_report_text(MsrpEndpoint *endpoint, char *text, int status);


#if defined __cplusplus
	}
#endif


#endif
