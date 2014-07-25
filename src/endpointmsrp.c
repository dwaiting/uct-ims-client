#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "includes.h"
#include "endpointmsrp.h"
#include "msrp.h"

MsrpEndpoint* endpointmsrp_create_endpoint(int call_id, char *address, int port, int active)
{
 
	//destroy existing sessions
	// endpointmsrp_end_session(0);	

	msrp_ep_callback(callback_msrp);

	MsrpEndpoint *endpoint = msrp_endpoint_new();
	if(!endpoint) {
		g_warning("MSRP Error: creating new endpoint...\n");
		return NULL;
	}
	msrp_destroyed = 0;
	
	//set call ID and label 
	//HACK LABEL is not used and set to any number (11 for now as we know this works)
	char call_id_number[50];
	sprintf(call_id_number,"%i",call_id);
	msrp_endpoint_set_callid(endpoint, call_id_number);
	msrp_endpoint_set_label(endpoint, 11);

	/* Create a new peer for us in the endpoint */
	int content = MSRP_TEXT_PLAIN | MSRP_TEXT_HTML;
	int flags = MSRP_OVER_TCP;
	if(active == 1)
	{
		flags |= MSRP_ACTIVE;
	}
	else
	{
		flags |= MSRP_PASSIVE;
	}
	if(msrp_endpoint_set_from(endpoint, address, port, content, flags, MSRP_SENDRECV) < 0) {
		g_warning("MSRP Error: creating new peer...\n");
		return NULL;
	}
	return endpoint;
}

int send_msrp_message(MsrpEndpoint *msrp_endpoint, char *message)
{
	if(msrp_send_text(msrp_endpoint, message, 0) < 0)
	{
				g_warning("MSRP Error: sending text...\n");
				return 0;
	}
	return 1;
}

int endpointmsrp_start_session(MsrpEndpoint *msrp_endpoint, char *path, int active)
{
	int content = MSRP_TEXT_PLAIN | MSRP_TEXT_HTML;
	int flags = MSRP_OVER_TCP;
	if(active == 1)
	{
		flags |= MSRP_ACTIVE;
	}
	else
	{
		flags |= MSRP_PASSIVE;
	}
	if(msrp_endpoint)
	{
		if(msrp_endpoint_set_to(msrp_endpoint, path, content, flags, MSRP_SENDRECV) < 0) {
			g_warning("MSRP Error: creating new %s 'To' peer...\n", active ? "active" : "passive" );
			return 0;
		}
	}
	return 1;
	
}

/* The callback to receive incoming messages from the MSRP library */
void callback_msrp(int event, MsrpEndpoint *endpoint, int content, void *data, int bytes)
{
	int debug = 0;
	char* peerdisplay ="";
	switch(event) {
		case MSRP_INCOMING_SEND:
		if(content == MSRP_TEXT_PLAIN) {
		
			printf("Received MSRP message: %s\n", (char*)data);

			ims_process_message(endpoint, (char*)data);

			
			eXosip_event_t *null_event = NULL;
			if(local_msrp_endpoint)
			{
				
		
				/* Receive our messages here */
				// ims_start_im_session(null_event, (char *)data);
			}

			
		}
		break;
		
 		default:	/* Here you can handle what to do with code responses (e.g. event=403 --> Forbidden) */
 			//if(event == 403)
 			//	printf("\n *** '403 Forbidden' in reply to our message!\n");
 			break;
	}
	//fflush(stdout);
}

/* The callback to receive incoming events from the library */
void events_msrp(int event, void *info)
{
	int debug = 0;
	if(!debug || (event == MSRP_NONE))
		return;

	/* Debug text notifications */
	char *head = NULL;
	if(event == MSRP_LOG)
		head = "MSRP_LOG";
	else if(event == MSRP_ERROR)
		head = "MSRP_ERROR";
	else
		return;

	//printf("[%s] %s\n", head ? head : "??", info ? (char *)info : "??");
}


void endpointmsrp_end_session(int active)
{
	
	if(msrp_destroyed == 0)
	{
		//if(active == 1)
		//{
		//	msrp_send_text(local_msrp_endpoint, " ", 0);
		//}
		if(msrp_endpoint_destroy(local_msrp_endpoint) < 0)
		{
			g_warning("MSRP Error: Couldn't destroy endpoint...\n");
		}
		else
		{
			msrp_destroyed = 1;
		}
	}
	//also hide im_window
	if(im_window_open == 1)
	{
		gtk_widget_destroy(GTK_WIDGET(im_window));
		im_window_open = 0;
		num_im_tabs=0;
	}
}
