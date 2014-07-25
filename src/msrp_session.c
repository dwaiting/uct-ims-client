#include "msrp_session.h"

/*
	Contexts management
*/
/*
	Recursive function to get the full context path, considering its relay hops
*/
char *msrp_context_build_path(msrp_context *context)
{
	if(!context)
		return NULL;
	if(!context->relay)
		return NULL;

	/* Get previous paths recursively */
	char *path = msrp_context_build_path(context->through);
	if(context->through && !path)	/* Previous hops have no path? */
		return NULL;
	char *relay = msrp_peer_get_path(context->relay);
	if(!relay)
		return NULL;

	if(!context->through)	/* Just one relay */
		return relay;
	
	char *fullpath = calloc(strlen(path) + strlen(relay)+1, sizeof(char));
	if(!fullpath)
		return NULL;
	/* Concatenate the paths from the previous hops and the context's relay */
	sprintf(fullpath, "%s %s", path, relay);
	free(path);

	return fullpath;
}

/*
	Setup the external relay this context will refer to
*/
int msrp_context_setup_relay(msrp_context *context, char *address, unsigned short int port)
{
	if(!context || !address || !port)
		return -1;

	/* TODO build relay */

	return 0;
}

/*
	Free an existing context
*/
int msrp_context_free(msrp_context *context)
{
	if(!context)
		return -1;

	free(context->name);
	if(context->relay)	/* FIXME actually remove relay peer */
		free(context->relay);
	free(context);

	return 0;
}


/*
	Sessions management
*/
/*
	Create a new session
*/
msrp_session *msrp_session_new(unsigned long int ID)
{
	if(!ID)
		return NULL;

	msrp_session *session = calloc(1, sizeof(*session));
	if(!session)
		return NULL;

	session->callid = NULL;
	session->context = NULL;
	session->session = NULL;
	session->from = NULL;
	session->to = NULL;
	session->fd = -1;

	int j = 0;
	for(j = 0; j < MSRP_MSG_BUFFER; j++) {
		session->in_msg[j] = NULL;
		session->out_msg[j] = NULL;
	}

	session->next = NULL;

	return session;
}

/*
	Set the local peer ("From") in this session
*/
int msrp_session_set_from(msrp_session *session, msrp_peer *from)
{
	if(!session || !from)
		return -1;

	session->from = from;
	from->session = session;

	return 0;
}

/*
	Set the remote peer ("To") in this session
*/
int msrp_session_set_to(msrp_session *session, msrp_peer *to)
{
	if(!session || !to)
		return -1;

	session->to = to;
	to->session = session;

	return 0;
}

/*
	Get the address for a peer (whose? "From"/"To") in this session
*/
char *msrp_session_get_address(msrp_session *session, char *whose)
{
	if(!session || !whose)
		return NULL;

	msrp_peer *who = NULL;
	if(!strcasecmp(whose, "from"))
		who = session->from;
	else if(!strcasecmp(whose, "to"))
		who = session->to;
	if(!who)
		return NULL;

	return who->address;
}

/*
	Get the port for a peer (whose? "From"/"To") in this session
*/
unsigned short int msrp_session_get_port(msrp_session *session, char *whose)
{
	if(!session || !whose)
		return 0;

	msrp_peer *who = NULL;
	if(!strcasecmp(whose, "from"))
		who = session->from;
	else if(!strcasecmp(whose, "to"))
		who = session->to;
	if(!who)
		return 0;

	return who->port;
}

/*
	Get the session ID for a peer (whose? "From"/"To") in this session
*/
char *msrp_session_get_sessionid(msrp_session *session, char *whose)
{
	if(!session || !whose)
		return NULL;

	msrp_peer *who = NULL;
	if(!strcasecmp(whose, "from"))
		who = session->from;
	else if(!strcasecmp(whose, "to"))
		who = session->to;
	if(!who)
		return NULL;

	return who->sessionid;
}

/*
	Get the full MSRP URL path for a peer (whose? "From"/"To") in this session
*/
char *msrp_session_get_fullpath(msrp_session *session, char *whose)
{
	if(!session || !whose)
		return NULL;

	msrp_peer *who = NULL;
	if(!strcasecmp(whose, "from"))
		who = session->from;
	else if(!strcasecmp(whose, "to"))
		who = session->to;
	if(!who) {
		local_events(MSRP_ERROR, "Invalid whose '%s'", who);
		return NULL;
	}

	return msrp_peer_get_path(who);		/* TODO add paths from context */
}

