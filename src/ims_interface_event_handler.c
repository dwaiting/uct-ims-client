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
#include "ims_interface_event_handler.h"


#define GLADE_HOOKUP_OBJECT(component,widget,name) \
  g_object_set_data_full (G_OBJECT (component), name, \
    gtk_widget_ref (widget), (GDestroyNotify) gtk_widget_unref)

#define GLADE_HOOKUP_OBJECT_NO_REF(component,widget,name) \
  g_object_set_data (G_OBJECT (component), name, widget)


/*
	Send an INVITE message
		uri_entry - the destination URI
		session_type - 	0 = normal call
				1 = IM session
*/
int ims_call_initiate(char *uri_entry, int session_type)
{

	Preferences *pref = client->pref;

	// the invite message
	osip_message_t *invite = NULL;

	char display[500];
	// const gchar *uri_entry;
	char from_string[50];

	// get URI from address bar
	// uri_entry = gtk_entry_get_text(GTK_ENTRY(client->uri_entry));

	// create from header with name and public UI
	sprintf(from_string, "\"%s\" <%s>", pref->name, pref->impu);

	osip_route_t *route = NULL;
	osip_route_init(&route);

	// Check that we're not calling ourselves by mistake
	if (!strcmp(uri_entry, pref->impu))
	{
		set_display("Not allowed to call yourself");
		return ;
	}

	/* 
		Check for valid P-CSCF 
	*/
	if (osip_route_parse(route, add_sip_scheme(pref->pcscf)) < 0)
	{
		set_display("Invalid P-CSCF\n\nCorrect usage:\n  sip:pcscf.open-ims.test");
		return ;
	}

	/* 
		Check for valid public UI and destination URI
	*/
	if (strlen(pref->impu) < 6)
	{
		set_display("Invalid Public User Identity\n\nCheck Preferences");
		return ;
	}
	else if((strstr(pref->impu, sip_str) != pref->impu) && (strstr(pref->impu, sip_strs) != pref->impu))
	{
		set_display("Invalid Public User Identity\n\nCorrect usage sip:user@address.com");
		return ;
	}
	else if (eXosip_call_build_initial_invite (context_eXosip, &invite, uri_entry, from_string, add_lr_to_route(add_sip_scheme(pref->pcscf)), "IMS Call"))
	{
		set_display("Invalid Destination URI or PCSCF\n\nCheck both start with \"sip:\"");
		return ;
	}

	/* 
		Add the service routes learned during registration
	*/
	int j;
	for (j = num_service_routes; j > 0; j--)
	{
		osip_message_set_route(invite, ims_service_route[j-1]);
	}
	
	/*
		Support for RFC 3325 - Private Extensions to the Session Initiation Protocol (SIP)
		for Asserted Identity within Trusted Networks
	*/
	osip_message_set_header((osip_message_t *)invite,(const char *)"P-Preferred-Identity",from_string);

	/*
		Support for draft-drage-sipping-service-identification-01 - Extension for the Identification of Services
	*/
	osip_message_set_header((osip_message_t *)invite,(const char *)"P-Preferred-Service","urn:xxx:3gpp-service.ims.icsi.mmtel");

	osip_message_set_header((osip_message_t *)invite,(const char *)"Privacy","none");

	/*
		Support for RFC 3455 - Private Header (P-Header) Extensions to the Session Initiation
    		Protocol (SIP) for the 3rd-Generation Partnership Project (3GPP)
	*/
	osip_message_set_header((osip_message_t *)invite,(const char *)"P-Access-Network-Info",access_networks[pref->access_network]);

	/* 
		Require preconditions if QoS required
	*/
	if (pref->qos_strength != QOS_NONE)
		osip_message_set_require(invite, "precondition");

	osip_message_set_require(invite, "sec-agree");
	osip_message_set_proxy_require(invite, "sec-agree");
	osip_message_set_supported(invite, "100rel");

	osip_message_set_allow(invite, "INVITE, ACK, CANCEL, BYE, PRACK, UPDATE, REFER, MESSAGE");

	/* Create a new MSRP endpoint */
	if (pref->session_im_enabled)
	{
		printf("Creating MSRP endpoint\n");

		osip_call_id_t *call_id_header = osip_message_get_call_id (invite);
		char *call_id_str = osip_call_id_get_number(call_id_header);
		int call_id = atoi(call_id_str);

		if((client->msrp_endpoint = endpointmsrp_create_endpoint(call_id, pref->local_msrp_ip, pref->local_msrp_port, 1)) == NULL)
		{
			g_warning("MSRP Error: creating new endpoint...\n");
		}
	}

	/* Insert an SDP body */	
	sdp_complete_ims(NULL, &invite, NULL, session_type);

	// send the invite
	eXosip_lock (context_eXosip);
	int i = eXosip_call_send_initial_invite (context_eXosip, invite);
	eXosip_unlock (context_eXosip);

	/* 
		Start timer for delay tests
	*/
	gettimeofday(&start_time, NULL);

	if (i == 0)
	{
		set_display("Error sending IMS invite");
		return -1;
	}
	else
	{
		sprintf(display, "Sent IMS INVITE to\n%s", uri_entry);
		set_display(display);
		imsua_set_message_display("INVITE", 1);
		return 0;
	}
	
}



