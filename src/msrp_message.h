#ifndef _MSRP_MESSAGE_H
#define _MSRP_MESSAGE_H

#include "msrp.h"
#include "msrp_session.h"

/*
	Definitions
*/
enum {	/* Supported methods */
	MSRP_SEND,			/* SEND, to send text or MIME data */
	MSRP_REPORT,			/* REPORT, to report successes and failures */
	MSRP_AUTH			/* AUTH, to authenticate at relays */
};

enum {
	MSRP_LAST_CHUNK,	/* Add a '$' to the end line */
	MSRP_MID_CHUNK,		/* Add a '+' to the end line */
	MSRP_INTERRUPT_CHUNK,	/* Add a '#' to the end line */
};

static const struct msrp_content_types {	/* Content-types and descriptions FIXME */
	int content;
	char *desc;
} msrp_content_type[] = {
	{ MSRP_TEXT_PLAIN, "text/plain" }, 		/* text/plain */
	{ MSRP_TEXT_HTML, "text/html" },		/* text/html */
	{ -1, NULL }
};

static const struct msrp_request_codes {	/* Status codes and descriptions */
	int status;
	char *desc;
} msrp_status_code[] = {
	{ 200, "OK" }, 				/* Successful transaction */
	{ 400, "Bad Request" },			/* Request was unintelligible */
	{ 403, "Forbidden" },			/* Attempted action is not allowed */
	{ 408, "Request Timeout" },		/* Downstream transaction did not complete in the allowed time */
	{ 413, "Stop Sending" },		/* Receiver wishes the sender to stop sending the message */
	{ 415, "Unsupported Media Type" },	/* Request contained a media type that is not understood by the receiver */
	{ 423, "Out Of Bounds" },		/* One of the requested parameters is out of bounds */
	{ 481, "Session Does Not Exist" },	/* Indicated session does not exist */
	{ 501, "Not Implemented" },		/* Recipient does not understand the request method */
	{ 506, "Session Already Bound" },	/* Request arrived on a session which is already bound to another network connection */
	{ -1, NULL }
};


/*
	Structures
*/
typedef struct msrp_message {
	struct msrp_session *session;	/* MSRP session this message belongs to */
	int method;			/* MSRP method (SEND/REPORT/AUTH) */
	char *messageid;		/* Message-ID */
	int content;			/* Payload content type */
	char *data;			/* Total payload data (all chunks) */
	int bytes;			/* Total payload lenght */
	int status;			/* Delivery Status (for REPORT) */
	int reports;			/* Flag for Success/Failure reports (yes/no/partial) */
} msrp_message;

typedef struct msrp_chunk {
	msrp_message *message;		/* MSRP message this chunk belongs to */
	char *transactionid;		/* The transaction identifier */
	int method;			/* MSRP request (SEND/REPORT) */
	char *messageid;		/* Message-ID */
	int start, end, total;		/* Byte-Range fields (start-end/total) */
	int content;			/* Payload content type */
	char *data;			/* Payload data (text, binary, etc) */
	int bytes;			/* Payload lenght */
} msrp_chunk;


/*
	Headers
*/
/* Message/chunk management */
msrp_message *msrp_message_new(char *messageid, int content, int bytes);
int msrp_message_setup(msrp_message *message, struct msrp_session *session, int method, int reports, int status);
int msrp_message_fill(msrp_message *message, char *data, int start, int end);
msrp_message *msrp_message_get(struct msrp_session *session, char *messageid);
msrp_chunk *msrp_chunk_new(msrp_message *message, char *transactionid, int start, int end);

/* Message/chunk sending helpers */
int msrp_queue_text(int method, struct msrp_session *session, char *text, int reports, int status);
int msrp_send_message(msrp_message *message);
int msrp_send_chunk(msrp_message *message, int start, int end);
int msrp_send_reply(msrp_message *message, char *transactionid, int start, int end, int code);

/* Line constructors */
int msrp_add_line(char *buffer, char *format, ...);
int msrp_add_request_line(char *buffer, char *transactionid, int method);
int msrp_add_topath_line(char *buffer, char *topath);
int msrp_add_frompath_line(char *buffer, char *frompath);
int msrp_add_messageid_line(char *buffer, char *messageid);
int msrp_add_byterange_line(char *buffer, int start, int end, int total);
int msrp_add_reports_line(char *buffer, int reports);
int msrp_add_content_line(char *buffer, int content);
int msrp_add_status_line(char *buffer, int status);
int msrp_add_empty_line(char *buffer);
int msrp_add_body(char *buffer, char *text, int start, int end);
int msrp_add_end_line(char *buffer, char *transactionid, int trailer);
/* TODO */
int msrp_add_wwwauth_line(void);
int msrp_add_auth_line(void);
int msrp_add_usepath_line(void);
int msrp_add_authinfo_line(void);

/* Parsing */
int msrp_buffer_parse(int fd, struct msrp_session *session, char *buffer, int bytes);
char *msrp_buffer_extract_sessionid(char *buffer, int bytes);


#endif
