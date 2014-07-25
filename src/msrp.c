#include "msrp.h"
#include "msrp_utils.h"


/*
	Initialize the library
*/
int msrp_init(void(*app_events)(int event, void *info))
{
	if(msrp_exists)		/* Library is already initialized */
		return -1;

	/* Create a default context for no-relays connections (endpoint-to-endpoint) */
	if(msrp_context_new("default") < 0)
		return -1;

 	srandom(time(0));		/* Initialize the seed for the random() function */

	events = app_events;		/* The callback to notify about events (logs/errors) */

	
	msrp_exists = 1;	

	pthread_t recv_thread;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	if(pthread_create(&(recv_thread), &attr, msrp_recv_thread, NULL) < 0)
		return -1;

	

	/* Setup the linked lists and their locks */
	MSRP_LIST_SETUP(sessions, sessions_lock);
	MSRP_LIST_SETUP(contexts, contexts_lock);
	MSRP_LIST_SETUP(relays, relays_lock);
	MSRP_LIST_SETUP(switches, switches_lock);

	pthread_mutex_init(&counter_lock, NULL);

	sleep(1);

	return 0;
}

/*
	Set the callback for endpoint-related events (e.g. incoming messages)
*/
void msrp_ep_callback(void(*app_ep_callback)(int event, MsrpEndpoint *endpoint, int content, void *data, int bytes))
{
	ep_callback = app_ep_callback;
}

/*
	Set the callback for relay-related events (e.g. ?)
*/
void msrp_rl_callback(void(*app_rl_callback)(MsrpRelay *relay))
{
	rl_callback = app_rl_callback;
}

/*
	Set the callback for switch-related events (e.g. ?)
*/
void msrp_sw_callback(void(*app_sw_callback)(int event, MsrpSwitch *switcher, unsigned short int user, void *data, int bytes))
{
	sw_callback = app_sw_callback;
}


/*
	Quit the library (FIXME)
*/
int msrp_quit(void)
{
	msrp_exists = 0;

	MSRP_LIST_FREE(sessions, sessions_lock);
	MSRP_LIST_FREE(contexts, contexts_lock);
	MSRP_LIST_FREE(relays, relays_lock);
	MSRP_LIST_FREE(switches, switches_lock);

	return 0;
}


/*
	Contexts/External Relays management
*/

/*
	Create a new context
*/
int msrp_context_new(char *name)
{
	if(!name)
		return -1;

	msrp_context *context = calloc(1, sizeof(*context));
	if(!context)
		return -1;
	context->name = calloc(strlen(name)+1, sizeof(char));
	if(!context->name) {
		free(context);
		return -1;
	}
	strcpy(context->name, name);
	context->through = NULL;
	context->relay = NULL;
	context->authenticated = 0;
	context->next = NULL;

	MSRP_LIST_ADD(contexts, contexts_lock, context, -1);

	return 0;
}

/*
	Set the reference external relay for this context
*/
int msrp_context_set_relay(char *context, char *address, unsigned short int port)
{
	if(!context || !address || !port)
		return -1;

	msrp_context *ctx = NULL;
	MSRP_LIST_CHECK(contexts, -1);
	MSRP_LIST_CROSS(contexts, contexts_lock, ctx)
		if(!strcasecmp(ctx->name, context))
			break;
	MSRP_LIST_STEP(contexts, contexts_lock, ctx);
	if(!ctx)
		return -1;

	return msrp_context_setup_relay(ctx, address, port);
}

/*
	Force this context to pass through another one
*/
int msrp_context_pass_through(char *context, char *through)
{
	if(!context || !through)
		return -1;

	/* Look for context */
	msrp_context *ctx = NULL;
	MSRP_LIST_CHECK(contexts, -1);
	MSRP_LIST_CROSS(contexts, contexts_lock, ctx)
		if(!strcasecmp(ctx->name, context))
			break;
	MSRP_LIST_STEP(contexts, contexts_lock, ctx);
	if(!ctx)
		return -1;

	/* Look for context to pass through */
	msrp_context *th = NULL;
	MSRP_LIST_CROSS(contexts, contexts_lock, th)
		if(!strcasecmp(th->name, through))
			break;
	MSRP_LIST_STEP(contexts, contexts_lock, th);
	if(!th)
		return -1;

	ctx->through = th;

	return 0;
}