int ims_send_instant_message (char *uri, char *im_text_entry)
{

	Preferences *pref = client->pref;

	if ((client == NULL) || (uri == NULL) || (im_text_entry == NULL))
	{
		g_warning("Invalid parameters for ims_send_instant_message");
		return -1;
	}

	Call *ca;

	/* 
		Check if IM session already exists 
		If not, save the message in a buffer, try create an IM session and return to this method later
	*/
	if (!find_im_call_by_uri((char*)uri, &ca))
	{
		printf("IM: An IM session with this user already exists\n");

		printf("My MSRP fullpath: %s\n", msrp_endpoint_get_from_fullpath(ca->msrp_endpoint));

		send_msrp_message(ca->msrp_endpoint, im_text_entry);


		/* Update the display */
		char *uri_name;
		uri_name = strstr(pref->impu,":") + 1;
	
		gchar buf [10000]= "";
		gchar buf1 [10000]= "";

		strcat (buf, "me: ");
		
		//uses name if available, otherwise uses URI
		/*		
		if(strcmp(pref->name,"")==0)
		{
			strcat (buf,uri_name);
		}
		else
		{
			strcat (buf,pref->name);
		}
		*/

		GtkWidget *scrolled_window = gtk_notebook_get_nth_page(GTK_NOTEBOOK(client->im_notebook), gtk_notebook_get_current_page(GTK_NOTEBOOK(client->im_notebook)));
		GtkWidget *im_text_view = gtk_bin_get_child(GTK_BIN(scrolled_window));

		GtkTextBuffer *im_output_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW(im_text_view));


		GtkTextTagTable *tag_table = gtk_text_buffer_get_tag_table(im_output_buffer);
		if(gtk_text_tag_table_lookup(tag_table,"bold") == NULL)
		{
			// Tag with weight bold and tag name "bold"
			gtk_text_buffer_create_tag (im_output_buffer,"bold","weight", PANGO_WEIGHT_BOLD, NULL);
		}
		if(gtk_text_tag_table_lookup(tag_table,"font") == NULL)
		{
			// Tag with font fixed and tag name "font". 
			gtk_text_buffer_create_tag (im_output_buffer, "font", "font", "fixed", NULL);
		}
		if(gtk_text_tag_table_lookup(tag_table,"italic") == NULL)
		{
			// Tag with font fixed and tag name "italic". 
			gtk_text_buffer_create_tag (im_output_buffer, "italic", "style", PANGO_STYLE_ITALIC, NULL);
		}
		if(gtk_text_tag_table_lookup(tag_table,"red") == NULL)
		{
			// Tag with font fixed and tag name "italic". 
			gtk_text_buffer_create_tag (im_output_buffer, "red", "foreground", "darkred", NULL);
		}
		
		//if time stamp visible this prints the time the message was sent
		//if(time_stamps == VISIBLE)
		
		//{
	/*
		
			gchar buf2 [10000]= "";
			GtkTextIter start_message_iter1;
			gtk_text_buffer_get_end_iter(im_output_buffer, &start_message_iter1);
			GtkTextMark *start_message_mark1 = gtk_text_buffer_create_mark(im_output_buffer,"start_message_mark",&start_message_iter1,TRUE);
			strcat (buf2, "(");
			strcat (buf2, imsua_get_time());
			strcat (buf2, ") ");
			gtk_text_buffer_insert(im_output_buffer,&start_message_iter1,buf2,strlen(buf2));
			GtkTextIter end_message_iter1;
			gtk_text_buffer_get_end_iter(im_output_buffer, &end_message_iter1);
			gtk_text_buffer_get_iter_at_mark(im_output_buffer,&start_message_iter1,start_message_mark1);
			//Apply the RED tag to the selected text.
			gtk_text_buffer_apply_tag_by_name (im_output_buffer, "red", &start_message_iter1, &end_message_iter1);
	*/	

		//} 
		
		//This writes the sender portion of the message in bold for display on the screen
		GtkTextIter start_send_iter;
		gtk_text_buffer_get_end_iter(im_output_buffer, &start_send_iter);
		GtkTextMark *start_send_mark = gtk_text_buffer_create_mark(im_output_buffer,"start_send_mark",&start_send_iter,TRUE);
		gtk_text_buffer_insert(im_output_buffer,&start_send_iter,buf,strlen(buf));
		GtkTextIter end_send_iter;
		gtk_text_buffer_get_end_iter(im_output_buffer, &end_send_iter);	
		gtk_text_buffer_get_iter_at_mark(im_output_buffer,&start_send_iter,start_send_mark);
		//Apply the BOLD tag to the selected text.
		gtk_text_buffer_apply_tag_by_name (im_output_buffer, "bold", &start_send_iter, &end_send_iter);
		//Apply the RED tag to the selected text.
		gtk_text_buffer_apply_tag_by_name (im_output_buffer, "red", &start_send_iter, &end_send_iter);
	
		//This writes the message portion is normal font for display on the screen
		GtkTextIter start_message_iter;
		gtk_text_buffer_get_end_iter(im_output_buffer, &start_message_iter);
		GtkTextMark *start_message_mark = gtk_text_buffer_create_mark(im_output_buffer,"start_message_mark",&start_message_iter,TRUE);
		strcat (buf1, im_text_entry);
		strcat (buf1, "\n");
		gtk_text_buffer_insert(im_output_buffer,&start_message_iter,buf1,strlen(buf1));
		GtkTextIter end_message_iter;
		gtk_text_buffer_get_end_iter(im_output_buffer, &end_message_iter);
		gtk_text_buffer_get_iter_at_mark(im_output_buffer,&start_message_iter,start_message_mark);
		//Apply the normal FONT tag to the selected text.
		gtk_text_buffer_apply_tag_by_name (im_output_buffer, "font", &start_message_iter, &end_message_iter);
		
		//clears text input buffer
		// gtk_entry_set_text(GTK_ENTRY(im_text_entry),"");
		
		//scrolls the viewing window down
		GtkTextIter output_end_iter;
		gtk_text_buffer_get_end_iter(im_output_buffer, &output_end_iter);
		GtkTextMark *last_pos = last_pos = gtk_text_buffer_create_mark (im_output_buffer, "last_pos", &output_end_iter, FALSE);	
		gtk_text_view_scroll_mark_onscreen (GTK_TEXT_VIEW(im_text_view), last_pos);

	}	
	else	
	{
		printf("IM: Cannot find an existing IM session with this user. Creating a new session.\n");
		strcpy(client->im_buffer, im_text_entry);

		/* Start an IMS session with message component */		
		ims_call_initiate (uri, 1);

		return -1;

	}

	return 0;
}



