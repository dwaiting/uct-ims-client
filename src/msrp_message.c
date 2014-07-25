#include "msrp_message.h"


/*
	Create a new MSRP message container
*/
msrp_message *msrp_message_new(char *messageid, int content, int bytes)
{
	msrp_message *message = calloc(1, sizeof(*message));
	if(!message)
		return NULL;

	message->content = content;

	if(messageid) {	/* We're receiving a message */
		message->messageid = calloc(strlen(messageid)+1, sizeof(char));
		strcpy(message->messageid, messageid);
	} else {
		/* Generate random Message-ID for outgoing message */
		message->messageid = calloc(9, sizeof(char));
		message->messageid = random_string(message->messageid, 9);
	}

	if(bytes > 0)
		message->data = calloc(bytes+1, sizeof(char));
	else
		message->data = NULL;
	message->bytes = bytes;

	return message;
}


/*
	Setup the MSRP message arguments
*/
int msrp_message_setup(msrp_message *message, msrp_session *session, int method, int reports, int status)
{
	if(!message || !session)
		return -1;

	message->session = session;
	message->method = method;
	message->reports = reports;
	message->status = status;

	return 0;
}


/*
	Fill in the buffer to the message data from 'start' to 'end'
*/
int msrp_message_fill(msrp_message *message, char *data, int start, int end)
{
	if(!message || !data)
		return -1;
	if((start < 0) || (end < 0) || (start >= end) || (end > message->bytes))
		return -1;

	memcpy(message->data + start, data, end - start);

	return 0;
}


/*
	Get MSRP message from the list of incoming messages for a session
*/
msrp_message *msrp_message_get(msrp_session *session, char *messageid)
{
	if(!session || !messageid)
		return NULL;

	msrp_message *message = NULL;

	int j = 0;
	for(j = 0; j < MSRP_MSG_BUFFER; j++) {
		if(session->in_msg[j]) {
			if(!strcasecmp(session->in_msg[j]->messageid, messageid)) {
				message = session->in_msg[j];
				break;
			}
		}
	}

	return message;
}


/*
	Create a new chunk from a message
*/
msrp_chunk *msrp_chunk_new(msrp_message *message, char *transactionid, int start, int end)
{
	if(!message)
		return NULL;
	if(start > end)
		return NULL;

	msrp_chunk *chunk = calloc(1, sizeof(*chunk));
	if(!chunk)
		return NULL;

	chunk->content = message->content;

	if(transactionid) {	/* We're receiving a chunk */
		chunk->transactionid = calloc(strlen(transactionid)+1, sizeof(char));
		strcpy(chunk->transactionid, transactionid);
	} else {
		/* Generate random Message-ID for outgoing message */
		chunk->transactionid = calloc(9, sizeof(char));
		chunk->transactionid = random_string(chunk->transactionid, 9);
	}

	int bytes = end - start;
	chunk->start = start;
	chunk->end = end;
	chunk->bytes = message->bytes;
	if(bytes > 0)
		chunk->data = calloc(bytes+1, sizeof(char));
	else
		chunk->data = NULL;

	return chunk;
}


/*
	Enqueue a text message (SEND) or notification (REPORT) to the peer in this session
*/
int msrp_queue_text(int method, msrp_session *session, char *text, int reports, int status)
{
	if(!session) {
		local_events(MSRP_ERROR, "Invalid session, cannot queue text");
		return -1;
	}

	if(method == MSRP_SEND)
		status = 0;		/* SEND has no Status header field */
	else if(method == MSRP_REPORT) {
		if(status < 1)		/* Status header field is mandatory in REPORT */
			return -1;
		reports = 0;	/* REPORT has no Success-Report and Failure-Report header fields */
	} else
		return -1;	/* Only SEND and REPORT are supported, no AUTH for now */

	int total = 0;
	if(text) {
		total = strlen(text);
		if(total < 1) {
			local_events(MSRP_ERROR, "Text exists but is empty, cannot queue");
			return -1;
		}
	}

	msrp_message *message = msrp_message_new(NULL, MSRP_TEXT_PLAIN, total);
	if(msrp_message_setup(message, session, method, reports, status) < 0)
		return -1;
	if(total > 0) {		/* Sending empty SEND text messages is allowed */
		if(msrp_message_fill(message, text, 0, total) < 0) {
			local_events(MSRP_ERROR, "Error filling message, cannot queue text");
			return -1;
		}
	}

	return msrp_send_message(message);
}


