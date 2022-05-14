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
#include "preferences.h"

char *preferences_remove_newline(char *input)
{
	int i;
	char *response = malloc(128);
	
	for (i = 0; i < strlen(input); i++)
	{
		if (input[i] == '\n')
			response[i] = '\0';
		else
			response[i] = input[i];
	}

	return response;
}


void preferences_set_preferences_dialog()
{

	Preferences *pref = client->pref;

	gtk_entry_set_text(GTK_ENTRY(client->general_name_entry), pref->name);
	gtk_entry_set_text(GTK_ENTRY(client->general_impu_entry), pref->impu);
	gtk_entry_set_text(GTK_ENTRY(client->general_impi_entry), pref->impi);
	gtk_entry_set_text(GTK_ENTRY(client->general_pcscf_entry), pref->pcscf);
	gtk_entry_set_text(GTK_ENTRY(client->general_realm_entry), pref->realm);
	gtk_entry_set_text(GTK_ENTRY(client->general_password_entry), pref->password);


	gtk_combo_box_set_active (GTK_COMBO_BOX(client->media_primaryvoicecodec_combobox), pref->audio_codec_primary);
	gtk_combo_box_set_active (GTK_COMBO_BOX(client->media_secondaryvoicecodec_combobox), pref->audio_codec_secondary);

	// get list of Linux IP addresses if not set already
	if (gtk_combo_box_get_active(GTK_COMBO_BOX(client->media_mediainterface_combobox)) == -1)
	{

		gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT(client->media_mediainterface_combobox), NULL, "Default");

		int i;
		int s = socket (PF_INET, SOCK_STREAM, 0);

		for (i=1;;i++)
	 	{
	   		struct ifreq ifr;
	   		struct sockaddr_in *sin = (struct sockaddr_in *) &ifr.ifr_addr;
	   		char *ip;

	   		ifr.ifr_ifindex = i;
	   		if (ioctl (s, SIOCGIFNAME, &ifr) < 0)
	     		break;

	   		// now ifr.ifr_name is set
	   		if (ioctl (s, SIOCGIFADDR, &ifr) < 0)
	 	    	continue;

	   		ip = inet_ntoa (sin->sin_addr);

	   		gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT(client->media_mediainterface_combobox), NULL, ip);
	 	}
		close (s);


		gtk_combo_box_set_active (GTK_COMBO_BOX(client->media_mediainterface_combobox), pref->media_interface);

	}

	gtk_combo_box_set_active (GTK_COMBO_BOX(client->media_videocalling_combobox), pref->video_enabled);
	gtk_combo_box_set_active (GTK_COMBO_BOX(client->media_videoquality_combobox), pref->video_bw);


/*
	if(preferences_open == 0)
	{
		preferences_open = 1;
		preferences = create_preferences ();
	
		GtkWidget *profile_name_widget = lookup_widget(GTK_WIDGET(preferences), "profile_name");
		GtkWidget *enable_presence_widget = lookup_widget(GTK_WIDGET(preferences), "enable_presence");
		GtkWidget *enable_video_widget = lookup_widget(GTK_WIDGET(preferences), "enable_video_calling");
		GtkWidget *enable_session_im_widget = lookup_widget(GTK_WIDGET(preferences), "enable_session_im");
	
		GtkWidget *ims_public_ui_widget = lookup_widget(GTK_WIDGET(preferences), "ims_public_ui");
		GtkWidget *ims_private_ui_widget = lookup_widget(GTK_WIDGET(preferences), "ims_private_ui");
		GtkWidget *ims_pcscf_widget = lookup_widget(GTK_WIDGET(preferences), "ims_pcscf");
		GtkWidget *ims_realm_widget = lookup_widget(GTK_WIDGET(preferences), "ims_realm");
		GtkWidget *ims_password_widget = lookup_widget(GTK_WIDGET(preferences), "ims_password");
		GtkWidget *ims_qos_widget = lookup_widget(GTK_WIDGET(preferences), "ims_qos");
		GtkWidget *ims_qos_type_widget = lookup_widget(GTK_WIDGET(preferences), "ims_qos_type");
		GtkWidget *ims_access_network_widget = lookup_widget(GTK_WIDGET(preferences), "ims_access_network");
		
	
		GtkWidget *media_1st_codec_widget = lookup_widget(GTK_WIDGET(preferences), "media_1st_codec");
		GtkWidget *media_2nd_codec_widget = lookup_widget(GTK_WIDGET(preferences), "media_2nd_codec");	
		GtkWidget *media_dtmf_events_widget = lookup_widget(GTK_WIDGET(preferences), "media_dtmf_events");
		GtkWidget *media_interface_widget = lookup_widget(GTK_WIDGET(preferences), "media_interface");
		GtkWidget *media_video_bw_widget = lookup_widget(GTK_WIDGET(preferences), "media_video_bw");
		GtkWidget *media_iptv_server_widget = lookup_widget(GTK_WIDGET(preferences), "iptv_server_entry");
		GtkWidget *media_iptv_hw_acceleration_widget = lookup_widget(GTK_WIDGET(preferences), "media_hw_acceleration");

		GtkWidget *xcap_server_url_widget = lookup_widget(GTK_WIDGET(preferences), "xcap_server_url_entry");
		GtkWidget *xcap_username_widget = lookup_widget(GTK_WIDGET(preferences), "xcap_username_entry");
		GtkWidget *xcap_password_widget = lookup_widget(GTK_WIDGET(preferences), "xcap_password_entry");
*/