/*
//see RFC 3994
void ims_send_instant_message_status (int status)
{
	
	Call *ca;

	//session based IM component exists
	if (find_call(current_dialog_id, &ca) >= 0 && ca->remote_msrp_port && pref->session_im_enabled) 
	{
		return ;
	}
	GtkWidget *notebook = lookup_widget(GTK_WIDGET(im_window), "im_notebook");
	GtkWidget *im_text_entry = lookup_widget(GTK_WIDGET(im_window), "im_text_input");
	GtkWidget *im_tab_label = gtk_notebook_get_tab_label(GTK_NOTEBOOK(notebook),gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook),gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook))));

	char status_message[4096];
	char from_string[50];
	int i;	
	osip_message_t *message;
	const gchar *uri_entry;
	uri_entry = gtk_label_get_text(GTK_LABEL(im_tab_label));

	if(status == 1)//ACTIVE
	{
		sprintf(status_message,
			"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
			"<isComposing xmlns=\"urn:ietf:params:xml:ns:im-iscomposing\"\n"
			"xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
			"xsi:schemaLocation=\"urn:ietf:params:xml:ns:im-composing iscomposing.xsd\">\n"
			"<state>active</state>\n"
			"<contenttype>text/plain</contenttype>\n"
			"</isComposing>");
	}
	else if (status == 0)	//IDLE
	{
		sprintf(status_message,
			"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
			"<isComposing xmlns=\"urn:ietf:params:xml:ns:im-iscomposing\"\n"
			"xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
			"xsi:schemaLocation=\"urn:ietf:params:xml:ns:im-composing iscomposing.xsd\">\n"
			"<state>idle</state>\n"
			"<contenttype>text/plain</contenttype>\n"
			"</isComposing>");	
	}

	
	if(strcmp(pref->name,"")==0)
	{
		sprintf(from_string, "<%s>", pref->impu);
	}
	else
	{
		sprintf(from_string, "\"%s\" <%s>", pref->name, pref->impu);
	}		
	
	osip_route_t *rt=NULL;
	osip_route_init(&rt);
	char *tmproute=NULL;
	if (osip_route_parse(rt,add_sip_scheme(pref->pcscf)) < 0)
	{
		set_display("Cannot send IM: Invalid PCSCF\n\nCorrect usage - sip:<uri>[:port]");
	}
	else
	{
		osip_uri_uparam_add(rt->url,osip_strdup("lr"),NULL);
		osip_route_to_str(rt,&tmproute);
		eXosip_lock (context_eXosip);
		i = eXosip_message_build_request (context_eXosip, &message, "MESSAGE", uri_entry, from_string, tmproute);
		eXosip_unlock (context_eXosip);
	
		// add the service routes
		int j;
		for (j = num_service_routes; j > 0; j--)
		{
			osip_message_set_route(message, ims_service_route[j-1]);
		}


		if (i != 0)
		{
			fprintf(stderr, "Invalid Destination URI");
		}
		else
		{				
			char tmp[10000];
			*/
			// get local IP address
			/*
			if(strcmp(media_interface, "Default") == 0)
			{
				eXosip_lock (context_eXosip);
				eXosip_guess_localip (context_eXosip, AF_INET, local_ip, 128);
				eXosip_unlock (context_eXosip);
			}
			else 
			{
				sprintf(local_ip,media_interface);
			}
			*/