/*
	Get the full relay path associated with this context
*/
char *msrp_context_get_full_path(char *context)
{
	if(!context)
		return NULL;

	msrp_context *ctx = NULL;
	MSRP_LIST_CHECK(contexts, NULL);
	MSRP_LIST_CROSS(contexts, contexts_lock, ctx)
		if(!strcasecmp(ctx->name, context))
			break;
	MSRP_LIST_STEP(contexts, contexts_lock, ctx);
	if(!ctx)
		return NULL;

	return msrp_context_build_path(ctx);
}

/*
	Destroy an existing context
*/
int msrp_context_destroy(char *context)
{
	if(!context)
		return -1;

	if(msrp_exists && !strcasecmp(context, "default"))
		return -1;	/* Can't destroy the default context while living */

	msrp_context *ctx = NULL;
	MSRP_LIST_CHECK(contexts, -1);
	MSRP_LIST_CROSS(contexts, contexts_lock, ctx)
		if(!strcasecmp(ctx->name, context))
			break;
	MSRP_LIST_STEP(contexts, contexts_lock, ctx);
	if(!ctx)
		return -1;

	MSRP_LIST_REMOVE(contexts, contexts_lock, ctx);

	return msrp_context_free(ctx);
}


/*
	Endpoint Sessions management
*/
/*
	Create a new endpoint
*/
MsrpEndpoint *msrp_endpoint_new(void)
{
	MsrpEndpoint *endpoint = calloc(1, sizeof(*endpoint));
	if(!endpoint)
		return NULL;

	endpoint->ID = msrp_new_identifier();
	endpoint->callid = NULL;
	endpoint->label = 0;
	endpoint->context = NULL;
	endpoint->flags = 0;

	/* Create a new session and add it to the list */
	msrp_session *session = msrp_session_new(endpoint->ID);
	if(!session) {
		local_events(MSRP_ERROR, "Couldn't create session for endpoint %lu", endpoint->ID);
		return NULL;
	}
	session->type = MSRP_ENDPOINT;
	session->session = endpoint;

	MSRP_LIST_ADD(sessions, sessions_lock, session, NULL);

	/* Keep track of it as an opaque pointer in the public endpoint too */
	endpoint->endpoint = session;

	/* The library doesn't use this opaque pointer */
	endpoint->opaque = NULL;

	return endpoint;
}

/*
	Get an existing endpoint out of its ID
*/
MsrpEndpoint *msrp_endpoint_get(unsigned long int ID)
{
	if(!ID)
		return NULL;

	msrp_session *session = NULL;
	MsrpEndpoint *endpoint = NULL;
	MSRP_LIST_CHECK(sessions, NULL);
	MSRP_LIST_CROSS(sessions, sessions_lock, session)
		if(session->session) {
			endpoint = (MsrpEndpoint *)session->session;
			if(endpoint) {
				if(endpoint->ID == ID)
					break;
			}
		}
	MSRP_LIST_STEP(sessions, sessions_lock, session);

	return session ? (MsrpEndpoint *)session->session : NULL;
}

/*
	Set the SIP Call-ID this MSRP endpoint session matches to
*/
int msrp_endpoint_set_callid(MsrpEndpoint *endpoint, char *callid)
{
	if(!endpoint || !callid)
		return -1;

	if(endpoint->callid)
		free(endpoint->callid);
	endpoint->callid = calloc(strlen(callid)+1, sizeof(char));
	if(!endpoint->callid)
		return -1;
	strcpy(endpoint->callid, callid);

	return 0;
}