/*
		gtk_combo_box_set_active (GTK_COMBO_BOX(enable_presence_widget), pref->presence_enabled);
		gtk_combo_box_set_active (GTK_COMBO_BOX(enable_video_widget), pref->video_enabled);
		gtk_combo_box_set_active (GTK_COMBO_BOX(enable_session_im_widget), pref->session_im_enabled);
	
		gtk_entry_set_text(GTK_ENTRY(ims_public_ui_widget), pref->impu);
		gtk_entry_set_text(GTK_ENTRY(ims_private_ui_widget), pref->impi);
		gtk_entry_set_text(GTK_ENTRY(ims_pcscf_widget), pref->pcscf);
		gtk_entry_set_text(GTK_ENTRY(ims_realm_widget), pref->realm);
		gtk_entry_set_text(GTK_ENTRY(ims_password_widget), pref->password);
	
		gtk_combo_box_set_active (GTK_COMBO_BOX(ims_qos_widget), pref->qos_strength);
		gtk_combo_box_set_active (GTK_COMBO_BOX(ims_qos_type_widget), pref->qos_type);
		gtk_combo_box_set_active (GTK_COMBO_BOX(ims_access_network_widget), pref->access_network);
		gtk_combo_box_set_active (GTK_COMBO_BOX(media_1st_codec_widget), pref->audio_codec_primary);
		gtk_combo_box_set_active (GTK_COMBO_BOX(media_2nd_codec_widget), pref->audio_codec_secondary);
		gtk_combo_box_set_active (GTK_COMBO_BOX(media_dtmf_events_widget), pref->dtmf);
		gtk_combo_box_set_active (GTK_COMBO_BOX(media_video_bw_widget), pref->video_bw);
		
		gtk_entry_set_text(GTK_ENTRY(media_iptv_server_widget), pref->iptv_server);
		gtk_combo_box_set_active (GTK_COMBO_BOX(media_iptv_hw_acceleration_widget), pref->iptv_hw_acceleration);
		


		// get list of Linux IP addresses
 		gtk_combo_box_append_text (GTK_COMBO_BOX(media_interface_widget), "Default");

		int i;
      		int s = socket (PF_INET, SOCK_STREAM, 0);

       		for (i=1;;i++)
         	{
           		struct ifreq ifr;
           		struct sockaddr_in *sin = (struct sockaddr_in *) &ifr.ifr_addr;
           		char *ip;
 
           		ifr.ifr_ifindex = i;
           		if (ioctl (s, SIOCGIFNAME, &ifr) < 0)
             		break;
 
           		// now ifr.ifr_name is set
           		if (ioctl (s, SIOCGIFADDR, &ifr) < 0)
         	    	continue;
 	
           		ip = inet_ntoa (sin->sin_addr);
           		gtk_combo_box_append_text (GTK_COMBO_BOX(media_interface_widget), ip);
         	}
       		close (s);

 		gtk_combo_box_set_active (GTK_COMBO_BOX(media_interface_widget), pref->media_interface);

		gtk_entry_set_text(GTK_ENTRY(xcap_server_url_widget), pref->xdms_root_url);
		gtk_entry_set_text(GTK_ENTRY(xcap_username_widget), pref->xdms_username);
		gtk_entry_set_text(GTK_ENTRY(xcap_password_widget), pref->xdms_password);
	
		gtk_widget_show (preferences);
	}
	else
	{
		//bring focus to preferences window
		gtk_window_present(GTK_WINDOW(preferences));
	}

*/
}