/*
			snprintf (tmp, 10000,status_message, pref->local_audio_ip, pref->local_audio_ip);
			osip_message_set_body (message, tmp, strlen (tmp));;
			osip_message_set_content_type (message, "application/im-iscomposing+xml");
			
			osip_message_set_header((osip_message_t *)message,(const char *)"P-Preferred-Identity",from_string);
			
			// Support for RFC 3455

			switch(pref->access_network)
			{
				case 0 : osip_message_set_header((osip_message_t *)message,(const char *)"P-Access-Network-Info", "IEEE-802.11a");
				break;
				case 1 : osip_message_set_header((osip_message_t *)message,(const char *)"P-Access-Network-Info", "IEEE-802.11b");
				break;
				case 2 : osip_message_set_header((osip_message_t *)message,(const char *)"P-Access-Network-Info", "3GPP-GERAN");
				break;
				case 3 : osip_message_set_header((osip_message_t *)message,(const char *)"P-Access-Network-Info", "3GPP-UTRAN-FDD");
				break;
				case 4 : osip_message_set_header((osip_message_t *)message,(const char *)"P-Access-Network-Info", "3GPP-UTRAN-TDD");
				break;
				case 5 : osip_message_set_header((osip_message_t *)message,(const char *)"P-Access-Network-Info", "3GPP-CDMA2000");
				break;

			}
	
			eXosip_lock (context_eXosip);
			i = eXosip_message_send_request (context_eXosip, message);
			eXosip_unlock (context_eXosip);
	
	
			if (i = 0)
				fprintf(stderr, "Error sending message\n");
		}
	}
}

*/
/*

void ims_send_instant_message ()
{
	
	Call *ca;

	//session based IM component exists
	if (find_call(current_dialog_id, &ca) >= 0 && ca->remote_msrp_port && pref->session_im_enabled) 
	{
		GtkWidget *notebook = lookup_widget(GTK_WIDGET(im_window), "im_notebook");
		GtkWidget *im_text_entry = lookup_widget(GTK_WIDGET(im_window), "im_text_input");
		GtkWidget *im_output_text_view = lookup_widget(GTK_WIDGET(im_window), gtk_label_get_text(GTK_LABEL(gtk_notebook_get_tab_label(GTK_NOTEBOOK(notebook),gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook),gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)))))));
		GtkTextBuffer *im_output_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW(im_output_text_view));	
		
		const gchar *im_message;
		im_message = gtk_entry_get_text(GTK_ENTRY(im_text_entry));	
		if (send_msrp_message((char *)im_message) == 0)
		{
			fprintf(stderr, "Error sending message\n");
		}
		else
		{
			//writes the text to the IM output screen
			//this removes "sip:" or "sips:"from from_uri
			//It is used for displaying message sender 
			char *uri_name;
			uri_name = strstr(pref->impu,":") + 1;
		
			gchar buf [10000]= "";
			gchar buf1 [10000]= "";
			
			//uses name if available, otherwise uses URI
			if(strcmp(pref->name,"")==0)
			{
				strcat (buf,uri_name);
			}
			else
			{
				strcat (buf,pref->name);
			}
			
			GtkTextTagTable *tag_table = gtk_text_buffer_get_tag_table(im_output_buffer);
			if(gtk_text_tag_table_lookup(tag_table,"bold") == NULL)
			{
				// Tag with weight bold and tag name "bold"
				gtk_text_buffer_create_tag (im_output_buffer,"bold","weight", PANGO_WEIGHT_BOLD, NULL);
			}
			if(gtk_text_tag_table_lookup(tag_table,"font") == NULL)
			{
				// Tag with font fixed and tag name "font". 
				gtk_text_buffer_create_tag (im_output_buffer, "font", "font", "fixed", NULL);
			}
			if(gtk_text_tag_table_lookup(tag_table,"italic") == NULL)
			{
				// Tag with font fixed and tag name "italic". 
				gtk_text_buffer_create_tag (im_output_buffer, "italic", "style", PANGO_STYLE_ITALIC, NULL);
			}
			if(gtk_text_tag_table_lookup(tag_table,"red") == NULL)
			{
				// Tag with font fixed and tag name "italic". 
				gtk_text_buffer_create_tag (im_output_buffer, "red", "foreground", "darkred", NULL);
			}
			
			//if time stamp visible this prints the time the message was sent
			if(time_stamps == VISIBLE)
			{
				gchar buf2 [10000]= "";
				GtkTextIter start_message_iter;
				gtk_text_buffer_get_end_iter(im_output_buffer, &start_message_iter);
				GtkTextMark *start_message_mark = gtk_text_buffer_create_mark(im_output_buffer,"start_message_mark",&start_message_iter,TRUE);
				strcat (buf2, "(");
				strcat (buf2, imsua_get_time());
				strcat (buf2, ") ");
				gtk_text_buffer_insert(im_output_buffer,&start_message_iter,buf2,strlen(buf2));
				GtkTextIter end_message_iter;
				gtk_text_buffer_get_end_iter(im_output_buffer, &end_message_iter);
				gtk_text_buffer_get_iter_at_mark(im_output_buffer,&start_message_iter,start_message_mark);
				//Apply the RED tag to the selected text.
				gtk_text_buffer_apply_tag_by_name (im_output_buffer, "red", &start_message_iter, &end_message_iter);
			} 
			
			//This writes the sender portion of the message in bold for display on the screen
			GtkTextIter start_send_iter;
			gtk_text_buffer_get_end_iter(im_output_buffer, &start_send_iter);
			GtkTextMark *start_send_mark = gtk_text_buffer_create_mark(im_output_buffer,"start_send_mark",&start_send_iter,TRUE);
			strcat (buf, ": ");
			gtk_text_buffer_insert(im_output_buffer,&start_send_iter,buf,strlen(buf));
			GtkTextIter end_send_iter;
			gtk_text_buffer_get_end_iter(im_output_buffer, &end_send_iter);	
			gtk_text_buffer_get_iter_at_mark(im_output_buffer,&start_send_iter,start_send_mark);
			//Apply the BOLD tag to the selected text.
			gtk_text_buffer_apply_tag_by_name (im_output_buffer, "bold", &start_send_iter, &end_send_iter);
			//Apply the RED tag to the selected text.
			gtk_text_buffer_apply_tag_by_name (im_output_buffer, "red", &start_send_iter, &end_send_iter);
		
			//This writes the message portion is normal font for display on the screen
			GtkTextIter start_message_iter;
			gtk_text_buffer_get_end_iter(im_output_buffer, &start_message_iter);
			GtkTextMark *start_message_mark = gtk_text_buffer_create_mark(im_output_buffer,"start_message_mark",&start_message_iter,TRUE);
			strcat (buf1, im_message);
			strcat (buf1, "\n");
			gtk_text_buffer_insert(im_output_buffer,&start_message_iter,buf1,strlen(buf1));
			GtkTextIter end_message_iter;
			gtk_text_buffer_get_end_iter(im_output_buffer, &end_message_iter);
			gtk_text_buffer_get_iter_at_mark(im_output_buffer,&start_message_iter,start_message_mark);
			//Apply the normal FONT tag to the selected text.
			gtk_text_buffer_apply_tag_by_name (im_output_buffer, "font", &start_message_iter, &end_message_iter);
			
			//clears text input buffer
			gtk_entry_set_text(GTK_ENTRY(im_text_entry),"");
			
			//scrolls the viewing window down
			GtkTextIter output_end_iter;
			gtk_text_buffer_get_end_iter(im_output_buffer, &output_end_iter);
			GtkTextMark *last_pos = last_pos = gtk_text_buffer_create_mark (im_output_buffer, "last_pos", &output_end_iter, FALSE);	
			gtk_text_view_scroll_mark_onscreen (GTK_TEXT_VIEW(im_output_text_view), last_pos);
		}
		
		return ;	
	}	

	GtkWidget *notebook = lookup_widget(GTK_WIDGET(im_window), "im_notebook");
	GtkWidget *im_text_entry = lookup_widget(GTK_WIDGET(im_window), "im_text_input");
	GtkWidget *im_output_text_view = lookup_widget(GTK_WIDGET(im_window), gtk_label_get_text(GTK_LABEL(gtk_notebook_get_tab_label(GTK_NOTEBOOK(notebook),gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook),gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)))))));
	GtkTextBuffer *im_output_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW(im_output_text_view));	
	
	const gchar *im_message;
	im_message = gtk_entry_get_text(GTK_ENTRY(im_text_entry));	
	osip_message_t *message;
	int i;

	GtkWidget *im_tab_label = gtk_notebook_get_tab_label(GTK_NOTEBOOK(notebook),gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook),gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook))));
	
	const gchar *uri_entry;
	char from_string[50];
	uri_entry = gtk_label_get_text(GTK_LABEL(im_tab_label));
	
	if(strcmp(pref->name,"")==0)
	{
		sprintf(from_string, "<%s>", pref->impu);
	}
	else
	{
		sprintf(from_string, "\"%s\" <%s>", pref->name, pref->impu);
	}		
	
	osip_route_t *rt=NULL;
	osip_route_init(&rt);
	char *tmproute=NULL;
	if (osip_route_parse(rt,add_sip_scheme(pref->pcscf)) < 0)
	{
		set_display("Cannot send IM: Invalid PCSCF\n\nCorrect usage - sip:<uri>[:port]");
	}
	else
	{
		osip_uri_uparam_add(rt->url,osip_strdup("lr"),NULL);
		osip_route_to_str(rt,&tmproute);
		eXosip_lock (context_eXosip);
		i = eXosip_message_build_request (context_eXosip, &message, "MESSAGE", uri_entry, from_string, tmproute);
		eXosip_unlock (context_eXosip);
	
		// add the service routes
		int j;
		for (j = num_service_routes; j > 0; j--)
		{
			osip_message_set_route(message, ims_service_route[j-1]);
		}


		if (i != 0)
		{
			fprintf(stderr, "Invalid Destination URI");
		}
		else
		{				
			char tmp[10000];
			
*/

			// get local IP address
			/*
			if(strcmp(media_interface, "Default") == 0)
			{
				eXosip_lock (context_eXosip);
				eXosip_guess_lolip (context_eXosip, AF_INET, local_ip, 128);
				eXosip_unlock (context_eXosip);
			}
			else 
			{
				sprintf(local_ip,media_interface);
			}
			*/