/*
	Send a message, taking care of chunking if necessary
*/
int msrp_send_message(msrp_message *message)
{
	if(!message)
		return -1;
	if(!message->session)
		return -1;

	/* Take note of this message in the session */
	int j = 0;
	for(j = 0; j < MSRP_MSG_BUFFER; j++) {
		if(message->session->out_msg[j] == NULL) {
			message->session->out_msg[j] = message;
			break;
		}
		/* FIXME: handle errors */
	}

	int err = 0;
	int total = message->bytes;
	if(total < 2048) {	/* No chunking */
		err = msrp_send_chunk(message, 0, total);
		if(err < 0)
			local_events(MSRP_ERROR, "Error sending unique chunk (message %s)", message->messageid);
	} else {	/* Break the message in chunks */
		int start = 0, end = 0, left = 0;
		while(left > 0) {
			start = end;
			end = start + 2048;
			if(end > total)
				end = total;
			left = total - end;
			err = msrp_send_chunk(message, start, end);
			if(err < 0)
				local_events(MSRP_ERROR, "Error sending chunk (message %s)", message->messageid);
		}
	}

	return err;
}


/*
	Send a chunk (it might be the whole message)
*/
int msrp_send_chunk(msrp_message *message, int start, int end)
{
	if(!message)
		return -1;
	if(!message->session || !message->messageid)
		return -1;
	if((start < 0) || (end < 0) || (start > end))
		return -1;

	msrp_peer *from = message->session->from;
	msrp_peer *to = message->session->to;
	if(!from || !to)
		return -1;

	if(to->fd < 1) {
		local_events(MSRP_ERROR, "Error sending message, invalid file descriptor");
		return -1;
	}

/* 	int transport = message->session->transport;*/

	/* Generate random Transaction-ID */
	char *transactionid = calloc(9, sizeof(char));
	transactionid = random_string(transactionid, 9);

	char buffer[2600];
	memset(buffer, 0, 2600);
	int res = 0;
	res = msrp_add_request_line(buffer, transactionid, message->method);
	/* TODO from and to must consider the context, which is ignored for now */
	res |= msrp_add_topath_line(buffer, msrp_peer_get_path(to));
	res |= msrp_add_frompath_line(buffer, msrp_peer_get_path(from));
	res |= msrp_add_messageid_line(buffer, message->messageid);
	res |= msrp_add_byterange_line(buffer, start + 1, end, message->bytes);
	res |= msrp_add_reports_line(buffer, message->reports);
	res |= msrp_add_content_line(buffer, message->content);
	res |= msrp_add_status_line(buffer, message->status);
	res |= msrp_add_empty_line(buffer);
	res |= msrp_add_body(buffer, message->data, start, end);
	res |= msrp_add_end_line(buffer, transactionid,
		(end == message->bytes) ? MSRP_LAST_CHUNK : MSRP_MID_CHUNK);

	if(res < 0) {
		local_events(MSRP_ERROR, "Error building chunk (message %s, transaction %s)",
					message->messageid, transactionid);
		local_events(MSRP_ERROR, "Partial buffer:\n ###\n%s\n ###\n", buffer);
		return -1;
	}

	int total = strlen(buffer);
	int err = send(to->fd, buffer, total, 0);
	if(err < 0) {
		local_events(MSRP_ERROR, "Error sending chunk (message %s, transaction %s)",
					message->messageid, transactionid);
		return -1;
	}
	local_events(MSRP_LOG, "Chunk (message %s, transaction %s) sent (%d bytes)",
				message->messageid, transactionid, err);

	return 0;
}