/*
	Get the accepted-types for a peer (whose? "From"/"To") in this session
*/
char *msrp_session_get_accepttypes(msrp_session *session, char *whose)
{
	if(!session || !whose)
		return NULL;

	msrp_peer *who = NULL;
	if(!strcasecmp(whose, "from"))
		who = session->from;
	else if(!strcasecmp(whose, "to"))
		who = session->to;
	if(!who)
		return NULL;

	char *types = calloc(1, sizeof(char));	/* FIXME */
	if(who->flags & MSRP_TEXT_PLAIN) {
		types = realloc(types, strlen(types) + strlen("text/plain "));
		strcat(types, "text/plain ");
	}
	if(who->flags & MSRP_TEXT_HTML) {
		types = realloc(types, strlen(types) + strlen("text/html "));
		strcat(types, "text/html ");
	}

	return types;
}

/*
	Get session from the associated file descriptor(s)
*/
msrp_session *msrp_session_get(int fd)
{
	if(fd < 1)
		return NULL;

	msrp_session *session = NULL;
	msrp_peer *from = NULL, *to = NULL;
	MSRP_LIST_CHECK(sessions, NULL);
	MSRP_LIST_CROSS(sessions, sessions_lock, session)
		from = session->from;
		to = session->to;
		if(session->fd == fd)
			break;
		if(from) {
			if(from->fd == fd)
				break;
		}
		if(to) {
			if(to->fd == fd)
				break;
		}
	MSRP_LIST_STEP(sessions, sessions_lock, session);

	return session;
}

/*
	Setup the connection between the two peers of this session
*/
int msrp_session_connect(msrp_session *session)
{
	if(!session)
		return -1;
	msrp_peer *from = session->from;
	msrp_peer *to = session->to;
	if(!from || !to)
		return -1;

	if(from->flags & MSRP_PASSIVE)		/* Start server */
		return msrp_peer_listen(from);
	else if(from->flags & MSRP_ACTIVE)	/* Start client */
		return msrp_peer_connect(from, to);
	else
		return -1;
}

/*
	Destroy an existing session
*/
int msrp_session_destroy(msrp_session *session)
{
	if(!session)
		return -1;

	/* Remove from list of sessions */
	MSRP_LIST_REMOVE(sessions, sessions_lock, session);

	/* FIXME we don't take care of the whole session, peers are destroyed outside */
	free(session);

	return 0;
}


/*
	Peers management
*/
/*
	Create a new peer entity
*/
msrp_peer *msrp_peer_new(char *sessionid)
{
	msrp_peer *peer = calloc(1, sizeof(*peer));
	if(!peer)
		return NULL;

	if(!sessionid) {	/* Create a new random Session-ID */
		peer->sessionid = calloc(13, sizeof(char));
		random_string(peer->sessionid, 13);
	} else {		/* Copy the SDP-negotiated ID */
		peer->sessionid = calloc(strlen(sessionid)+1, sizeof(char));
		if(!peer->sessionid) {
			free(peer);
			return NULL;
		}
		peer->sessionid = strcpy(peer->sessionid, sessionid);
	}
	local_events(MSRP_LOG, "Created peer with Session-ID %s", peer->sessionid);

	peer->session = NULL;
	peer->path = NULL;
	peer->address = NULL;
	peer->port = 0;
	peer->sockaddr = NULL;
	peer->fd = -1;
	peer->flags = 0;
	peer->rights = 0;
	peer->content = 0;
	peer->opaque = NULL;

	return peer;
}

/*
	Set the transport address for this peer
*/
int msrp_peer_set_address(msrp_peer *peer, char *address, unsigned short int port)
{
	if(!peer || !address)
		return -1;

	peer->address = calloc(strlen(address)+1, sizeof(char));
	if(!peer->address)
		return -1;
	strcpy(peer->address, address);
	peer->port = port;

	return 0;
}

/*
	Set the options for this peer
*/
int msrp_peer_set_options(msrp_peer *peer, int content, int flags)
{
	if(!peer)
		return -1;

	peer->content = content;
	peer->flags = flags;

	return 0;
}

/*
	Update the read/write rights for this peer
*/
int msrp_peer_set_rights(msrp_peer *peer, int rights)
{
	if(!peer)
		return -1;

	peer->rights = rights;

	return 0;
}

/*
	Get the path for this peer
*/
char *msrp_peer_get_path(msrp_peer *peer)
{
	if(!peer)
		return NULL;

	if(!peer->path) {
		/* Build the full MSRP path */
		peer->path = calloc(20 + strlen(peer->address) + strlen(peer->sessionid), sizeof(char));
		if(!peer->path)
			return NULL;
		sprintf(peer->path, "msrp://%s:%hu/%s;tcp", peer->address, peer->port, peer->sessionid);
	}

	return peer->path;
}