/*
			snprintf (tmp, 10000,im_message, pref->local_audio_ip, pref->local_audio_ip);
			osip_message_set_body (message, tmp, strlen (tmp));;
			osip_message_set_content_type (message, "text/plain");
			
			osip_message_set_header((osip_message_t *)message,(const char *)"P-Preferred-Identity",from_string);
			
			// Support for RFC 3455

			switch(pref->access_network)
			{
				case 0 : osip_message_set_header((osip_message_t *)message,(const char *)"P-Access-Network-Info", "IEEE-802.11a");
				break;
				case 1 : osip_message_set_header((osip_message_t *)message,(const char *)"P-Access-Network-Info", "IEEE-802.11b");
				break;
				case 2 : osip_message_set_header((osip_message_t *)message,(const char *)"P-Access-Network-Info", "3GPP-GERAN");
				break;
				case 3 : osip_message_set_header((osip_message_t *)message,(const char *)"P-Access-Network-Info", "3GPP-UTRAN-FDD");
				break;
				case 4 : osip_message_set_header((osip_message_t *)message,(const char *)"P-Access-Network-Info", "3GPP-UTRAN-TDD");
				break;
				case 5 : osip_message_set_header((osip_message_t *)message,(const char *)"P-Access-Network-Info", "3GPP-CDMA2000");
				break;

			}
	
			eXosip_lock (context_eXosip);
			i = eXosip_message_send_request (context_eXosip, message);
			eXosip_unlock (context_eXosip);
	
	
			if (i = 0)
				fprintf(stderr, "Error sending message\n");
			else
			{
				// fprintf(stderr, "Sent message to %s\n", uri_entry);
				imsua_set_message_display("MESSAGE", 1);
			
				//writes the text to the IM output screen
				//this removes "sip:" or "sips:"from from_uri
				//It is used for displaying message sender 
				char *uri_name;
				uri_name = strstr(pref->impu,":") + 1;
			
				gchar buf [10000]= "";
				gchar buf1 [10000]= "";
				
				//uses name if available, otherwise uses URI
				if(strcmp(pref->name,"")==0)
				{
					strcat (buf,uri_name);
				}
				else
				{
					strcat (buf,pref->name);
				}
				
				GtkTextTagTable *tag_table = gtk_text_buffer_get_tag_table(im_output_buffer);
				if(gtk_text_tag_table_lookup(tag_table,"bold") == NULL)
				{
					// Tag with weight bold and tag name "bold"
					gtk_text_buffer_create_tag (im_output_buffer,"bold","weight", PANGO_WEIGHT_BOLD, NULL);
				}
				if(gtk_text_tag_table_lookup(tag_table,"font") == NULL)
				{
					// Tag with font fixed and tag name "font". 
					gtk_text_buffer_create_tag (im_output_buffer, "font", "font", "fixed", NULL);
				}
				if(gtk_text_tag_table_lookup(tag_table,"italic") == NULL)
				{
					// Tag with font fixed and tag name "italic". 
					gtk_text_buffer_create_tag (im_output_buffer, "italic", "style", PANGO_STYLE_ITALIC, NULL);
				}
				if(gtk_text_tag_table_lookup(tag_table,"red") == NULL)
				{
					// Tag with font fixed and tag name "italic". 
					gtk_text_buffer_create_tag (im_output_buffer, "red", "foreground", "darkred", NULL);
				}
				
				//if time stamp visible this prints the time the message was sent
				if(time_stamps == VISIBLE)
				{
					gchar buf2 [10000]= "";
					GtkTextIter start_message_iter;
					gtk_text_buffer_get_end_iter(im_output_buffer, &start_message_iter);
					GtkTextMark *start_message_mark = gtk_text_buffer_create_mark(im_output_buffer,"start_message_mark",&start_message_iter,TRUE);
					strcat (buf2, "(");
					strcat (buf2, imsua_get_time());
					strcat (buf2, ") ");
					gtk_text_buffer_insert(im_output_buffer,&start_message_iter,buf2,strlen(buf2));
					GtkTextIter end_message_iter;
					gtk_text_buffer_get_end_iter(im_output_buffer, &end_message_iter);
					gtk_text_buffer_get_iter_at_mark(im_output_buffer,&start_message_iter,start_message_mark);
					//Apply the RED tag to the selected text.
					gtk_text_buffer_apply_tag_by_name (im_output_buffer, "red", &start_message_iter, &end_message_iter);
				} 
				
				//This writes the sender portion of the message in bold for display on the screen
				GtkTextIter start_send_iter;
				gtk_text_buffer_get_end_iter(im_output_buffer, &start_send_iter);
				GtkTextMark *start_send_mark = gtk_text_buffer_create_mark(im_output_buffer,"start_send_mark",&start_send_iter,TRUE);
				strcat (buf, ": ");
				gtk_text_buffer_insert(im_output_buffer,&start_send_iter,buf,strlen(buf));
				GtkTextIter end_send_iter;
				gtk_text_buffer_get_end_iter(im_output_buffer, &end_send_iter);	
				gtk_text_buffer_get_iter_at_mark(im_output_buffer,&start_send_iter,start_send_mark);
				//Apply the BOLD tag to the selected text.
				gtk_text_buffer_apply_tag_by_name (im_output_buffer, "bold", &start_send_iter, &end_send_iter);
				//Apply the RED tag to the selected text.
				gtk_text_buffer_apply_tag_by_name (im_output_buffer, "red", &start_send_iter, &end_send_iter);
			
				//This writes the message portion is normal font for display on the screen
				GtkTextIter start_message_iter;
				gtk_text_buffer_get_end_iter(im_output_buffer, &start_message_iter);
				GtkTextMark *start_message_mark = gtk_text_buffer_create_mark(im_output_buffer,"start_message_mark",&start_message_iter,TRUE);
				strcat (buf1, im_message);
				strcat (buf1, "\n");
				gtk_text_buffer_insert(im_output_buffer,&start_message_iter,buf1,strlen(buf1));
				GtkTextIter end_message_iter;
				gtk_text_buffer_get_end_iter(im_output_buffer, &end_message_iter);
				gtk_text_buffer_get_iter_at_mark(im_output_buffer,&start_message_iter,start_message_mark);
				//Apply the normal FONT tag to the selected text.
				gtk_text_buffer_apply_tag_by_name (im_output_buffer, "font", &start_message_iter, &end_message_iter);
				
				//clears text input buffer
				gtk_entry_set_text(GTK_ENTRY(im_text_entry),"");
				
				//scrolls the viewing window down
				GtkTextIter output_end_iter;
				gtk_text_buffer_get_end_iter(im_output_buffer, &output_end_iter);
				GtkTextMark *last_pos = last_pos = gtk_text_buffer_create_mark (im_output_buffer, "last_pos", &output_end_iter, FALSE);	
				gtk_text_view_scroll_mark_onscreen (GTK_TEXT_VIEW(im_output_text_view), last_pos);
			}
		}
	}
}
*/