void preferences_get_preferences_dialog()
{

	Preferences *pref = client->pref;

	/*
	GtkWidget *profile_name_widget = lookup_widget(GTK_WIDGET(button), "profile_name");

	GtkWidget *enable_presence_widget = lookup_widget(GTK_WIDGET(button), "enable_presence");
	GtkWidget *enable_video_widget = lookup_widget(GTK_WIDGET(button), "enable_video_calling");
	GtkWidget *enable_session_im_widget = lookup_widget(GTK_WIDGET(button), "enable_session_im");

	GtkWidget *ims_public_ui_widget = lookup_widget(GTK_WIDGET(button), "ims_public_ui");
	GtkWidget *ims_private_ui_widget = lookup_widget(GTK_WIDGET(button), "ims_private_ui");
	GtkWidget *ims_pcscf_widget = lookup_widget(GTK_WIDGET(button), "ims_pcscf");
	GtkWidget *ims_realm_widget = lookup_widget(GTK_WIDGET(button), "ims_realm");
	GtkWidget *ims_password_widget = lookup_widget(GTK_WIDGET(button), "ims_password");
	GtkWidget *ims_qos_widget = lookup_widget(GTK_WIDGET(button), "ims_qos");
	GtkWidget *ims_qos_type_widget = lookup_widget(GTK_WIDGET(button), "ims_qos_type");
	GtkWidget *ims_access_network_widget = lookup_widget(GTK_WIDGET(button), "ims_access_network");

	GtkWidget *media_1st_codec_widget = lookup_widget(GTK_WIDGET(button), "media_1st_codec");
	GtkWidget *media_2nd_codec_widget = lookup_widget(GTK_WIDGET(button), "media_2nd_codec");	
	GtkWidget *media_dtmf_events_widget = lookup_widget(GTK_WIDGET(button), "media_dtmf_events");
	GtkWidget *media_interface_widget = lookup_widget(GTK_WIDGET(button), "media_interface");
	GtkWidget *media_video_bw_widget = lookup_widget(GTK_WIDGET(button), "media_video_bw");
	GtkWidget *media_iptv_server_widget = lookup_widget(GTK_WIDGET(button), "iptv_server_entry");
	GtkWidget *media_iptv_hw_acceleration_widget = lookup_widget(GTK_WIDGET(button), "media_hw_acceleration");
	
	GtkWidget *xcap_server_url_widget = lookup_widget(GTK_WIDGET(preferences), "xcap_server_url_entry");
	GtkWidget *xcap_username_widget = lookup_widget(GTK_WIDGET(preferences), "xcap_username_entry");
	GtkWidget *xcap_password_widget = lookup_widget(GTK_WIDGET(preferences), "xcap_password_entry");

*/


	strcpy(pref->name, gtk_entry_get_text(GTK_ENTRY(client->general_name_entry)));

/*
	pref->presence_enabled = gtk_combo_box_get_active(GTK_COMBO_BOX(enable_presence_widget));
	pref->video_enabled = gtk_combo_box_get_active(GTK_COMBO_BOX(enable_video_widget));
	pref->session_im_enabled = gtk_combo_box_get_active(GTK_COMBO_BOX(enable_session_im_widget));

*/

	strcpy(pref->impu, gtk_entry_get_text(GTK_ENTRY(client->general_impu_entry)));
	strcpy(pref->impi, gtk_entry_get_text(GTK_ENTRY(client->general_impi_entry)));
	strcpy(pref->pcscf, gtk_entry_get_text(GTK_ENTRY(client->general_pcscf_entry)));
	strcpy(pref->realm, gtk_entry_get_text(GTK_ENTRY(client->general_realm_entry)));
	strcpy(pref->password, gtk_entry_get_text(GTK_ENTRY(client->general_password_entry)));
	
/*
	pref->qos_strength = gtk_combo_box_get_active(GTK_COMBO_BOX(ims_qos_widget));
	pref->qos_type = gtk_combo_box_get_active(GTK_COMBO_BOX(ims_qos_type_widget));
	pref->access_network = gtk_combo_box_get_active(GTK_COMBO_BOX(ims_access_network_widget));

*/

	pref->audio_codec_primary = gtk_combo_box_get_active(GTK_COMBO_BOX(client->media_primaryvoicecodec_combobox));
	pref->audio_codec_secondary = gtk_combo_box_get_active(GTK_COMBO_BOX(client->media_secondaryvoicecodec_combobox));
	//pref->dtmf = gtk_combo_box_get_active(GTK_COMBO_BOX(media_dtmf_events_widget));
	pref->media_interface = gtk_combo_box_get_active(GTK_COMBO_BOX(client->media_mediainterface_combobox));


	// Get the IP address if we are not using the default
	if(strcmp("Default", gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(client->media_mediainterface_combobox))))
	{
		strcpy(pref->local_audio_ip, gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(client->media_mediainterface_combobox)));
		strcpy(pref->local_video_ip, gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(client->media_mediainterface_combobox)));
	}

	pref->video_enabled = gtk_combo_box_get_active(GTK_COMBO_BOX(client->media_videocalling_combobox));
	pref->video_bw = gtk_combo_box_get_active(GTK_COMBO_BOX(client->media_videoquality_combobox));