/*
	If this is a local peer, have it bind to the provided port
*/
int msrp_peer_bind(msrp_peer *peer)
{
	if(!peer)
		return -1;

	/* Create a socket and bind it to the provided port */
	peer->sockaddr = calloc(1, sizeof(peer->sockaddr));
	peer->sockaddr->sin_family = AF_INET;
	peer->sockaddr->sin_addr.s_addr = INADDR_ANY;
	peer->fd = socket(AF_INET, SOCK_STREAM, 0);
	if(peer->fd < 0)
		return -1;

	int yes = 1;
	if(setsockopt(peer->fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0) {
		close(peer->fd);
		peer->fd = -1;
		return -1;
	}

	unsigned short int port = peer->port;
	int retry = 0, success = 0, randomport = 0;
	if(!port)
		randomport++;
	for (retry = 0; retry < 100; retry++) {	/* This could be needed for the random port */
		if(randomport)
			port = 2000 + random() % 8000;
		peer->sockaddr->sin_port = htons(port);
		if(bind(peer->fd, (struct sockaddr *)(peer->sockaddr), sizeof(struct sockaddr)) < 0) {
			if(!randomport) {	/* Port was explicit, fail */
				close(peer->fd);
				peer->fd = -1;
				return -1;
			}
		} else {
			success++;
			break;
		}
	}

	if(success)
		peer->port = port;
	else {
		close(peer->fd);
		peer->fd = -1;
		return -1;
	}

	return 0;
}

/*
	If this is a passive local peer, have it start listening (server)
*/
int msrp_peer_listen(msrp_peer *peer)
{
	if(!peer)
		return -1;
	if(!(peer->flags & MSRP_PASSIVE))
		return -1;
	if(peer->fd < 1)
		return -1;

	/* Start listening on the provided port */
	if(listen(peer->fd, 5) < 0) {
		close(peer->fd);
		peer->fd = -1;
		return -1;
	}
	msrp_recv_add_fd(peer->fd);
	if(peer->session)
		peer->session->fd = peer->fd;

	return 0;
}

/*
	If this is an active local peer, have it start connecting (client)
*/
int msrp_peer_connect(msrp_peer *peer, msrp_peer* dst)
{
	if(!peer || !dst)
		return -1;
	if(!(peer->flags & MSRP_ACTIVE))
		return -1;
	if(!(dst->flags & MSRP_PASSIVE))
		return -1;
	if(peer->fd < 1)
		return -1;

	/* We're going to be the client, connect to the other peer */
	dst->sockaddr = calloc(1, sizeof(dst->sockaddr));
	dst->sockaddr->sin_family = AF_INET;
	dst->sockaddr->sin_port = htons(dst->port);
	if(inet_aton(dst->address, &(dst->sockaddr->sin_addr)) == 0) {	/* Not a numeric IP... */
		struct hostent *host = gethostbyname(dst->address);	/* ...resolve name */
		if(!host) {
			local_events(MSRP_ERROR, "Invalid host for address %s",
				dst->address ? dst->address : "???.???.???.???");
			return -1;
		}
		dst->sockaddr->sin_addr = *(struct in_addr *)host->h_addr_list;
	}
	if(connect(peer->fd, (struct sockaddr *)dst->sockaddr, sizeof(struct sockaddr_in)) < 0) {
		local_events(MSRP_ERROR, "Couldn't connect to %s:%hu", dst->address, dst->port);
		return -1;
	}
	local_events(MSRP_LOG, "Connected at %s:%hu", dst->address, dst->port);
	dst->fd = peer->fd;
	msrp_recv_add_fd(dst->fd);

	if(peer->session->type == MSRP_ENDPOINT)
		local_ep_callback(MSRP_LOCAL_CONNECT, peer->session->session, 0, NULL, 0);
	/* TODO what else? */

	return 0;
}

/*
	Destroy an existing peer entity
*/
int msrp_peer_destroy(msrp_peer *peer)
{
	if(!peer)
		return -1;

	if(peer->path)
		free(peer->path);
	if(peer->address)
		free(peer->address);
	if(peer->sockaddr)
		free(peer->sockaddr);
	if(peer->sessionid)
		free(peer->sessionid);
	shutdown(peer->fd, SHUT_RDWR);
	close(peer->fd);

	free(peer);

	return 0;
}