gint ims_send_register()
{
	char *uuid, gruu[256];
	Preferences *pref = client->pref;

	is_message_deregister = 0;
 	eXosip_lock(context_eXosip);
       	eXosip_automatic_action (context_eXosip);
	eXosip_clear_authentication_info(context_eXosip);
	eXosip_unlock(context_eXosip);

	/* Generate our UUID and GRUUU - this is appended to the contact header field later */
	uuid_t u;
	uuid_create(&u);
	uuid = get_uuid(u);
	sprintf(gruu, "\"<urn:uuid:%s>\"", uuid);
	
	char display[500] = "";
	osip_route_t *route = NULL;
	osip_route_init(&route);

	// setup a new register message
	osip_message_t *reg = NULL;
	osip_message_init(&reg);

	// printf("PCSCF: %s\n", add_sip_scheme(pref->pcscf));

	// checks for valid P-CSCF - exit on error
	if (osip_route_parse(route, add_sip_scheme(pref->pcscf)) < 0)
	{
		set_display("Invalid P-CSCF\n\nCorrect usage:\n  sip:pcscf.open-ims.test");
		return -1;

	}

	// build initial register message with the public ui and realm
	eXosip_lock (context_eXosip);

	if ((reg_id < 0) || (preferences_changed == 1))
	{

		reg_id = eXosip_register_build_initial_register(context_eXosip, pref->impu,add_sip_scheme(pref->realm),NULL,REG_EXPIRE,&reg);

		/* check to see that register was built correctly */
		if (reg_id < 0)
		{
			set_display("Error building register message\n\nTry deregister then register");
			eXosip_unlock (context_eXosip);
			return -1;
		}

		/* add p-cscf uri to header field */
		osip_message_set_route(reg, add_lr_to_route(add_sip_scheme(pref->pcscf)));
	
		/* add path and gruu to supported header */
		osip_message_set_supported(reg, "path");
		osip_message_set_supported(reg, "gruu");

		/* Add our GRUU to the Contact header field */
		osip_contact_t *contact_header;
		osip_message_get_contact (reg, 0, &contact_header);
		osip_contact_param_add(contact_header, osip_strdup("+sip.instance"), osip_strdup(gruu));

		/* Add a timeout to refresh our registration 32 seconds before it expires */
		g_timeout_add ((1000 * (REG_EXPIRE - 32)), (GSourceFunc)ims_send_register, NULL);

		/* Add a timeout to refresh our presence subscriptions 32 seconds before they expire */
		// g_timeout_add ((1000 * (PRESENCE_EXPIRE - 32)), (GSourceFunc)presence_subscribe_to_all_presentities, (gpointer)PRESENCE_EXPIRE);
	
		/* Add a timeout to refresh our watcher info subscription 32 seconds before it expires */
		// g_timeout_add ((1000 * (PRESENCE_EXPIRE - 32)),(GSourceFunc)watchers_subscribe_to_watcherinfo, (gpointer)PRESENCE_EXPIRE);

		preferences_changed = 0;

	}
	else
	{
		eXosip_register_build_register(context_eXosip, reg_id, REG_EXPIRE, &reg);
	}

	eXosip_unlock (context_eXosip);

	/* create an authorisation header */
	osip_authorization_t *auth_header;
	osip_authorization_init(&auth_header);

	osip_authorization_set_auth_type(auth_header, "Digest");
	osip_authorization_set_username(auth_header, add_quotes(pref->impi));
	osip_authorization_set_realm(auth_header, add_quotes(pref->realm));	
	osip_authorization_set_nonce(auth_header, "\" \"");
	osip_authorization_set_uri(auth_header, add_quotes(add_sip_scheme(pref->realm)));
	osip_authorization_set_response(auth_header, "\" \"");

	// add authorisation header to register message
	char *h_value;
	osip_authorization_to_str(auth_header, &h_value);

	if (osip_message_set_authorization(reg, h_value) != 0)
	{
		printf("Error adding authorisation header");
		return -1;
	}

	
	/* send register message */
	eXosip_lock(context_eXosip);
	if (eXosip_register_send_register (context_eXosip, reg_id, reg) != 0)	
	{
		set_display("Error sending registration");
		eXosip_unlock (context_eXosip);
		return -1;
	}
	eXosip_unlock (context_eXosip);

	// start timer for delay tests
	gettimeofday(&start_time, NULL);

	sprintf(display, "REGISTER sent to\n%s",pref->pcscf);
	set_display (display);
	imsua_set_message_display ("REGISTER", 1);


}