/*
	strcpy(pref->iptv_server, gtk_entry_get_text(GTK_ENTRY(media_iptv_server_widget)));
	pref->iptv_hw_acceleration = gtk_combo_box_get_active(GTK_COMBO_BOX(media_iptv_hw_acceleration_widget));
	
	strcpy(pref->xdms_root_url, gtk_entry_get_text(GTK_ENTRY(xcap_server_url_widget)));
	strcpy(pref->xdms_username, gtk_entry_get_text(GTK_ENTRY(xcap_username_widget)));
	strcpy(pref->xdms_password, gtk_entry_get_text(GTK_ENTRY(xcap_password_widget)));


	GtkWidget *dialog = lookup_widget(GTK_WIDGET(button), "preferences");
	gtk_widget_destroy(dialog);

	preferences_open = 0;
	preferences_changed = 1;
*/
/*
	printf("Name: %s\n", pref->name);
	printf("IMPU: %s\n", pref->impu);
	printf("IMPI: %s\n", pref->impi);
	printf("PCSCF: %s\n", pref->pcscf);
	printf("Realm: %s\n", pref->realm);
	printf("Password: %s\n", pref->password);
	printf("QoS Strength: %d\n", pref->qos_strength);
	printf("QoS Type: %d\n", pref->qos_type);
	printf("Access Network: %d\n", pref->access_network);
	printf("Audio Codec Primarry: %d\n", pref->audio_codec_primary);
	printf("Audio Codec Secondary %d\n", pref->audio_codec_secondary);	
	printf("DTMF: %d\n", pref->dtmf);
	//printf("Sound Card: %d\n", pref->sound_card);
	printf("Media Interface: %d\n", pref->media_interface);
	printf("Video BW: %d\n", pref->video_bw);
	printf("XDMS Root: %s\n", pref->xdms_root_url);
	printf("XDMS Username: %s\n", pref->xdms_username);
	printf("XDMS Password %s\n", pref->xdms_password);
*/
}