/*
	Get the SIP Call-ID this MSRP endpoint session matches to
*/
char *msrp_endpoint_get_callid(MsrpEndpoint *endpoint)
{
	if(!endpoint)
		return NULL;

	return endpoint->callid;
}

/*
	Set the SIP label this MSRP endpoint session matches to
*/
int msrp_endpoint_set_label(MsrpEndpoint *endpoint, int label)
{
	if(!endpoint || (label < 0))
		return -1;

	endpoint->label = label;

	return 0;
}

/*
	Set the SIP label this MSRP endpoint session matches to
*/
int msrp_endpoint_get_label(MsrpEndpoint *endpoint)
{
	if(!endpoint)
		return -1;

	return endpoint->label;
}

/*
	Set the external relay context for this endpoint
*/
int msrp_endpoint_set_context(MsrpEndpoint *endpoint, char *context)
{
	if(!endpoint || !context)
		return -1;

	msrp_context *ctx = NULL;
	MSRP_LIST_CHECK(contexts, -1);
	MSRP_LIST_CROSS(contexts, contexts_lock, ctx)
		if(!strcasecmp(ctx->name, context))
			break;
	MSRP_LIST_STEP(contexts, contexts_lock, ctx);
	if(!ctx)
		return -1;

	return 0;
}

/*
	Get the external relay context for this endpoint
*/
char *msrp_endpoint_get_context(MsrpEndpoint *endpoint)
{
	if(!endpoint)
		return NULL;

	return endpoint->context;
}

/*
	Authenticate the endpoint at the current context (AUTH)
*/
int msrp_endpoint_authenticate(MsrpEndpoint *endpoint)
{
	if(!endpoint)
		return -1;

	/* TODO authenticate at context (external relay) */

	return -1;
}

/*
	Set the local peer ("From") for this endpoint
*/
int msrp_endpoint_set_from(MsrpEndpoint *endpoint, char *address, unsigned short int port, int content, int flags, int rights)
{
	if(!endpoint || !address)
		return -1;

	msrp_session *session = (msrp_session *)endpoint->endpoint;
	if(!session) {
		local_events(MSRP_ERROR, "Invalid session for endpoint %lu (set_from)", endpoint->ID);
		return -1;
	}

	/* Create new peer with a random sessionid */
	msrp_peer *peer = msrp_peer_new(NULL);
	if(!peer)
		return -1;
	if(msrp_peer_set_address(peer, address, port) < 0)
		return -1;
	if(msrp_peer_bind(peer) < 0)
		return -1;
	if(msrp_peer_set_options(peer, content, flags) < 0)
		return -1;
	if(msrp_peer_set_rights(peer, rights) < 0)
		return -1;

	return msrp_session_set_from(session, peer);
}

/*
	Get the local peer ("From") address
*/
char *msrp_endpoint_get_from_address(MsrpEndpoint *endpoint)
{
	if(!endpoint)
		return NULL;

	msrp_session *session = (msrp_session *)endpoint->endpoint;
	if(!session) {
		local_events(MSRP_ERROR, "Invalid session for endpoint %lu (get_from_address)", endpoint->ID);
		return NULL;
	}

	return msrp_session_get_address(session, "from");
}

/*
	Get the local peer ("From") port
*/
unsigned short int msrp_endpoint_get_from_port(MsrpEndpoint *endpoint)
{
	if(!endpoint)
		return 0;

	msrp_session *session = (msrp_session *)endpoint->endpoint;
	if(!session) {
		local_events(MSRP_ERROR, "Invalid session for endpoint %lu (get_from_port)", endpoint->ID);
		return 0;
	}

	return msrp_session_get_port(session, "from");
}

/*
	Get the local peer ("From") MSRP Session-ID
*/
char *msrp_endpoint_get_from_sessionid(MsrpEndpoint *endpoint)
{
	if(!endpoint)
		return NULL;

	msrp_session *session = (msrp_session *)endpoint->endpoint;
	if(!session) {
		local_events(MSRP_ERROR, "Invalid session for endpoint %lu (get_from_sessionid)", endpoint->ID);
		return NULL;
	}

	return msrp_session_get_sessionid(session, "from");
}