/*
	Automated reply (OK or error) to a received chunk/message
*/
int msrp_send_reply(msrp_message *message, char *transactionid, int start, int end, int code)
{
	if(!message || !transactionid || (code < 200))
		return -1;
	if(!message->session)
		return -1;

// 	int transport = chunk->message->session->transport;

	msrp_peer *from = message->session->from;
	msrp_peer *to = message->session->to;
	if(!from || !to)
		return -1;

	char buffer[500];
	memset(buffer, 0, 500);
	int res = 0;
	res = msrp_add_request_line(buffer, transactionid, code);
	/* TODO from and to must consider the context, which is ignored for now */
	res |= msrp_add_topath_line(buffer, msrp_peer_get_path(to));
	res |= msrp_add_frompath_line(buffer, msrp_peer_get_path(from));
	res |= msrp_add_byterange_line(buffer, start, end, message->bytes);
	res |= msrp_add_end_line(buffer, transactionid, MSRP_LAST_CHUNK);

	if(res < 0) {
		local_events(MSRP_ERROR, "Error building reply (message %s, transaction %s)",
					message->messageid, transactionid);
		return -1;
	}

	int total = strlen(buffer);
	int err = send(to->fd, buffer, total, 0);
	if(err < 0) {
		local_events(MSRP_ERROR, "Error sending reply (message %s, transaction %s)",
					message->messageid, transactionid);
		return -1;
	}
	local_events(MSRP_LOG, "Reply (message %s, transaction %s) sent (%d bytes)",
				message->messageid, transactionid, err);

	return 0;
}


/*
	Add a line to the buffer according to the printf-like format string
*/
int msrp_add_line(char *buffer, char *format, ...)
{
	if(!buffer)
		return -1;

	va_list ap;
	va_start(ap, format);

	if(format) {
		char line[200];
		vsprintf(line, format, ap);
		strcat(buffer, line);
	}
	strcat(buffer, "\r\n");
	va_end(ap);

	return 0;
}

int msrp_add_request_line(char *buffer, char *transactionid, int method)
{
	if(!buffer || !transactionid)
		return -1;

	if(method == MSRP_SEND)
		return msrp_add_line(buffer, "MSRP %s SEND", transactionid);
	else if(method == MSRP_REPORT)
		return msrp_add_line(buffer, "MSRP %s REPORT", transactionid);
	else if(method == MSRP_AUTH)
		return msrp_add_line(buffer, "MSRP %s AUTH", transactionid);
	else {		/* Lookup for the response code */
		char *statuscode = NULL;
		int i = 0;
		while(1) {
			if(msrp_status_code[i].status < 0)
				break;
			if(msrp_status_code[i].status == method) {
				statuscode = msrp_status_code[i].desc;
				break;
			}
			i++;
		}
		if(!statuscode)
			return -1;
		return msrp_add_line(buffer, "MSRP %s %d %s", transactionid, method, statuscode);
	}
}

int msrp_add_topath_line(char *buffer, char *topath)
{
	if(!buffer || !topath)
		return -1;

	return msrp_add_line(buffer, "To-Path: %s", topath);
}

int msrp_add_frompath_line(char *buffer, char *frompath)
{
	if(!buffer || !frompath)
		return -1;

	return msrp_add_line(buffer, "From-Path: %s", frompath);
}

int msrp_add_messageid_line(char *buffer, char *messageid)
{
	if(!buffer || !messageid)
		return -1;

	return msrp_add_line(buffer, "Message-ID: %s", messageid);
}

int msrp_add_byterange_line(char *buffer, int start, int end, int total)
{
	if(!buffer)
		return -1;

	if((end < 0) || (total < 0))
		return msrp_add_line(buffer, "Byte-Range: %d-*/*", start);
	else
		return msrp_add_line(buffer, "Byte-Range: %d-%d/%d", start, end, total);
}

int msrp_add_reports_line(char *buffer, int reports)
{
	if(!buffer)
		return -1;

	int err = 0;
	if(reports & MSRP_SUCCESS_REPORT)
		err |= msrp_add_line(buffer, "Success-Report: yes");
	else if(reports & MSRP_SUCCESS_REPORT_PARTIAL)
		err |= msrp_add_line(buffer, "Success-Report: partial");

	if(reports & MSRP_FAILURE_REPORT)
		err |= msrp_add_line(buffer, "Failure-Report: yes");
	else if(reports & MSRP_FAILURE_REPORT_PARTIAL)
		err |= msrp_add_line(buffer, "Failure-Report: partial");

	return err;
}

int msrp_add_content_line(char *buffer, int content)
{
	if(!buffer)
		return -1;

	char *contenttype = NULL;
	int i = 0;
	while(1) {
		if(msrp_content_type[i].content < 0)
			break;
		if(msrp_content_type[i].content == content) {
			contenttype = msrp_content_type[i].desc;
			break;
		}
		i++;
	}
	if(!contenttype)
		return -1;

	return msrp_add_line(buffer, "Content-Type: %s", contenttype);
}