/*
void ims_open_im_window()
{
	Call *ca;

	//session based IM component exists
	if (find_call(current_dialog_id, &ca) >= 0 && ca->remote_msrp_port && pref->session_im_enabled) 
	{
		if(im_window_open == 0)
			{
				im_window = GTK_WINDOW(create_im_window());
				gtk_widget_show (GTK_WIDGET(im_window));
				im_window_open = 1;
				time_stamps = NOT_VISIBLE;
			}
			else
			{
				//bring focus to im_window
				gtk_window_present(im_window);
			}
	}
	else
	{
		const gchar *chat_uri_entry;
		GtkWidget *chat_uri = lookup_widget(GTK_WIDGET(imsUA), "chat_uri_entry");
		chat_uri_entry = gtk_entry_get_text(GTK_ENTRY(chat_uri));
		
		osip_route_t *rt=NULL;
		osip_route_init(&rt);
		char *tmproute=NULL;
	
		char *sip_str = "sip:";
		char *sip_strs = "sips:";
	
		if(registered == NOT_REGISTERED)
		{
			set_display("Not registered with a domain");
		}
		//checks for valid public_ui
		else if((strstr(pref->impu, sip_str) != pref->impu) && (strstr(pref->impu, sip_strs) != pref->impu))
		{
			set_display("Invalid IMS URI\nIdentity\n\nCheck Preferences");
		}
		//check for valid sending uri
		else if (osip_route_parse(rt,chat_uri_entry) < 0)
		{
			set_display("Cannot start IM session: Invalid URI\n\nCorrect usage - " 	"sip:{name}@{domain}:{port}");
		}
		else if (osip_route_parse(rt, add_sip_scheme(pref->pcscf)) < 0)
		{
			set_display("Invalid P-CSCF\n\nCheck Preferences");
		}
		else
		{
			if(im_window_open == 0)
			{
				im_window = GTK_WINDOW(create_im_window());
				gtk_widget_show (GTK_WIDGET(im_window));
				im_window_open = 1;
				time_stamps = NOT_VISIBLE;
			}
			else
			{
				//bring focus to im_window
				gtk_window_present(im_window);
			}
			common_start_im_session(chat_uri_entry);
		}
	}
}

*/



void ims_call_answer()
{

	Call *ca;

	if (find_call(current_dialog_id, &ca) < 0)
		return ;

	osip_message_t *answer = NULL;
	eXosip_lock (context_eXosip);
	eXosip_call_build_answer (context_eXosip, ca->tid, 200, &answer);
	eXosip_unlock (context_eXosip);

	if ((ca->most_recent_message) && (!ca->media_negotiated))
		sdp_complete_ims(ca->most_recent_message, &answer, ca, 0);

	eXosip_lock (context_eXosip);
	eXosip_call_send_answer (context_eXosip, ca->tid, 200, answer);
	eXosip_unlock (context_eXosip);
	set_display("Sending 200 OK");

	state = IN_CALL;

	ca->call_is_active = 1;

	imsua_set_message_display("200 OK (INVITE)", 1);
	
}




void ims_send_deregister_message ()
{

	Preferences *pref = client->pref;

	/*
	if (pref->presence_enabled)
	{
		presence_publish_presentity("closed", "Offline");
		// presence_subscribe_to_all_presentities(0);
	}
	*/

	is_message_deregister = 1;

	int i;
	osip_message_t *reg = NULL;
	eXosip_lock (context_eXosip);
	i = eXosip_register_build_register (context_eXosip, reg_id, 0, &reg);
	if (i < 0)
	{
		eXosip_unlock (context_eXosip);
		return ;
	}
	eXosip_unlock (context_eXosip);

	char *sip_str = "sip:";
	char *sip_strs = "sips:";
	char new_realm[128];

	if((strstr(pref->realm, sip_str) != pref->realm) && (strstr(pref->realm, sip_strs) != pref->realm))
	{
		strcpy(new_realm, "sip:");
		strcat(new_realm, pref->realm);
	}
	else
	{
		return ;
	}

	osip_authorization_t *auth_header;
	osip_authorization_init(&auth_header);

	osip_authorization_set_auth_type(auth_header, "Digest");
	osip_authorization_set_username(auth_header, add_quotes(pref->impi));
	osip_authorization_set_realm(auth_header, add_quotes(pref->realm));
	osip_authorization_set_nonce(auth_header, "\" \"");
	osip_authorization_set_uri(auth_header, add_quotes(pref->realm));
	osip_authorization_set_response(auth_header, "\" \"");

	char *h_value;
	osip_authorization_to_str(auth_header, &h_value);

	if (osip_message_set_authorization(reg, h_value) != 0)
		return ;

	eXosip_lock (context_eXosip);
	eXosip_register_send_register (context_eXosip, reg_id, reg);
	eXosip_unlock (context_eXosip);

	imsua_set_message_display ("REGISTER", 1);
	
}


