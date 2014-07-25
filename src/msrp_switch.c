#include "msrp_switch.h"

/*
	Create a new conference switch
*/
msrp_conference *msrp_conference_new(MsrpSwitch *sw, unsigned long int ID)
{
	if(!sw || !ID)
		return NULL;

	msrp_conference *conf = calloc(1, sizeof(*conf));
	if(!conf)
		return NULL;

	conf->ID = ID;
	conf->sw = sw;
	conf->server = NULL;
	conf->next = NULL;

	int i = 0;
	for(i = 0; i < 100; i++)
			conf->fd[i] = 0;

	/* Create the list of users */
	MSRP_LIST_SETUP(conf->users, conf->lock);
	conf->userID = 0;

	return conf;
}

/*
	Get the conference object associated to the specified public switch instance
*/
msrp_conference *msrp_conference_find(MsrpSwitch *sw)
{
	if(!sw)
		return NULL;

	msrp_conference *conf = (msrp_conference *)sw->switcher;
	if(!conf) {	/* The opaque pointer is invalid, try looking for the ID */
		MSRP_LIST_CHECK(switches, NULL);
		conf = NULL;
		MSRP_LIST_CROSS(switches, switches_lock, conf)
			if(conf->ID == sw->ID)
				break;
		MSRP_LIST_STEP(switches, switches_lock, conf);
		if(!conf)
			return NULL;
		sw->switcher = conf;
	}

	return conf;
}

/*
	Get conference from the associated file descriptor(s)
*/
msrp_conference *msrp_conference_get(int fd)
{
	if(fd < 1)
		return NULL;

	msrp_conference *conf = NULL;
	msrp_conf_user *user = NULL;
	msrp_peer *server = NULL, *client = NULL;
	MSRP_LIST_CHECK(switches, NULL);
	MSRP_LIST_CROSS(switches, switches_lock, conf)
		/* First check the server (accept) */
		server = conf->server;
		if(server) {
			if(server->fd == fd)
				break;
		}
		if(!conf->users)
			continue;
		/* Then check the users (clients) */
		MSRP_LIST_CROSS(conf->users, conf->lock, user)
		client = user->peer;
		if(client) {
			if(client->fd == fd)
				break;
		}
		MSRP_LIST_STEP(conf->users, conf->lock, user);
	MSRP_LIST_STEP(switches, switches_lock, conf);

	return conf;
}

/*
	Set the local conference endpoint associated with the conference
*/
int msrp_conference_set_from(msrp_conference *switcher, msrp_peer *from)
{
	if(!switcher || !from)
		return -1;

	switcher->server = from;
	return msrp_peer_listen(switcher->server);
}

/*
	Add a user to the conference switch
*/
unsigned short int msrp_conference_add_user(msrp_conference *conf, char *display, msrp_peer *peer)
{
	if(!conf || !peer)
		return 0;

	msrp_conf_user *user = calloc(1, sizeof(*user));
	if(!user)
		return 0;

	conf->userID++;
	user->ID = conf->userID;
	if(display) {
		user->display = calloc(strlen(display)+1, sizeof(char));
		if(!user->display) {
			free(user);
			return 0;
		}
		strcpy(user->display, display);
	} else {
		user->display = calloc(11, sizeof(char));
		if(!user->display) {
			free(user);
			return -1;
		}
		sprintf(user->display, "user-%hu", user->ID);
	}

	/* Create a new session and add it to the list */
	user->peer = peer;
	msrp_session *session = msrp_session_new(msrp_new_identifier());
	if(!session) {
		local_events(MSRP_ERROR, "Couldn't create session for user %hu in conference %lu", user->ID, conf->ID);
		free(user);
		return -1;
	}
	session->type = MSRP_SWITCH;
	session->session = conf;
	msrp_session_set_from(session, conf->server);
	msrp_session_set_to(session, user->peer);
	user->session = session;
	peer->opaque = user;
	MSRP_LIST_ADD(sessions, sessions_lock, session, -1);

	MSRP_LIST_ADD(conf->users, conf->lock, user, -1);

	return user->ID;
}