int preferences_get_preferences_from_xml_file(Preferences *pref, char *filename)
{

	// Parse our preferences file
	xmlDocPtr doc;
	xmlNodePtr cur, child;

	if(!(doc = xmlParseFile(filename)))
	{
		// g_warning("Error opening preferences file: %s", filename);
		xmlFreeDoc( doc );		
		return -1;
	}

	if(!(cur = xmlDocGetRootElement(doc)))
	{
		g_warning("Preferences document has no root element");
		xmlFreeDoc( doc );
		return -1;
	}

	if(xmlStrcmp(cur->name, (const xmlChar *) "preferences" ))
	{
		g_warning("XML document of the wrong type, root node != preferences");
		xmlFreeDoc( doc );
		return -1;
	}

	cur = cur->xmlChildrenNode;

	// Traverse through document looking for our preferences
	while(cur)
	{
		if (cur->type == XML_ELEMENT_NODE)
		{	
			if(child = cur->xmlChildrenNode)
			{

				if(!xmlStrcmp(cur->name, (const xmlChar *)"name"))
					strcpy(pref->name, child->content);

				if(!xmlStrcmp(cur->name, (const xmlChar *)"presence-enabled"))
					pref->presence_enabled = atoi(child->content);

				if(!xmlStrcmp(cur->name, (const xmlChar *)"video-enabled"))
					pref->video_enabled = atoi(child->content);
				
				if(!xmlStrcmp(cur->name, (const xmlChar *)"session-im-enabled"))
					pref->session_im_enabled = atoi(child->content);
		
				if(!xmlStrcmp(cur->name, (const xmlChar *)"impu"))
					strcpy(pref->impu, child->content);

				if(!xmlStrcmp(cur->name, (const xmlChar *)"impi"))
					strcpy(pref->impi, child->content);

				if(!xmlStrcmp(cur->name, (const xmlChar *)"pcscf"))
					strcpy(pref->pcscf, child->content);

				if(!xmlStrcmp(cur->name, (const xmlChar *)"realm"))
					strcpy(pref->realm, child->content);

				if(!xmlStrcmp(cur->name, (const xmlChar *)"password"))
					strcpy(pref->password, child->content);

				if(!xmlStrcmp(cur->name, (const xmlChar *)"qos-strength"))
					pref->qos_strength = atoi(child->content);

				if(!xmlStrcmp(cur->name, (const xmlChar *)"qos-type"))
					pref->qos_type = atoi(child->content);

				if(!xmlStrcmp(cur->name, (const xmlChar *)"access-network"))
					pref->access_network = atoi(child->content);

				if(!xmlStrcmp(cur->name, (const xmlChar *)"audio-codec-primary"))
					pref->audio_codec_primary = atoi(child->content);

				if(!xmlStrcmp(cur->name, (const xmlChar *)"audio-codec-secondary"))
					pref->audio_codec_secondary = atoi(child->content);

				if(!xmlStrcmp(cur->name, (const xmlChar *)"dtmf"))
					pref->dtmf = atoi(child->content);

				if(!xmlStrcmp(cur->name, (const xmlChar *)"media-interface"))
					pref->media_interface = atoi(child->content);
	
				if(!xmlStrcmp(cur->name, (const xmlChar *)"video-bw"))
					pref->video_bw = atoi(child->content);

				if(!xmlStrcmp(cur->name, (const xmlChar *)"iptv-server"))
					strcpy(pref->iptv_server, child->content);

				if(!xmlStrcmp(cur->name, (const xmlChar *)"iptv-hw-acceleration"))
					pref->iptv_hw_acceleration = atoi(child->content);

				if(!xmlStrcmp(cur->name, (const xmlChar *)"xdms-root-url"))
					strcpy(pref->xdms_root_url, child->content);

				if(!xmlStrcmp(cur->name, (const xmlChar *)"xdms-username"))
					strcpy(pref->xdms_username, child->content);

				if(!xmlStrcmp(cur->name, (const xmlChar *)"xdms-password"))
					strcpy(pref->xdms_password, child->content);

			}

		}
		
		cur = cur->next;
	}



	printf("Name: %s\n", pref->name);
	printf("IMPU: %s\n", pref->impu);
	printf("IMPI: %s\n", pref->impi);
	printf("PCSCF: %s\n", pref->pcscf);
	printf("Realm: %s\n", pref->realm);
	printf("Password: %s\n", pref->password);
	printf("QoS Strength: %d\n", pref->qos_strength);
	printf("QoS Type: %d\n", pref->qos_type);
	printf("Access Network: %d\n", pref->access_network);
	printf("Audio Codec Primarry: %d\n", pref->audio_codec_primary);
	printf("Audio Codec Secondary %d\n", pref->audio_codec_secondary);	
	printf("DTMF: %d\n", pref->dtmf);
	// printf("Sound Card: %d\n", pref->sound_card);
	printf("Media Interface: %d\n", pref->media_interface);
	printf("Video Enabled: %d\n", pref->video_enabled);
	printf("Session IM Enabled: %d\n", pref->session_im_enabled);
	printf("Video BW: %d\n", pref->video_bw);
	printf("XDMS Root: %s\n", pref->xdms_root_url);
	printf("XDMS Username: %s\n", pref->xdms_username);
	printf("XDMS Password %s\n", pref->xdms_password);


	return 0;
}