int msrp_add_status_line(char *buffer, int status)
{
	if(!buffer)
		return -1;
	if(!status)
		return 0;	/* No status to report */

	char *statuscode = NULL;
	int i = 0;
	while(1) {
		if(msrp_status_code[i].status < 0)
			break;
		if(msrp_status_code[i].status == status) {
			statuscode = msrp_status_code[i].desc;
			break;
		}
		i++;
	}
	if(!statuscode)
		return -1;

	return msrp_add_line(buffer, "Status: 000 %d %s", status, statuscode);
}

int msrp_add_empty_line(char *buffer)
{
	if(!buffer)
		return -1;

	return msrp_add_line(buffer, NULL);
}

int msrp_add_body(char *buffer, char *text, int start, int end)
{
	if(!buffer)
		return -1;

	if(!text)	/* If there's no text, just return */
		return 0;

	char *body = calloc(end - start + 1, sizeof(char));
	if(!body)
		return -1;
	strncpy(body, text + start, end - start);
	int res = msrp_add_line(buffer, "%s", body);
	free(body);
	return res;
}

int msrp_add_end_line(char *buffer, char *transactionid, int trailer)
{
	if(!buffer || !transactionid)
		return -1;

	char ch = '\0';
	if(trailer == MSRP_LAST_CHUNK)
		ch = '$';
	else if(trailer == MSRP_MID_CHUNK)
		ch = '+';
	else
		return -1;

	return msrp_add_line(buffer, "-------%s%c", transactionid, ch);
}