/*
	Update the read/write rights for this user in the conference
*/
int msrp_conference_set_user_rights(msrp_conference *switcher, unsigned short int ID, int rights)
{
	if(!switcher || !ID)
		return -1;

	msrp_conf_user *user = NULL;
	MSRP_LIST_CHECK(switcher->users, -1);
	MSRP_LIST_CROSS(switcher->users, switcher->lock, user)
		if(user->ID == ID)
			break;
	MSRP_LIST_STEP(switcher->users, switcher->lock, user);
	if(!user)
		return -1;

	return msrp_peer_set_rights(user->peer, rights);
}

/*
	Get the user associated with the specified address
*/
int msrp_conference_match_user(msrp_conference *switcher, int fd, struct sockaddr_in *addr)
{
	if(!switcher || (fd < 1) || !addr)
		return -1;

	char *client_ip = inet_ntoa(addr->sin_addr);
	unsigned short int client_port = ntohs(addr->sin_port);
	if(!client_ip || !client_port) {
		local_events(MSRP_ERROR, "Invalid address %s:%hu, could not match any user", client_ip, client_port);
		return -1;
	}

	/* Check which user has a matching address for this file descriptor */
	msrp_conf_user *user = NULL;
	msrp_peer *peer = NULL;
	char *ip = NULL;
	MSRP_LIST_CHECK(switcher->users, -1);
	MSRP_LIST_CROSS(switcher->users, switcher->lock, user)
		peer = user->peer;
		if(peer) {
			if(!peer->sockaddr) {	/* Build the address first */
				peer->sockaddr = calloc(1, sizeof(peer->sockaddr));
				peer->sockaddr->sin_family = AF_INET;
				peer->sockaddr->sin_port = htons(peer->port);
				if(inet_aton(peer->address, &(peer->sockaddr->sin_addr)) == 0) {	/* Not a numeric IP... */
					struct hostent *host = gethostbyname(peer->address);	/* ...resolve name */
					if(!host)
						continue;
					peer->sockaddr->sin_addr = *(struct in_addr *)host->h_addr_list;
				}
			}
			ip = inet_ntoa(peer->sockaddr->sin_addr);
			if(!ip)		/* No IP */
				continue;
			/* Compare the addresses */
			local_events(MSRP_LOG, "Comparing %s:%hu with %s:%hu (user %hu)...",
				client_ip, client_port, ip, peer->port, user->ID);
			if(!strcasecmp(client_ip, ip) && (client_port == peer->port)) {
				local_events(MSRP_LOG, "\tUser %hu in conference %lu matches the address for file descriptor %d", user->ID, switcher->ID, fd);
				peer->fd = fd;
				break;
			}
		}
	MSRP_LIST_STEP(switcher->users, switcher->lock, user);
	if(!user) {
		local_events(MSRP_ERROR, "\tNo user in conference %lu matches the address for file descriptor %d", switcher->ID, fd);
		return -1;
	}

	local_sw_callback(MSRP_CONFUSER_JOIN, switcher->sw, user->ID, user->display, 0);

	return 0;
}

/*
	Remove a user from the conference
*/
int msrp_conference_remove_user(msrp_conference *conf, unsigned short int ID)
{
	if(!conf || !ID)
		return -1;

	msrp_conf_user *user = NULL;
	MSRP_LIST_CHECK(conf->users, -1);
	MSRP_LIST_CROSS(conf->users, conf->lock, user)
		if(user->ID == ID)
			break;
	MSRP_LIST_STEP(conf->users, conf->lock, user);
	if(!user) {
		local_events(MSRP_LOG, "Couldn't find (and remove) user %hu in conference %lu", ID, conf->ID);
		return -1;
	}

	MSRP_LIST_REMOVE(conf->users, conf->lock, user);

	/* FIXME destroy session for this user */
	if(user->peer)
		local_sw_callback(MSRP_CONFUSER_LEAVE, conf->sw, user->ID, user->display, 0);
	msrp_peer_destroy(user->peer);
	msrp_session_destroy(user->session);
	if(user->display)
		free(user->display);
	free(user);
	local_events(MSRP_LOG, "User %hu removed from conference %lu", ID, conf->ID);

	return 0;
}