/*
	Get the local peer ("From") full MSRP URL path
*/
char *msrp_endpoint_get_from_fullpath(MsrpEndpoint *endpoint)
{
	if(!endpoint)
		return NULL;

	msrp_session *session = (msrp_session *)endpoint->endpoint;
	if(!session) {
		local_events(MSRP_ERROR, "Invalid session for endpoint %lu (get_from_fullpath)", endpoint->ID);
		return NULL;
	}

	return msrp_session_get_fullpath(session, "from");
}

/*
	Get the local peer ("From") accepted types
*/
char *msrp_endpoint_get_from_accepttypes(MsrpEndpoint *endpoint)
{
	if(!endpoint)
		return NULL;

	msrp_session *session = (msrp_session *)endpoint->endpoint;
	if(!session) {
		local_events(MSRP_ERROR, "Invalid session for endpoint %lu (get_from_accepttypes)", endpoint->ID);
		return NULL;
	}

	return msrp_session_get_accepttypes(session, "from");
}

/*
	Set the remote peer ("To") for this endpoint
*/
int msrp_endpoint_set_to(MsrpEndpoint *endpoint, const char *path, int content, int flags, int rights)
{
	if(!endpoint || !path)
		return -1;

	msrp_session *session = (msrp_session *)endpoint->endpoint;
	if(!session) {

		printf("Invalid session for endpoint %lu (set_to)\n", endpoint->ID);
		
		local_events(MSRP_ERROR, "Invalid session for endpoint %lu (set_to)", endpoint->ID);
		return -1;
	}

	if(!session->from) {

		printf("Invalid 'From' for endpoint %lu (set_to)\n", endpoint->ID);

		local_events(MSRP_ERROR, "Invalid 'From' for endpoint %lu (set_to)", endpoint->ID);
		return -1;
	}

	/* Parse path to create new peer */
	while(*path && (*path < 33))
		path++;
	char *sessionid = NULL, *address = NULL;
	unsigned short int port = 0;
	const char *pnt = NULL;
	int pos = 0;
	pnt = stristr((const char *)path, "msrp://");
	pos += 7;
	if(!pnt) {	/* Try msrps */
		pnt = stristr((const char *)path, "msrps://");
		pos += 1;
	}
	if(!pnt)	/* Invalid path */
		return -1;
	path += pos;
	pnt = strstr(path, ":");
	if(!pnt)
		return -1;
	address = calloc(pnt - path + 1, sizeof(char));
	strncpy(address, path, pnt - path);
	path = pnt + 1;
	port = atoi(path);
	pnt = strstr(path, "/");
	if(!pnt)
		return -1;
	path = pnt + 1;
	pnt = stristr((const char *)path, ";tcp");
	if(!pnt)
		return -1;
	sessionid = calloc(pnt - path + 1, sizeof(char));
	strncpy(sessionid, path, pnt - path);

	local_events(MSRP_LOG, "Path parsed:");
	local_events(MSRP_LOG, "\tAddress:\t%s", address);
	local_events(MSRP_LOG, "\tPort n.:\t%hu", port);
	local_events(MSRP_LOG, "\tSessionID:\t%s", sessionid);

	printf("Path parsed:\tAddress:\t%s\tPort n.:\t%hu\tSessionID:\t%s\n", address, port, sessionid);

	msrp_peer *peer = msrp_peer_new(sessionid);
	if(!peer)
		return -1;
	peer->session = session;
	if(msrp_peer_set_address(peer, address, port) < 0)
		return -1;
	if(msrp_peer_set_options(peer, content, flags) < 0)
		return -1;
	if(msrp_peer_set_rights(peer, rights) < 0)
		return -1;

	if(msrp_session_set_to(session, peer) < 0)
		return -1;

	return msrp_session_connect(session);
}

