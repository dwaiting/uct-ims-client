/*
  The UCT IMS Client
  Copyright (C) 2006 - University of Cape Town
  David Waiting <david@crg.ee.uct.ac.za>
  Richard Good <rgood@crg.ee.uct.ac.za>

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/


#include "includes.h"
#include "common_exosip_event_handler.h"

#include <gst/controller/gstcontroller.h>


GList *call_list = NULL;


int find_call(int did, Call **ca)
{
	GList *li;
	Call *current;
	
	for(li = call_list; li != NULL; li = li->next)
	{
		current = (Call *)li->data;

		if (current->did == did)
		{
			*ca = li->data;
			return 0;
		}
	}

	return -1;
}

int find_im_call_by_uri(char *uri, Call **ca)
{
	GList *li;
	Call *current;
	
	for(li = call_list; li != NULL; li = li->next)
	{
		current = (Call *)li->data;

		printf("Looking for: %s\n", uri);
		printf("Found:       %s\n", current->to_uri);

		/* 
			Look for a session with the same To/From URI and that supports IM 
		*/
		if ((current->caller == 1) && !strcmp(current->to_uri, uri) && (current->im_supported))
		{
			*ca = li->data;
			return 0;
		}
		else if ((current->caller == 0) && !strcmp(current->from_uri, uri) && (current->im_supported))
		{
			*ca = li->data;
			return 0;
		}
	}

	return -1;
}


int find_call_by_cid(int cid, Call **ca)
{
	GList *li;
	Call *current;
	
	for(li = call_list; li != NULL; li = li->next)
	{
		current = (Call *)li->data;

		if (current->cid == cid)
		{
			*ca = li->data;
			return 0;
		}
	}

	return -1;
}



void add_call(Call **ca)
{
	Preferences *pref = client->pref;

	*ca = (Call *)malloc(sizeof(Call));

	if (*ca == NULL)
	{
		printf("Could not allocate memory\n");
    		return ;
	}

	memset (*ca, 0, sizeof (Call));

	// printf("Creating new pipelines\n");
	(*ca)->ringingPipeline = NULL;
	(*ca)->videoTxPipeline = NULL;
	(*ca)->videoRxPipeline = NULL;
	(*ca)->audioTxPipeline= NULL;
	(*ca)->audioRxPipeline= NULL;
	(*ca)->iptvVideoPipeline= NULL;

	(*ca)->audio_supported = 0;
	(*ca)->video_supported = 0;
	(*ca)->im_supported = 0;

	(*ca)->audio_codec = 0;
	(*ca)->qos_confirm = 0;
	(*ca)->media_negotiated = 0;
	(*ca)->content_indirected = 0;
	strcpy((*ca)->local_ip, pref->local_audio_ip);
	(*ca)->local_audio_port = pref->local_audio_port;
	(*ca)->local_video_port = pref->local_video_port;

	call_list = g_list_append(call_list, (gpointer)*ca);
}


void delete_call(int did)
{
	GList *li; 	/* an iterator */
	Call *current;
	
	for(li = call_list; li != NULL; li = li->next)
	{
		current = (Call *)li->data;

		if (current->did == did)
		{
			call_list = g_list_remove(call_list, li->data);
			break;
		}
	}
}


void print_calls()
{
	GList *li; 	/* an iterator */
	Call *current;
	
	for(li = call_list; li != NULL; li = li->next)
	{
		current = (Call *)li->data;

		printf("Call %d  To: %s  From: %s \n", current->did, current->to_uri, current->from_uri);
	}
}

/* 
	Not really used 
	Rather use ims_process_released_call

*/
void call_released(eXosip_event_t *je)
{
	Preferences *pref = client->pref;
	Call *ca;

	printf("Using call released!\n");

	if (find_call(current_dialog_id, &ca) < 0)
	{
		g_warning("no current dialogs");
		return ;
	}


	set_display("Call released");

	printf("Call ID:  %d   Event ID:  %d   Current Dialog ID: %d  Event Dialog ID: %d\n", ca->cid, je->cid, current_dialog_id, je->did);

	/* check if this is the current call and end it */
	if ((ca->cid == je->cid) || (current_dialog_id == je->did))
	{

		/* Destroy all media pipelines */
		destroyAudioTxPipeline(ca);
		destroyAudioRxPipeline(ca);

		destroyRingingPipeline(ca);
		destroyVideoTxPipeline(ca);
		destroyVideoRxPipeline(ca);

		destroyBackgroundVideoPipeline();

		destroyIptvVideoPipeline();

		/*destroy rtsp session if exists*/
//		if(vod_window_open == 1)
	//	{
		//	rtsp_end_session();
		//}
		/*destroy msrp session if exists*/
		/*
		if(ca->remote_msrp_port && pref->session_im_enabled)
		{
			endpointmsrp_end_session(0);
		}*/
// 		if(im_window_open == 1)
// 		{
// 			gtk_widget_destroy(GTK_WIDGET(im_window));
// 			im_window_open = 0;
// 			num_im_tabs=0;
// 		}

		if(msrp_endpoint_destroy(ca->msrp_endpoint) < 0)
		{
			g_warning("MSRP Error: Couldn't destroy endpoint...\n");
		}

		ca->call_is_active = 0;

		printf("Deleting call\n");
		delete_call(je->did);
		
		state = IDLE;

	}


 }




void common_process_info(eXosip_event_t *je)
{

	osip_body_t *info_body;
	char display[500];
	osip_body_init(&info_body);
	osip_message_get_body(je->request, 0, &info_body);

	/* send 200ok */
	osip_message_t *answer;
	eXosip_call_build_answer(je->tid, 200, &answer);
	eXosip_call_send_answer(je->tid, 200, answer);
	

	osip_content_type_t *content_type;
	content_type = osip_message_get_content_type(je->request);

	if(strcmp(content_type->type, "application/dtmf-relay"))
	{
		char *c;
		c = strstr(info_body->body, "=") + 1;

		sprintf(display, "Received DMTF tone: %s", c);
		// set_display(display);
	}

}