/*
	Forward a just received message to all the other users
*/
int msrp_conference_forward_message(msrp_conference *switcher, int fd, struct msrp_session *session, char *data, int bytes)
{
	if(!switcher || !session || !data || (fd < 0))
		return -1;

	msrp_peer *senderpeer = session->to;
	if(!senderpeer)
		return -1;
	msrp_conf_user *sender = (msrp_conf_user *)senderpeer->opaque;
	if(!sender)
		return -1;
	local_sw_callback(MSRP_CONFUSER_SEND, switcher->sw, sender->ID, data, bytes);

	char *new_data = NULL;
	msrp_conf_user *user = NULL;
	msrp_peer *peer = NULL;
	MSRP_LIST_CHECK(switcher->users, 0);
	MSRP_LIST_CROSS(switcher->users, switcher->lock, user)
		peer = user->peer;
		if(peer) {
			if(peer->fd == fd) {	/* This is the sender, callback the application */
				if((peer != senderpeer) || (user != sender))
					local_events(MSRP_ERROR, "Sender mismatch...?");
			} else {	/* Not the sender, forward */
				if(!new_data) {		/* Add the sender's display before the text */
					new_data = calloc(strlen(data) + strlen(sender->display) + 4, sizeof(char));
					sprintf(new_data, "@%s@ %s", sender->display, data);
				}
				local_events(MSRP_LOG, "Forwarding text to user %hu in conference %lu...", user->ID, switcher->ID);
				if(msrp_queue_text(MSRP_SEND, user->session, new_data, 0, 0) < 0)
					local_events(MSRP_ERROR, "\tCouldn't forward text to user %hu in conference %lu", user->ID, switcher->ID);
				else
					local_events(MSRP_LOG, "\tReceived text forwarded to user %hu in conference %lu", user->ID, switcher->ID);
			}
		}
	MSRP_LIST_STEP(switcher->users, switcher->lock, user);

	return 0;
}

/*
	Destroy an existing conference switch
*/
int msrp_conference_destroy(msrp_conference *conf)
{
	if(!conf)
		return -1;

	if(conf->users) {
		local_events(MSRP_LOG, "Removing all users from conference %lu", conf->ID);
		/* Close all sessions */
		MSRP_LIST_CHECK(conf->users, -1);
		while(conf->users)
			msrp_conference_remove_user(conf, conf->users->ID);
		/* Free the list of users */
		MSRP_LIST_FREE(conf->users, conf->lock);
	}

	/* Remove conference from the list */
	MSRP_LIST_REMOVE(switches, switches_lock, conf);

	/* Destroy conference session */
	local_events(MSRP_LOG, "Destroying peer server object for conference %lu", conf->ID);
	msrp_peer_destroy(conf->server);

	free(conf);

	return 0;
}

/*
	Send a SEND message to a user (all users if ID=0) in the conference
*/
int msrp_conference_message(msrp_conference *switcher, unsigned short int ID, char *text)
{
	if(!switcher || !text)
		return -1;

	msrp_conf_user *user = NULL;
	MSRP_LIST_CHECK(switcher->users, 0);
	MSRP_LIST_CROSS(switcher->users, switcher->lock, user)
		if(!ID || (ID == user->ID)) {
			if(msrp_queue_text(MSRP_SEND, user->session, text, 0, 200) < 0)
				local_events(MSRP_ERROR, "\tCouldn't send text to user %hu in conference %lu", user->ID, switcher->ID);
			else
				local_events(MSRP_LOG, "\tSent text to user %hu in conference %lu", user->ID, switcher->ID);
		}
	MSRP_LIST_STEP(switcher->users, switcher->lock, user)

	return 0;
}

/*
	Send a REPORT announcement to a user (all users if ID=0) in the conference
*/
int msrp_conference_announcement(msrp_conference *switcher, unsigned short int ID, char *text)
{
	if(!switcher || !text)
		return -1;

	msrp_conf_user *user = NULL;
	MSRP_LIST_CHECK(switcher->users, 0);
	MSRP_LIST_CROSS(switcher->users, switcher->lock, user)
		if(!ID || (ID == user->ID)) {
			if(msrp_queue_text(MSRP_REPORT, user->session, text, 0, 200) < 0)
				local_events(MSRP_ERROR, "\tCouldn't announce text to user %hu in conference %lu", user->ID, switcher->ID);
			else
				local_events(MSRP_LOG, "\tAnnounced text to user %hu in conference %lu", user->ID, switcher->ID);
		}
	MSRP_LIST_STEP(switcher->users, switcher->lock, user)

	return 0;
}