/*
	Get the remote peer ("To") address
*/
char *msrp_endpoint_get_to_address(MsrpEndpoint *endpoint)
{
	if(!endpoint)
		return NULL;

	msrp_session *session = (msrp_session *)endpoint->endpoint;
	if(!session) {
		local_events(MSRP_ERROR, "Invalid session for endpoint %lu (get_to_address)", endpoint->ID);
		return NULL;
	}

	return msrp_session_get_address(session, "to");
}

/*
	Get the remote peer ("To") port
*/
unsigned short int msrp_endpoint_get_to_port(MsrpEndpoint *endpoint)
{
	if(!endpoint)
		return 0;

	msrp_session *session = (msrp_session *)endpoint->endpoint;
	if(!session) {
		local_events(MSRP_ERROR, "Invalid session for endpoint %lu (get_to_port)", endpoint->ID);
		return 0;
	}

	return msrp_session_get_port(session, "to");
}

/*
	Get the remote peer ("To") MSRP Session-ID
*/
char *msrp_endpoint_get_to_sessionid(MsrpEndpoint *endpoint)
{
	if(!endpoint)
		return NULL;

	msrp_session *session = (msrp_session *)endpoint->endpoint;
	if(!session) {
		local_events(MSRP_ERROR, "Invalid session for endpoint %lu (get_to_sessionid)", endpoint->ID);
		return NULL;
	}

	return msrp_session_get_sessionid(session, "to");
}

/*
	Get the remote peer ("To") full MSRP URL path
*/
char *msrp_endpoint_get_to_fullpath(MsrpEndpoint *endpoint)
{
	if(!endpoint)
		return NULL;

	msrp_session *session = (msrp_session *)endpoint->endpoint;
	if(!session) {
		local_events(MSRP_ERROR, "Invalid session for endpoint %lu (get_to_fullpath)", endpoint->ID);
		return NULL;
	}

	return msrp_session_get_fullpath(session, "to");
}

/*
	Get the remote peer ("To") accepted-types
*/
char *msrp_endpoint_get_to_accepttypes(MsrpEndpoint *endpoint)
{
	if(!endpoint)
		return NULL;

	msrp_session *session = (msrp_session *)endpoint->endpoint;
	if(!session) {
		local_events(MSRP_ERROR, "Invalid session for endpoint %lu (get_to_accepttypes)", endpoint->ID);
		return NULL;
	}

	return msrp_session_get_accepttypes(session, "to");
}

/*
	Destroy an existong endpoint session
*/
int msrp_endpoint_destroy(MsrpEndpoint *endpoint)
{
	if(!endpoint)
		return -1;

	msrp_session *session = (msrp_session *)endpoint->endpoint;
	if(!session) {
		local_events(MSRP_ERROR, "Couldn't destroy session for endpoint %lu", endpoint->ID);
		return -1;
	}

	if(msrp_session_destroy(session), 0)
		return -1;

	if(endpoint->callid)
		free(endpoint->callid);
	if(endpoint->context)
		free(endpoint->context);
	free(endpoint);

	return 0;
}


/*
	Switches (conference rooms) management
*/
/*
	Create a new switch (conference room)
*/
MsrpSwitch *msrp_switch_new()
{
	MsrpSwitch *sw = calloc(1, sizeof(*sw));
	if(!sw) {
		local_events(MSRP_ERROR, "Not enough memory");
		return NULL;
	}

	sw->ID = msrp_new_identifier();
	msrp_conference *conf = msrp_conference_new(sw, sw->ID);
	if(!conf) {
		local_events(MSRP_ERROR, "Error creating new private conference structure (%lu)", sw->ID);
		return NULL;
	}

	MSRP_LIST_ADD(switches, switches_lock, conf, NULL);

	/* Keep track of it as an opaque pointer in the public switch too */
	sw->switcher = conf;

	/* The library doesn't use this opaque pointer */
	sw->opaque = NULL;

	return sw;
}

