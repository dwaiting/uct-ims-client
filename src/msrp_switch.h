#ifndef _MSRP_SWITCH_H
#define _MSRP_SWITCH_H

#include "msrp.h"
#include "msrp_utils.h"


/*
	Structures
*/
typedef struct msrp_conference {
	unsigned long int ID;		/* Unique identifier for this room */
	MsrpSwitch *sw;			/* The public switch reference */
	struct msrp_peer *server;	/* Session peer for the server */
	struct msrp_conf_user *users;	/* List of peers in this room */
	unsigned short int userID;	/* Counter to assign user IDs */
	pthread_mutex_t lock;		/* Lock to access the peers list */
	int fd[100];			/* List of pending connections to identify */
	struct msrp_conference *next;	/* Next switch in the list */
} msrp_conference;

typedef struct msrp_conf_user {
	msrp_conference *switcher;	/* The MSRP switch this user is in */
	unsigned short int ID;		/* Unique identifier for the user in this room */
	char *display;			/* Nickname (will be used in switching) */
	struct msrp_session *session;	/* Session with the conference */
	struct msrp_peer *peer;		/* This peer in the session with the switch */
	struct msrp_conf_user *next;	/* Next user in this switch */
} msrp_conf_user;


/*
	Headers
*/
msrp_conference *msrp_conference_new(MsrpSwitch *sw, unsigned long int ID);
msrp_conference *msrp_conference_find(MsrpSwitch *sw);
msrp_conference *msrp_conference_get(int fd);
int msrp_conference_set_from(msrp_conference *switcher, struct msrp_peer *from);
unsigned short int msrp_conference_add_user(msrp_conference *switcher, char *display, struct msrp_peer *peer);
int msrp_conference_set_user_rights(msrp_conference *switcher, unsigned short int ID, int rights);
int msrp_conference_match_user(msrp_conference *switcher, int fd, struct sockaddr_in *addr);
int msrp_conference_remove_user(msrp_conference *switcher, unsigned short int ID);
int msrp_conference_forward_message(msrp_conference *switcher, int fd, struct msrp_session *session, char *data, int bytes);
int msrp_conference_message(msrp_conference *switcher, unsigned short int ID, char *text);
int msrp_conference_announcement(msrp_conference *switcher, unsigned short int ID, char *text);
int msrp_conference_destroy(msrp_conference *switcher);

#endif
