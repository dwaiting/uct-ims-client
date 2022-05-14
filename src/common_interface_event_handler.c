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
#include "common_interface_event_handler.h"



void terminate_call()
{
	Preferences *pref = client->pref;
	Call *ca;

	if (find_call(current_dialog_id, &ca) == 0)
	{
		
		ca->call_is_active = 0;

		/* Destroy all media pipelines */
		destroyRingingPipeline(ca);
		destroyVideoTxPipeline(ca);
		destroyVideoRxPipeline(ca);

		destroyAudioTxPipeline(ca);
		destroyAudioRxPipeline(ca);

		destroyBackgroundVideoPipeline();

		destroyIptvVideoPipeline(ca);
	
		/*destroy rtsp session if exists*/
	//	if(vod_window_open == 1)
		//{
			//rtsp_end_session();
		//}
		/*destroy msrp session if exists*/
		
/*
		if(ca->remote_msrp_port && pref->session_im_enabled)
		{
			endpointmsrp_end_session(1);
		}
		if(im_window_open == 1)
		{
			gtk_widget_destroy(GTK_WIDGET(im_window));
			im_window_open = 0;
			num_im_tabs=0;
		}
		*/


		if (ca->im_supported)
		{
			//if(ca->caller == 1)
			//{
			//	msrp_send_text(ca->msrp_endpoint, " ", 0);
			//}

			if(msrp_endpoint_destroy(ca->msrp_endpoint) < 0)
			{
				g_warning("MSRP Error: Couldn't destroy endpoint... (Terminate Call)\n");
			}
			else
				printf("Destroyed MSRP endpoint\n");
		}

	
		eXosip_lock (context_eXosip);
		eXosip_call_terminate(context_eXosip, ca->cid, ca->did);
		eXosip_unlock (context_eXosip);

		if (state == LOCAL_RINGING)
			imsua_set_message_display("603 Decline (Invite)", 1);
		else if (state == REMOTE_RINGING)
			imsua_set_message_display("CANCEL", 1);
		else
			imsua_set_message_display("BYE", 1);

		/* Delete call from our list of active dialogs */
		delete_call(current_dialog_id);

		set_display("Call Released");	

	}
	else
		g_warning("Can't find call to terminate");

	state = IDLE;

}


/* 
	Reject an incoming call - use terminate_call instead
*/
void reject_call()
{
	Call *ca;

	if (find_call(current_dialog_id, &ca) == 0)
	{

		if ((state == LOCAL_RINGING) || (state == REMOTE_RINGING))
			destroyRingingPipeline(ca);

		ca->call_is_active = 0;
		
		eXosip_lock(context_eXosip);
		eXosip_call_terminate(context_eXosip, ca->cid, ca->did);
		eXosip_unlock(context_eXosip);

		set_display("Call rejected");

		delete_call(ca->did);
		state = IDLE;
	}
	else
	{
		g_warning("Can't find a call to reject");
	}
}	


/*
void common_start_im_session(const gchar *chat_uri_entry)
{
	if(num_im_tabs == 0)
	{
		num_im_tabs = 1;
		GtkWidget *im_tab_label = lookup_widget(GTK_WIDGET(im_window), "im_tab_1");
		GtkWidget *im_text = lookup_widget(GTK_WIDGET(im_window), "im_text_view");
		// Store pointers to all widgets, for use by lookup_widget().
		GLADE_HOOKUP_OBJECT_NO_REF (GTK_WIDGET(im_window), GTK_WIDGET(im_text), chat_uri_entry);
		gtk_label_set_text(GTK_LABEL(im_tab_label),chat_uri_entry);
	}
	else
	{
		GtkWidget *im_label = gtk_label_new(chat_uri_entry);
		GtkWidget *notebook = lookup_widget(GTK_WIDGET(im_window), "im_notebook");
						
		int i = 0;
		int found_tab = 0;
		GtkWidget *im_tab = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook),i);
		GtkWidget *im_tab_label = gtk_notebook_get_tab_label(GTK_NOTEBOOK(notebook),im_tab);
		for(i = 0; i < num_im_tabs; i++)
		{
			
			const char *tab_label = gtk_label_get_text(GTK_LABEL(gtk_notebook_get_tab_label(GTK_NOTEBOOK(notebook),gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook),i))));
			if(strcmp(chat_uri_entry,tab_label) == 0)
			{
				found_tab = 1;
				break;
			}
		}	
		
		if(found_tab == 1)
		{
			gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook),i);
		}
		else
		{
			GtkWidget *im_text = gtk_text_view_new();
			// Store pointers to all widgets, for use by lookup_widget(). 
  			GLADE_HOOKUP_OBJECT_NO_REF (GTK_WIDGET(im_window), GTK_WIDGET(im_text), chat_uri_entry);
						
			gtk_text_view_set_editable(GTK_TEXT_VIEW(im_text),FALSE);
			gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(im_text),FALSE);
			
			GtkWidget *im_label = gtk_label_new(chat_uri_entry);
			gtk_notebook_append_page(GTK_NOTEBOOK(notebook),GTK_WIDGET(im_text),GTK_WIDGET(im_label));
			gtk_widget_show (GTK_WIDGET(im_text));
			num_im_tabs++;
			gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook),num_im_tabs-1);
		}
		
	}
	
}
*/

/*

void common_send_dtmf(int val)
{
	char body[100];
	char display[500];

	Call *ca; // = (Call*)malloc(sizeof(Call));
//	ca = find_call(current_dialog_id);

	if (find_call(current_dialog_id, &ca) <  0)
		return;

	// if (ca == NULL)
	//	return ;

	sprintf(body, "Signal=%c\r\nDuration=250\r\n", dtmf_tab[val]);

	osip_message_t *request;
	int r;

	eXosip_lock(context_eXosip);
	r = eXosip_call_build_info(context_eXosip, ca->did, &request);
	eXosip_unlock(context_eXosip);

	if (r < 0)
		return ;
	
	osip_message_set_content_type (request, "application/dtmf-relay");
	osip_message_set_body (request, body, strlen(body));

	eXosip_lock();
	eXosip_call_send_request(ca->did, request);
	eXosip_unlock();

	sprintf(display, "Sent DTMF %c", dtmf_tab[val]);
	set_display(display);

}
*/