/*
	Set the local peer ("From") for this switch
*/
int msrp_switch_set_from(MsrpSwitch *switcher, char *address, unsigned short int port)
{
	if(!switcher || !address)
		return -1;

	msrp_conference *conf = msrp_conference_find(switcher);
	if(!conf)
		return -1;

	/* Create new peer with a random sessionid */
	msrp_peer *peer = msrp_peer_new(NULL);
	if(!peer)
		return -1;
	if(msrp_peer_set_address(peer, address, port) < 0)
		return -1;
	if(msrp_peer_bind(peer) < 0)
		return -1;
	if(msrp_peer_set_options(peer, MSRP_TEXT_PLAIN,
		MSRP_PASSIVE | MSRP_OVER_TCP) < 0)
			return -1;
	if(msrp_peer_set_rights(peer, MSRP_SENDRECV) < 0)
		return -1;

	return msrp_conference_set_from(conf, peer);
}

/*
	Get the address of the switch server
*/
char *msrp_switch_get_address(MsrpSwitch *switcher)
{
	if(!switcher)
		return NULL;

	msrp_conference *conf = msrp_conference_find(switcher);
	if(!conf)
		return NULL;
	
	return conf->server ? conf->server->address : NULL;
}

/*
	Get the port of the switch server
*/
unsigned short int msrp_switch_get_port(MsrpSwitch *switcher)
{
	if(!switcher)
		return 0;

	msrp_conference *conf = msrp_conference_find(switcher);
	if(!conf)
		return 0;

	return conf->server ? conf->server->port : 0;
}

/*
	Get the Session-ID of the switch server
*/
char *msrp_switch_get_sessionid(MsrpSwitch *switcher)
{
	if(!switcher)
		return NULL;

	msrp_conference *conf = msrp_conference_find(switcher);
	if(!conf)
		return NULL;

	return conf->server ? conf->server->sessionid : NULL;
}

/*
	Get the full path of the switch server
*/
char *msrp_switch_get_fullpath(MsrpSwitch *switcher)
{
	if(!switcher)
		return NULL;

	msrp_conference *conf = msrp_conference_find(switcher);
	if(!conf)
		return NULL;

	return conf->server ? msrp_peer_get_path(conf->server) : NULL;
}

/*
	Get a switch (conference room) from its unique ID
*/
MsrpSwitch *msrp_switch_get(unsigned long int ID)
{
	if(!ID)
		return NULL;

	msrp_conference *conf = NULL;
	MSRP_LIST_CHECK(switches, NULL);
	MSRP_LIST_CROSS(switches, switches_lock, conf)
		if(conf->ID == ID)
			break;
	MSRP_LIST_STEP(switches, switches_lock, conf);

	return conf ? conf->sw : NULL;
}

/*
	Add a user to a switch (conference room)
*/
unsigned short int msrp_switch_add_user(MsrpSwitch *switcher, char *display, const char *path, int content, int flags, int rights)
{
	if(!switcher || !path)
		return -1;

	msrp_conference *conf = msrp_conference_find(switcher);
	if(!conf)
		return -1;

	/* Parse path to create new peer */
	while(*path && (*path < 33))
		path++;
	char *sessionid = NULL, *address = NULL;
	unsigned short int port = 0;
	const char *pnt = NULL;
	int pos = 0;
	pnt = stristr((const char *)path, "msrp://");
	pos += 7;
	if(!pnt) {	/* Try msrps */
		pnt = stristr((const char *)path, "msrps://");
		pos += 1;
	}
	if(!pnt)	/* Invalid path */
		return -1;
	path += pos;
	pnt = strstr(path, ":");
	if(!pnt)
		return -1;
	address = calloc(pnt - path + 1, sizeof(char));
	strncpy(address, path, pnt - path);
	path = pnt + 1;
	port = atoi(path);
	pnt = strstr(path, "/");
	if(!pnt)
		return -1;
	path = pnt + 1;
	pnt = stristr((const char *)path, ";tcp");
	if(!pnt)
		return -1;
	sessionid = calloc(pnt - path + 1, sizeof(char));
	strncpy(sessionid, path, pnt - path);

	local_events(MSRP_LOG, "Path parsed:");
	local_events(MSRP_LOG, "\tAddress:\t%s", address);
	local_events(MSRP_LOG, "\tPort n.:\t%hu", port);
	local_events(MSRP_LOG, "\tSessionID:\t%s", sessionid);

	msrp_peer *peer = msrp_peer_new(sessionid);
	if(!peer)
		return -1;
	peer->session = NULL;
	if(msrp_peer_set_address(peer, address, port) < 0)
		return -1;
	if(msrp_peer_set_options(peer, content, flags) < 0)
		return -1;
	if(msrp_peer_set_rights(peer, rights) < 0)
		return -1;

	return msrp_conference_add_user(conf, display, peer);
}