int preferences_write_preferences_to_xml_file(Preferences *pref, char *filename)
{

	xmlDocPtr doc;
	xmlNodePtr root, child;
	char string[10];

	// Create a new XML document and set the root node 
	doc = xmlNewDoc("1.0");
	root = xmlNewNode(NULL, "preferences");
	xmlDocSetRootElement(doc, root);

	// Add the children
	xmlNewTextChild(root, NULL, "name", pref->name);

	sprintf(string, "%d", pref->presence_enabled);
	xmlNewTextChild(root, NULL, "presence-enabled", string);

	sprintf(string, "%d", pref->video_enabled);
	xmlNewTextChild(root, NULL, "video-enabled", string);

	sprintf(string, "%d", pref->session_im_enabled);
	xmlNewTextChild(root, NULL, "session-im-enabled", string);

	xmlNewTextChild(root, NULL, "impu", pref->impu);
	xmlNewTextChild(root, NULL, "impi", pref->impi);
	xmlNewTextChild(root, NULL, "pcscf", pref->pcscf);
	xmlNewTextChild(root, NULL, "realm", pref->realm);
	xmlNewTextChild(root, NULL, "password", pref->password);

	sprintf(string, "%d", pref->qos_strength);
	xmlNewTextChild(root, NULL, "qos-strength", string);

	sprintf(string, "%d", pref->qos_type);
	xmlNewTextChild(root, NULL, "qos-type", string);

	sprintf(string, "%d", pref->access_network);
	xmlNewTextChild(root, NULL, "access-network", string);

	sprintf(string, "%d", pref->audio_codec_primary);
	xmlNewTextChild(root, NULL, "audio-codec-primary", string);

	sprintf(string, "%d", pref->audio_codec_secondary);
	xmlNewTextChild(root, NULL, "audio-codec-secondary", string);

	sprintf(string, "%d", pref->dtmf);
	xmlNewTextChild(root, NULL, "dtmf", string);

	sprintf(string, "%d", pref->media_interface);
	xmlNewTextChild(root, NULL, "media-interface", string);

	sprintf(string, "%d", pref->video_bw);
	xmlNewTextChild(root, NULL, "video-bw", string);

	xmlNewTextChild(root, NULL, "iptv-server", pref->iptv_server);

	sprintf(string, "%d", pref->iptv_hw_acceleration);
	xmlNewTextChild(root, NULL, "iptv-hw-acceleration", string);

	xmlNewTextChild(root, NULL, "xdms-root-url", pref->xdms_root_url);
	xmlNewTextChild(root, NULL, "xdms-username", pref->xdms_username);
	xmlNewTextChild(root, NULL, "xdms-password", pref->xdms_password);

	// Write the XML file to disk
	xmlSaveFormatFile (filename, doc, 1);
	

}


int preferences_set_default_preferences(Preferences *pref)
{

	/* Allocate memory for the preferences */
	// pref = (Preferences*)malloc(sizeof(Preferences));

	/* Define some default preferences - these will be overwritten by the preferences.xml file if found */
	strcpy(pref->name, "Alice");

	pref->presence_enabled = 0;
	pref->video_enabled = 1;

	strcpy(pref->impu, "sip:alice@open-ims.test");
	strcpy(pref->impi, "alice@open-ims.test");
	strcpy(pref->pcscf, "pcscf.open-ims.test");
	strcpy(pref->realm, "open-ims.test");
	strcpy(pref->password, "alice");
	
	pref->qos_strength = 0;
	pref->qos_type = 0;
	pref->access_network = 0;

	pref->audio_codec_primary = 0;
	pref->audio_codec_secondary = 1;
	pref->dtmf = 0;
	pref->media_interface = 0;
	pref->video_bw = 0;
	pref->iptv_hw_acceleration = 1;

	pref->session_im_enabled = 1;
	
	strcpy(pref->xdms_root_url, "http://xcap.example.com/xcap-root");
	strcpy(pref->xdms_username, "alice@open-ims.test");
	strcpy(pref->xdms_password, "alice");	

	eXosip_guess_localip (context_eXosip, AF_INET, pref->local_audio_ip, 128);
	eXosip_guess_localip (context_eXosip, AF_INET, pref->local_video_ip, 128); 
	eXosip_guess_localip (context_eXosip, AF_INET, pref->local_msrp_ip, 128); 

	/* Generate random media port numbers so many clients can run on the same machine */
	time_t seconds;
	time(&seconds);
	srand((unsigned int) seconds);
	pref->local_audio_port = 10000 + rand()/(int)(((unsigned)RAND_MAX + 1) / 30000);
	pref->local_video_port = 11000 + rand()/(int)(((unsigned)RAND_MAX + 1) / 30000);
	pref->local_msrp_port = 12000 + rand()/(int)(((unsigned)RAND_MAX + 1) / 30000);
}
