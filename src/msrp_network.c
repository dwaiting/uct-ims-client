#include "msrp_network.h"


/*
	Receiving thread
*/
void *msrp_recv_thread(void *data)
{
	local_events(MSRP_LOG, "Thread created");
	int err = 0, fd = 0, client = 0;
	struct sockaddr_in client_address;
	unsigned int addrlen = sizeof(struct sockaddr);
	char buffer[4096];
	fd_set tempfds;
	FD_ZERO(&recv_fds);
	/* Create a secret socket pair to unblock select, needed to update recv_fds */
	if(socketpair(AF_UNIX, SOCK_STREAM, 0, recv_pair) < 0)
		local_events(MSRP_ERROR, "Error setting Socket pair...");
	FD_SET(recv_pair[0], &recv_fds);
	if(recv_pair[0] > recv_fdmax)
		recv_fdmax = recv_pair[0];

	while(msrp_exists) {
		tempfds = recv_fds;
		err = select(recv_fdmax+1, &tempfds, NULL, NULL, (struct timeval *)NULL);
		local_events(MSRP_LOG, "Select unblocked (err = %d)", err);
		if(err < 0)
			break;
		else {
			/* Check which descriptor has data here ... */
			for(fd = 0; fd <= recv_fdmax; fd++) {
				if(FD_ISSET(fd, &tempfds)) {
					local_events(MSRP_LOG, "File descriptor %d is set", fd);
					if(fd == recv_pair[0]) {
						local_events(MSRP_LOG, "Unblocking select to refresh fdset");
						/* Unblock select to refresh fds */
						recv(fd, &buffer, 4096, 0);
						continue;
					}
					/* TODO check if incoming message is for session or conference */
					/* Get the session associated with this fd */
					msrp_session *session = msrp_session_get(fd);
					if(session) {	/* It's a valid session */
						if(!session->from) {
							/* Drop the buffer */
							local_events(MSRP_LOG, "No session associated to file descriptor %d, dropping buffer", fd);
							err = recv(fd, &buffer, 4096, 0);
							if(err < -1)
								local_events(MSRP_LOG, "Read error on file descriptor %d", fd);
							else if(err == 0)
								local_events(MSRP_LOG, "File descriptor %d disconnected", fd);
							else
								local_events(MSRP_LOG, "Dropped %d bytes (file descriptor %d)", fd);
							if(session->type == MSRP_ENDPOINT)
								local_ep_callback(MSRP_ENDPOINT_DUMPED, session->session, 0, NULL, 0);
							msrp_recv_del_fd(fd);
							continue;
						}
						if((session->from->flags & MSRP_PASSIVE) && (fd == session->from->fd)) {
							/* It's a 'connect', accept */
							client = accept(fd, (struct sockaddr *)(&client_address), &addrlen);
							if(client < 0)
								continue;
							if(session->type == MSRP_SWITCH) {
// 								/* This session belongs to a conference, check the sender's address to match it */
								if(msrp_conference_match_user((msrp_conference *)session->session, client, &client_address) < 0) {
									local_events(MSRP_LOG, "Unexpected connection to switch from %s:%hu (fd %d)",
										inet_ntoa(client_address.sin_addr) ? inet_ntoa(client_address.sin_addr) : "???.???.???.???",
										ntohs(client_address.sin_port), client);
									close(client);
								} else {
									local_events(MSRP_LOG, "Conference user connected from %s:%hu (fd %d)",
										inet_ntoa(client_address.sin_addr) ? inet_ntoa(client_address.sin_addr) : "???.???.???.???",
										ntohs(client_address.sin_port), client);
									msrp_recv_add_fd(client);
								}
							} else {	/* Normal session (TODO handle relays) */
								msrp_recv_del_fd(session->from->fd);	/* Stop accepting connections */
								if(session->to) {
									local_events(MSRP_LOG, "Peer '%s' connected from %s:%hu (fd %d)",
										session->to->sessionid ? session->to->sessionid : "??",
										inet_ntoa(client_address.sin_addr) ? inet_ntoa(client_address.sin_addr) : "???.???.???.???",
										ntohs(client_address.sin_port), client);
									msrp_recv_add_fd(client);
									session->to->fd = client;
									if(session->type == MSRP_ENDPOINT)
										local_ep_callback(MSRP_REMOTE_CONNECT, session->session, 0, NULL, 0);
								} else {
									local_events(MSRP_LOG, "Unexpected connection from from %s:%hu (fd %d)",
										inet_ntoa(client_address.sin_addr) ? inet_ntoa(client_address.sin_addr) : "???.???.???.???",
										ntohs(client_address.sin_port), client);
									close(client);
								}
							}
						} else {
							/* There's data to read from our peer in this session */
							err = recv(fd, &buffer, 4096, 0);
							if(err < 1) {	/* This client disconnected */
								local_events(MSRP_LOG, "Peer (fd %d) disconnected", fd);
								/* FIXME remove session */
								if(session->to && (fd == session->to->fd)) {
									if(session->type == MSRP_ENDPOINT) {
										local_ep_callback(MSRP_REMOTE_DISCONNECT, session->session, 0, NULL, 0);
										msrp_session_destroy(session);
									} else if(session->type == MSRP_SWITCH) {
										msrp_conf_user *user = (msrp_conf_user *)session->to->opaque;
										if(user)
											msrp_conference_remove_user((msrp_conference *)session->session, user->ID);
									}
								} else if(session->from && (fd == session->from->fd)) {
									if(session->type == MSRP_ENDPOINT)
										local_ep_callback(MSRP_LOCAL_DISCONNECT, session->session, 0, NULL, 0);
									msrp_session_destroy(session);
								}
								msrp_recv_del_fd(fd);
							} else {	/* Read chunk */
								local_events(MSRP_LOG, "Incoming buffer (%d bytes), parse it", err);
								msrp_buffer_parse(fd, session, buffer, err);
							}
						}
					} else {
						local_events(MSRP_LOG, "No session/relay/conference associated to file descriptor %d yet", fd);
						err = recv(fd, &buffer, 4096, 0);
						if(err < -1)
							local_events(MSRP_LOG, "Read error on file descriptor %d", fd);
						else if(err == 0)
							local_events(MSRP_LOG, "File descriptor %d disconnected", fd);
						else {	/* Drop the buffer */
							local_events(MSRP_LOG, "Dropped %d bytes (file descriptor %d)", fd);
							msrp_recv_del_fd(fd);
						}
						continue;
					}
				}
			}
		}
	}

	pthread_exit(0);
}