/*
	Update the rights for a specific user (e.g. mute a user by setting MSRP_RECVONLY)
*/
int msrp_switch_update_user_rights(MsrpSwitch *switcher, unsigned short int ID, int rights)
{
	if(!switcher || !ID)
		return -1;

	msrp_conference *conf = msrp_conference_find(switcher);
	if(!conf)
		return -1;

	return msrp_conference_set_user_rights(conf, ID, rights);
}

/*
	Remove a user from a switch (conference room)
*/
int msrp_switch_remove_user(MsrpSwitch *switcher, unsigned short int ID)
{
	if(!switcher || !ID)
		return -1;

	msrp_conference *conf = msrp_conference_find(switcher);
	if(!conf)
		return -1;

	return msrp_conference_remove_user(conf, ID);
}

/*
	Send a SEND message to a user (all users if ID=0) in a switch (conference room)
*/
int msrp_switch_message(MsrpSwitch *switcher, unsigned short int ID, char *text)
{
	if(!switcher || !text)
		return -1;

	msrp_conference *conf = msrp_conference_find(switcher);
	if(!conf)
		return -1;

	return msrp_conference_message(conf, ID, text);
}

/*
	Send a REPORT announcement to a user (all users if ID=0) in a switch (conference room)
*/
int msrp_switch_announcement(MsrpSwitch *switcher, unsigned short int ID, char *text)
{
	if(!switcher || !text)
		return -1;

	msrp_conference *conf = msrp_conference_find(switcher);
	if(!conf)
		return -1;

	return msrp_conference_announcement(conf, ID, text);
}

/*
	Destroy an existing switch (conference room)
*/
int msrp_switch_destroy(MsrpSwitch *switcher)
{
	if(!switcher)
		return -1;

	msrp_conference *conf = msrp_conference_find(switcher);
	if(!conf)
		return -1;

	return msrp_conference_destroy(conf);
}


/*
	Wrappers to send messages (Endpoints only)
*/
/*
	Send a SEND text message, requesting reports if necessary
*/
int msrp_send_text(MsrpEndpoint *endpoint, char *text, int reports)
{
	if(!endpoint)
		return -1;

	msrp_session *session = (msrp_session *)endpoint->endpoint;
	if(!session) {
		local_events(MSRP_ERROR, "Invalid session for endpoint %lu (send_text)", endpoint->ID);
		return -1;
	}

	return msrp_queue_text(MSRP_SEND, session, text, reports, 0);
}

/*
	Send a REPORT text message, with a status report if needed
*/
int msrp_report_text(MsrpEndpoint *endpoint, char *text, int status)
{
	if(!endpoint)
		return -1;

	msrp_session *session = (msrp_session *)endpoint->endpoint;
	if(!session) {
		local_events(MSRP_ERROR, "Invalid session for endpoint %lu (send_report)", endpoint->ID);
		return -1;
	}

	return msrp_queue_text(MSRP_REPORT, session, text, 0, status);
}