/*
	Parse a received buffer to get a chunk/message out of it
*/
int msrp_buffer_parse(int fd, msrp_session *session, char *buffer, int total)
{
	if(fd < 0) {
		local_events(MSRP_ERROR, "Invalid file descriptor for buffer to parse");
		return -1;
	}
	if(!session) {
		local_events(MSRP_ERROR, "Invalid session for buffer to parse");
		return -1;
	}
	if(!buffer || total < 1) {
		local_events(MSRP_ERROR, "Invalid buffer to parse (Call-ID %s)", session->callid);
		return -1;
	}

	const char *chunk = buffer;
	int bytes = total, method = -1, code = -1;
	char header[10], transactionid[20], request[10], desc[30];

	while(1) {	/* Loop in the buffer until we get all the chunks */
		local_events(MSRP_LOG, "%d bytes left to parse", bytes);
		if(bytes <= 1)
			break;

		/* Look for the beginning of the chunk/message, i.e. for the 'MSRP' string */
		const char *start = stristr((const char *)chunk, "msrp");
		if(!start) {	/* Invalid message */
			local_events(MSRP_ERROR, "Couldn't find starting 'MSRP'");
			return -1;
		}
		chunk = start;

		char line[100], *linedem = NULL;
		memset(line, 0, 100);
		linedem = strstr(chunk, "\r\n");
		if(!linedem) {
			local_events(MSRP_ERROR, "Couldn't find end of 'MSRP' line");
			return -1;
		}
		strncpy(line, chunk, linedem - chunk);
		/*
			Two possible cases now:
				1. MSRP transactionid SEND/REPORT/AUTH
				2. MSRP transactionid codenumber codedescription (e.g. 200 OK)
		*/
		memset(header, 0, 10);
		memset(transactionid, 0, 20);
		memset(request, 0, 10);
		memset(desc, 0, 30);
		if(!sscanf(line, "%s %s %s %s", header, transactionid, request, desc)) {
			if(!sscanf(line, "%s %s %s", header, transactionid, request)) {
				local_events(MSRP_ERROR, "Invalid header '%s'", line);
				return -1;	/* Invalid header */
			}
		}

		if(strcasecmp(header, "msrp")) {
			local_events(MSRP_ERROR, "Header is not 'MSRP' (%s)", header);
			return -1;	/* Invalid message */
		}
		if(!strcasecmp(request, "send"))
			method = MSRP_SEND;
		else if(!strcasecmp(request, "report"))
			method = MSRP_REPORT;
		else if(!strcasecmp(request, "auth"))
			method = MSRP_AUTH;
		else {
			code = atoi(request);
			if(!code) {
				local_events(MSRP_ERROR, "Invalid method '%s'", request);
				return -1;	/* Invalid message */
			}
		}

		/* Look for the end of the message using the Chunk-ID */
		char *end = strstr(linedem, transactionid);
		if(!end) {	/* FIXME */
			local_events(MSRP_ERROR, "Couldn't find end of the chunk/message (transaction %s)", transactionid);
			return -1;	/* No end? */
		}
		char *delimiter = end + strlen(transactionid);
		int last_chunk = 0;
		if(*delimiter == '$')
			last_chunk = 1;
		else if(*delimiter == '+')
			last_chunk = 0;
		else {	/* Delimiter # not implemented yet */
			local_events(MSRP_ERROR, "Unimplemented trailer char '%c'", *delimiter);
			return -1;
		}
		delimiter = delimiter + 2;	/* Skip the trailer \r\n */
		bytes -= delimiter - start;	/* Update the overall number of bytes */

		/* Payload starts after an empty line */
		char *payload = strstr(chunk, "\r\n\r\n");
		if(payload > delimiter)
			break;
		/* Parse the other lines between the header and the payload */
		linedem++;
		char *chunkline = linedem;
		char *separator = NULL;

		char attribute[20], value[100];
		char frompath[100], topath[100], messageid[20];
		int br_start = 0, br_end = 0, br_total = 0,
			content = -1, reports = 0, status = 0;

		while(1) {
			memset(attribute, 0, 20);
			memset(value, 0, 100);
			chunkline++;			/* Skip the previous \r\n */
			if(chunkline >= payload)	/* Stop if we reached the payload */
				break;
			linedem = strstr(chunkline, "\r\n");
			if(!linedem || (linedem > payload))
				break;
			strncpy(line, chunkline, linedem - chunkline);
			/* Attributes and values are separated by a semicolon */
			separator = strstr(chunkline, ":");
			if(!separator || (separator > linedem))
				break;
			/* Copy the attribute name */
			strncpy(attribute, chunkline, separator - chunkline);
			/* Skip the blanks after the semicolon... */
			separator++;
			while(*separator && (*separator < 33) && (separator < linedem))
				separator++;
			if(!separator || (separator > linedem))
				break;
			/* ...and copy the value(s) */
			strncpy(value, separator, linedem - separator);
			/* Now parse the attribute accordingly */
			if(!strcasecmp(attribute, "from-path")) {
				memset(frompath, 0, 100);
				strcpy(frompath, value);
			} else if(!strcasecmp(attribute, "to-path")) {
				memset(topath, 0, 100);
				strcpy(topath, value);
			} else if(!strcasecmp(attribute, "message-id")) {
				memset(messageid, 0, 20);
				strcpy(messageid, value);
			} else if(!strcasecmp(attribute, "byte-range")) {
				if(sscanf(value, "%d-%d/%d", &br_start, &br_end, &br_total)) {
					if(br_end == 0) {	/* Check if it is '*' */
						separator = strstr(separator, "-*");
						if(separator && (separator <= linedem))
							br_end = br_total;
					}
				}
			} else if(!strcasecmp(attribute, "content-type")) {
				int i = 0;
				while(1) {
					if(msrp_content_type[i].content < 0)
						break;
					if(!strcasecmp(msrp_content_type[i].desc, value)) {
						content = msrp_content_type[i].content;
						break;
					}
					i++;
				}
			} else if(!strcasecmp(attribute, "success-report")) {
				if(!strcasecmp(value, "yes"))
					reports |= MSRP_SUCCESS_REPORT;
				else if(!strcasecmp(value, "partial"))
					reports |= MSRP_SUCCESS_REPORT_PARTIAL;
			} else if(!strcasecmp(attribute, "failure-report")) {
				if(!strcasecmp(value, "yes"))
					reports |= MSRP_FAILURE_REPORT;
				else if(!strcasecmp(value, "partial"))
					reports |= MSRP_FAILURE_REPORT_PARTIAL;
			} else if(!strcasecmp(attribute, "status")) {
				sscanf(value, "%*s %d %*s", &status);	/* FIXME */
			}
			/* Pass to the following line */
			linedem++;
			chunkline = linedem;
		}
		payload += 4;
		if((method == MSRP_SEND) || (method == MSRP_REPORT) || (method == MSRP_AUTH)) {
			/* Now check if a message with this Message-ID exists in this session */
			msrp_message *message = msrp_message_get(session, messageid);
			if(!message) {	/* Create a new message */
				message = msrp_message_new(messageid, content, br_total);
				if(!message) {
					local_events(MSRP_ERROR, "Couldn't create new message (ID %s) from incoming buffer", messageid);
					return -1;
				}
				local_events(MSRP_LOG, "New message '%s' created from incoming buffer", messageid);
				if(msrp_message_setup(message, session, method, reports, status) < 0) {
					local_events(MSRP_ERROR, "Couldn't setup the new message '%s'", messageid);
					return -1;
				}
				local_events(MSRP_LOG, "New message '%s' setup completed", messageid);
			}
			/* TODO check consistency (values in chunk and values in message) */
			/* Get data from the chunk and put it in the right place in the overall buffer */
			if(msrp_message_fill(message, payload, br_start - 1, br_end) < 0)
				local_events(MSRP_ERROR, "Couldn't fill buffer in message '%s'", messageid);
			else
				local_events(MSRP_LOG, "Buffer filled in message '%s'", messageid);

			/* FIXME check better message for automated replies (e.g. ACKs) */
			msrp_peer *to = message->session->to;
			if((to->rights == MSRP_SENDRECV) || (to->rights == MSRP_SENDONLY)) {
				msrp_send_reply(message, transactionid, br_start, br_end, 200);

				if(session->type == MSRP_ENDPOINT) {
					/*
						FIXME: message should be sent to callback only if complete
							(i.e. check if all chunks have been received)
							besides, if we received MIME content, decode from
							base64 first?
					*/
					MsrpEndpoint *endpoint = (MsrpEndpoint *)session->session;
					if(endpoint) {
						local_events(MSRP_LOG, "Received text: %s", (char *)message->data);
						if(method == MSRP_SEND)
							local_ep_callback(MSRP_INCOMING_SEND, endpoint, content, message->data, message->bytes);
						else if(method == MSRP_REPORT)
							local_ep_callback(MSRP_INCOMING_REPORT, endpoint, content, message->data, message->bytes);
					}
				} else if(session->type == MSRP_SWITCH) {
					/* Forward received message to all other users in the conference */
					if(method != MSRP_SEND)
						continue;
					msrp_conference *conf = (msrp_conference *)session->session;
					msrp_conference_forward_message(conf, fd, session, message->data, message->bytes);
				}
			} else	/* Unauthorized */
				msrp_send_reply(message, transactionid, br_start, br_end, 403);
		} else { /* It is an automated reply */
			char *statuscode = NULL;
			int i = 0;
			while(1) {
				if(msrp_status_code[i].status < 0)
					break;
				if(msrp_status_code[i].status == code) {
					statuscode = msrp_status_code[i].desc;
					break;
				}
				i++;
			}
			local_events(MSRP_LOG, "Received a %d %s", code, statuscode ? statuscode : "??");
			MsrpEndpoint *endpoint = (MsrpEndpoint *)session->session;
			if(endpoint)
				local_ep_callback(code, endpoint, MSRP_TEXT_PLAIN, statuscode, strlen(statuscode));
			/* TODO check what else do with received ACKs */
		}
		chunk = start + bytes;		/* Pass to the next message, if present */
	}

	return 0;
}

/*
	Extract only the 'From-Path' line, to identify senders
*/
char *msrp_buffer_extract_sessionid(char *buffer, int bytes)
{
	if(!buffer && (bytes < 1))
		return NULL;

	const char *pnt = stristr(buffer, "from-path:");
	if(!pnt)
		return NULL;
	pnt = stristr(pnt, "//");
	if(!pnt)
		return NULL;
	pnt = stristr(pnt, ":");
	if(!pnt)
		return NULL;
	pnt = stristr(pnt, "/");
	if(!pnt)
		return NULL;
	pnt++;
	const char *end = stristr(pnt, ";tcp");
	if(!end)
		return NULL;

	char *sessionid = calloc(end - pnt + 1, sizeof(char));
	strncpy(sessionid, pnt, end - pnt);

	local_events(MSRP_LOG, "Extracted sessionid: %s (%d)", sessionid, strlen(sessionid));
	return sessionid;
}