/*
	Add a new file descriptor to the receiving set for select
*/
void msrp_recv_add_fd(int fd)
{
	if(fd < 0)
		return;

	local_events(MSRP_LOG, "File descriptor %d added to fdset", fd);

	pthread_mutex_lock(&recv_lock);
	/* Add the new file descriptor to the receiving fds */
	FD_SET(fd, &recv_fds);
	if(fd > recv_fdmax)
		recv_fdmax = fd;
	/* Unblock the select by writing a byte onto one end of the socket pair */
	char ch = '\0';
	int err = send(recv_pair[1], &ch, 1, 0);
	if(err < 1)
		local_events(MSRP_ERROR, "Error unblocking select");
	pthread_mutex_unlock(&recv_lock);
}

/*
	Remove a new file descriptor from the receiving set for select
*/
void msrp_recv_del_fd(int fd)
{
	if(fd < 0)
		return;

	local_events(MSRP_LOG, "File descriptor %d removed from fdset", fd);

	pthread_mutex_lock(&recv_lock);
	/* Remove the file descriptor from the receiving fds */
	int deleted_fd = fd;
	FD_CLR(fd, &recv_fds);
	/* Unblock the select by writing a byte onto one end of the socket pair */
	send(recv_pair[1], '\0', 1, 0);
	pthread_mutex_unlock(&recv_lock);
	close(deleted_fd);
	shutdown(deleted_fd, SHUT_RDWR);
}