/*
//editted Richard June 14 start
void ims_call_reinvite()
{

	if (state == MOD_SESSION)
	{
		set_display ("Session modification in process");	
	}

	//find current call
	Call *ca;

	if (find_call(current_dialog_id, &ca) < 0)
		return ;
	
	//ca->call_is_active = 0;

	// the reinvite message
	osip_message_t *reinvite = NULL;

	char display[500];
	//const gchar *uri_entry;
	char from_string[50];
	char sdp_body[4096];

	// get URI from address bar
	//GtkWidget *uri_entry_widget = lookup_widget(GTK_WIDGET(imsUA), "uri_entry");
	//uri_entry = gtk_entry_get_text(GTK_ENTRY(uri_entry_widget));
	// create from header with name and public UI
	sprintf(from_string, "\"%s\" <%s>", pref->name, pref->impu);

*/
	// get local IP address
	/*
	if(strcmp(media_interface, "Default") == 0)
	{
		eXosip_lock (context_eXosip);
		eXosip_guess_localip (context_eXosip, AF_INET, local_ip, 128);
		eXosip_unlock (context_eXosip);
	}
	else 
	{
		sprintf(local_ip,media_interface);
	}
	*/
/*
	osip_route_t *route = NULL;
	osip_route_init(&route);

	// check for valid P-CSCF
	if (osip_route_parse(route, add_sip_scheme(pref->pcscf)) < 0)
	{
		set_display("Invalid P-CSCF\n\nCorrect usage:\n  sip:pcscf.open-ims.test");
		return ;
	}

	// check for valid public UI and destination URI
	if (strlen(pref->impu) < 6)
	{
		set_display("Invalid Public User Identity\n\nCheck Preferences");
		return ;
	}
	else if((strstr(pref->impu, sip_str) != pref->impu) && (strstr(pref->impu, sip_strs) != pref->impu))
	{
		set_display("Invalid Public User Identity\n\nCorrect usage sip:user@address.com");
		return ;
	}
	else if(eXosip_call_build_request ( ca->did, "INVITE", &reinvite ))
	{
		set_display("ERROR sending re-invite\n\nCheck preferences");
		return ;
	}
	

	// set the desired QoS level and type
	// see RFC3312 - Integration of Resource Management and Session Initiation Protocol (SIP)
	char qos_info[200];
	if (pref->qos_strength == QOS_NONE)
		sprintf(qos_info, "a=curr:qos local none\r\n"
		"a=curr:qos remote none\r\n"
		"a=des:qos none local sendrecv\r\n"
		"a=des:qos none remote sendrecv\r\n");
	else if ((pref->qos_strength == QOS_OPTIONAL) && (pref->qos_type == QOS_SEGMENTED))
		sprintf(qos_info, "a=curr:qos local none\r\n"
		"a=curr:qos remote none\r\n"
		"a=des:qos optional local sendrecv\r\n"
		"a=des:qos optional remote sendrecv\r\n");
	else if ((pref->qos_strength == QOS_MANDATORY) && (pref->qos_type == QOS_SEGMENTED))
		sprintf(qos_info, "a=curr:qos local none\r\n"
		"a=curr:qos remote none\r\n"
		"a=des:qos mandatory local sendrecv\r\n"
		"a=des:qos mandatory remote sendrecv\r\n");
	else if ((pref->qos_strength == QOS_OPTIONAL) && (pref->qos_type == QOS_E2E))
		sprintf(qos_info, "a=curr:qos e2e none\r\n"
		"a=des:qos optional e2e sendrecv\r\n");
	else if ((pref->qos_strength == QOS_MANDATORY) && (pref->qos_type == QOS_E2E))
		sprintf(qos_info, "a=curr:qos e2e none\r\n"
		"a=des:qos mandatory e2e sendrecv\r\n");


	// Support for RFC 3325 - Private Extensions to the Session Initiation Protocol (SIP)
	// for Asserted Identity within Trusted Networks
	osip_message_set_header((osip_message_t *)reinvite,(const char *)"P-Preferred-Identity",from_string);

	osip_message_set_header((osip_message_t *)reinvite,(const char *)"Privacy","none");

	// Support for RFC 3455 - Private Header (P-Header) Extensions to the Session Initiation
    	// Protocol (SIP) for the 3rd-Generation Partnership Project (3GPP)
	osip_message_set_header((osip_message_t *)reinvite,(const char *)"P-Access-Network-Info",access_networks[pref->access_network]);

	osip_message_set_require(reinvite, "sec-agree");
	osip_message_set_proxy_require(reinvite, "sec-agree");
	osip_message_set_supported(reinvite, "100rel");

	//If message had an MSRP component must initialise an end point
	osip_call_id_t *call_id_header = osip_message_get_call_id (reinvite);
	char *temp = osip_call_id_get_number(call_id_header);
	int call_id = atoi(temp);
	
	if(ca->remote_msrp_port && local_msrp_endpoint)
	{
		endpointmsrp_end_session(1);
	}
	if(pref->session_im_enabled)
	{
		local_msrp_endpoint = endpointmsrp_create_endpoint(call_id,pref->local_audio_ip,pref->local_msrp_port,0);
	}
	
	// Put in our media preferences
	sdp_complete_ims(NULL, &reinvite, NULL, 0);

	// send the reinvite
	eXosip_lock (context_eXosip);
	int i = eXosip_call_send_request (context_eXosip, ca->did, reinvite );
 	eXosip_unlock (context_eXosip);

	// start timer for delay tests
	gettimeofday(&start_time, NULL);

	if (i != 0)
		set_display("Error sending IMS re-invite");
	else
	{
		if(ca->caller == 1)
		{
			sprintf(display, "Sent IMS re-INVITE to \n\n<%s>", ca->to_uri);
		}
		else
		{
			sprintf(display, "Sent IMS re-INVITE to \n\n<%s>", ca->from_uri);
		}
		set_display(display);
	}
	
	state = MOD_SESSION;
}

*/

