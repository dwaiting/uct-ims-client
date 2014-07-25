/*
  The UCT IMS Client
  Copyright (C) 2006-2012 - University of Cape Town
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
#include "globals.h"

//#include <eXosip2/eXosip.h>
//#include <gtk/gtk.h>
#include "main.h"

/*
typedef struct
{
        GtkWidget               *mainwindow;
        GtkWidget               *registration_statusbar;
	GtkWidget               *call_statusbar;
        GtkWidget               *log_view;
} ClientUI;
*/




int initialise_eXosip(Preferences *pref)
{
	int port = 5060;
	int i = eXosip_init();

	if (i != 0)  {
		fprintf(stderr, "Could not start eXosip\n");
		return -1;
	}

	while(eXosip_listen_addr (IPPROTO_UDP, NULL, port, AF_INET, 0) != 0)
	{
		port++;
	}

	printf("UCT IMS Client - SIP port: %d - Audio Port: %d - Video Port: %d - MSRP Port: %d\n\n", port, pref->local_audio_port, pref->local_video_port, pref->local_msrp_port);

	return 0;
}


gint get_exosip_events()
{
	eXosip_event_t *je;
	char display[500] = "";
 	eXosip_lock();
	eXosip_unlock();

	Preferences *pref = client->pref;
	// printf("Timeout!\n");

	/* Check for eXosip event - timeout after 50ms */
	if((je = eXosip_event_wait(0,50)) != NULL)
	{
		/* Uncomment the next line for debugging */
		 fprintf(stderr, "Event type: %d %s\n", je->type, je->textinfo);


		imsua_display_event_info(je);

		if (je->type == EXOSIP_CALL_INVITE)
		{
			ims_process_incoming_invite(je);
		}
		else if (je->type == EXOSIP_CALL_REINVITE)
		{
			ims_process_incoming_reinvite(je);
		}
		else if (je->type == EXOSIP_CALL_RINGING)
		{
			ims_process_18x(je);
		}
		else if (je->type == EXOSIP_CALL_GLOBALFAILURE)
		{
			ims_process_released_call(je);
		}
		else if (je->type == EXOSIP_CALL_CLOSED)
		{
			ims_process_released_call(je);
		}
		else if (je->type == EXOSIP_CALL_ANSWERED)
		{
			ims_process_200ok(je);

		}
		else if (je->type == EXOSIP_CALL_RELEASED)
		{
			ims_process_released_call(je);

		}
		else if (je->type ==  EXOSIP_CALL_CANCELLED)
		{
		 	ims_process_released_call(je);
		}
		else if (je->type == EXOSIP_CALL_ACK)
		{
			ims_process_ack(je);
		}
		else if (je->type == EXOSIP_CALL_MESSAGE_REQUESTFAILURE)
		{
			ims_process_released_call(je);
		}
		else if (je->type == EXOSIP_CALL_REQUESTFAILURE)
		{
			set_display("Call released");
		}
		else if (je->type == EXOSIP_CALL_SERVERFAILURE)
		{
			set_display("Call released by server");
		}
		else if (je->type == EXOSIP_CALL_MESSAGE_NEW)
		{
			if (MSG_IS_PRACK(je->request))
				ims_process_prack(je);
			else if (MSG_IS_UPDATE(je->request))
				ims_process_update(je);
			//else if (MSG_IS_INFO(je->request))
				// common_process_info(je);
			else if (MSG_IS_BYE(je->request))
				imsua_set_message_display("OK (BYE)", 1);
		}
		else if (je->type == EXOSIP_CALL_MESSAGE_ANSWERED)
		{
			if (MSG_IS_BYE(je->request))
				ims_process_released_call(je);
			else if (MSG_IS_UPDATE(je->request) || MSG_IS_PRACK(je->request))
				ims_process_2xx(je);
		}
		else if (je->type == EXOSIP_MESSAGE_NEW)
		{
			if (MSG_IS_MESSAGE(je->request))
			{
				// Checks that message is actually destined for user by comparing sending TO field to IMPU
				char sending_ui[50];
				strcpy(sending_ui,(((je->request)->to)->url)->username);
				strcat(sending_ui,"@");
				strcat(sending_ui,(((je->request)->to)->url)->host);

				char *temp;
				temp = strstr(pref->impu,":") + 1;

				if(strcmp(sending_ui,temp)==0)
				{
					char *null_string = NULL;
					// ims_start_im_session(je, null_string);
				}

			}
			else if (MSG_IS_BYE(je->request))
			{
				set_display("Call ended");
			}

		}
		else if(je->type == EXOSIP_MESSAGE_REQUESTFAILURE)
		{

		}
		else if(je->type == EXOSIP_MESSAGE_ANSWERED)
		{

		}
		else if(je->type == EXOSIP_REGISTRATION_SUCCESS)
		{

			if(is_message_deregister == 1)
			{
				registered = NOT_REGISTERED;

				is_message_deregister = 0;

				sprintf(display, "Deregistered with %s",pref->realm);
				set_display(display);

				sprintf(display,"Not registered");
				set_status_bar(display);

				// watchers_remove_all_watchers();

				num_associated_uris = 0;

			}
			else
			{
				registered = REGISTERED;
				ims_process_registration_200ok(je);

			}
		}
		else if(je->type == EXOSIP_REGISTRATION_FAILURE)
		{

			if((je->response)== NULL)
			{
				set_display("Registration failed for unknown reason\nMost probably incorrect credentials\n\nCheck Preferences");
			}
			else if(((je->response)->status_code == 403))
			{
				set_display("Invalid user name\n\nCheck Preferences");
			}
			else if(((je->response)->status_code == 401))
			{
				ims_process_401(je);
			}
			else if(((je->response)->status_code == 404) || ((je->response)->status_code == 407))
			{

				set_display("Error with credentials\n\nCheck Preferences");
			}
			else
			{
				set_display("Registration failed for unknown reason\n\nMost probably incorrect credentials\nCheck Preferences");
			}

		}
		else if(je->type == EXOSIP_REGISTRATION_REFRESHED)
		{
			set_display("Registration Refreshed");
			registered = REGISTERED;
		}
		else if(je->type == EXOSIP_REGISTRATION_TERMINATED)
		{
		}
		else if(je->type == EXOSIP_SUBSCRIPTION_ANSWERED)
		{
			ims_process_subscription_answered(je);
		}
		else if (je->type == EXOSIP_SUBSCRIPTION_NOTIFY)
		{
			ims_process_notify(je);

		}
		else if (je->type == EXOSIP_SUBSCRIPTION_REQUESTFAILURE)
		{

		}
		else if (je->type == EXOSIP_IN_SUBSCRIPTION_NEW)
		{

		}
		else if((je->response)&&((je->response)->status_code == 302))
		{
			ims_process_302(je);
		}
		else
		{
		}

		
	}


	return TRUE;
}

int initialise_ui() 
{
	GtkBuilder *builder;
	GError *error = NULL;
	PangoFontDescription    *font_desc;

	
	/* Create new GtkBuilder object */
	builder = gtk_builder_new();

	/* Load UI from file. */
	if( ! gtk_builder_add_from_file( builder, "interface/uctimsclient.glade", &error ) )
	{
		g_warning("%s", error->message);
		g_free(error);
		return(-1);
	}

	client->mainwindow = GTK_WIDGET(gtk_builder_get_object(builder, "mainwindow"));
	client->uri_entry = GTK_WIDGET(gtk_builder_get_object(builder, "uri_entry"));
	client->local_cam = GTK_WIDGET(gtk_builder_get_object(builder, "local_cam"));
	client->remote_cam = GTK_WIDGET(gtk_builder_get_object(builder, "remote_cam"));
	client->registration_status_label = GTK_WIDGET(gtk_builder_get_object(builder, "registration_status_label"));
	client->call_status_label = GTK_WIDGET(gtk_builder_get_object(builder, "call_status_label"));
	client->log_view = GTK_WIDGET(gtk_builder_get_object(builder, "log_view"));

	client->im_text_entry = GTK_WIDGET(gtk_builder_get_object(builder, "im_text_entry"));
	client->im_text_view = GTK_WIDGET(gtk_builder_get_object(builder, "im_text_view"));
	client->im_notebook = GTK_WIDGET(gtk_builder_get_object(builder, "im_notebook"));
	client->im_tab_label = GTK_WIDGET(gtk_builder_get_object(builder, "im_tab_label"));

	client->general_name_entry = GTK_WIDGET(gtk_builder_get_object(builder, "general_name_entry"));
	client->general_impu_entry = GTK_WIDGET(gtk_builder_get_object(builder, "general_impu_entry"));	
	client->general_impi_entry = GTK_WIDGET(gtk_builder_get_object(builder, "general_impi_entry"));
	client->general_pcscf_entry = GTK_WIDGET(gtk_builder_get_object(builder, "general_pcscf_entry"));
	client->general_realm_entry = GTK_WIDGET(gtk_builder_get_object(builder, "general_realm_entry"));
	client->general_password_entry = GTK_WIDGET(gtk_builder_get_object(builder, "general_password_entry"));

	client->media_primaryvoicecodec_combobox = GTK_WIDGET(gtk_builder_get_object(builder, "media_primaryvoicecodec_combobox"));
	client->media_secondaryvoicecodec_combobox = GTK_WIDGET(gtk_builder_get_object(builder, "media_secondaryvoicecodec_combobox"));
	client->media_mediainterface_combobox = GTK_WIDGET(gtk_builder_get_object(builder, "media_mediainterface_combobox"));
	client->media_videocalling_combobox = GTK_WIDGET(gtk_builder_get_object(builder, "media_videocalling_combobox"));
	client->media_videoquality_combobox = GTK_WIDGET(gtk_builder_get_object(builder, "media_videoquality_combobox"));

	strcpy(client->im_buffer, "");

	/* Connect signals and pass NULL as user data */
	gtk_builder_connect_signals( builder, NULL );

	/* Enable showing images within buttons */
	GtkSettings *default_settings = gtk_settings_get_default();
	g_object_set(default_settings, "gtk-button-images", TRUE, NULL);

	/* Destroy builder, since we don't need it anymore */
	g_object_unref(G_OBJECT(builder));

	/* set the text view font */
        font_desc = pango_font_description_from_string ("monospace 8");
        gtk_widget_modify_font (client->log_view, font_desc);     
        pango_font_description_free (font_desc); 

	/* set the text view font */
        font_desc = pango_font_description_from_string ("monospace 9");
        gtk_widget_modify_font (client->registration_status_label, font_desc);     
        pango_font_description_free (font_desc); 

	/* set the text view font */
        font_desc = pango_font_description_from_string ("monospace 9");
        gtk_widget_modify_font (client->call_status_label, font_desc);     
        pango_font_description_free (font_desc); 

	gtk_widget_realize(client->local_cam);
	gtk_widget_realize(client->remote_cam);

	return 0;
}

gboolean shutdown_ui(GtkWidget *widget, GdkEvent *event, gpointer userdata)
{
	/* Write our preferences to the preferences file */
	preferences_write_preferences_to_xml_file(client->pref, preferences_file);

	/* If registered to a proxy deregister on exit */
	if(registered == REGISTERED)
	{
		ims_send_deregister_message ();
		
		sleep(1);

		eXosip_event_t *je;

		while((je = eXosip_event_wait(0,50)) != NULL)
		{
			if((je->type == EXOSIP_REGISTRATION_FAILURE) && ((je->response)->status_code == 401))
				ims_process_401(je);
		}

	}

	gtk_main_quit();
	msrp_quit();
	eXosip_quit();

	/* Stop other signal handlers from being evoked for this event */
	return TRUE;

}


int main( int argc, char *argv[] )
{

	/* Init GTK+ */
	gtk_init(&argc, &argv);

	/* Initialize GStreaemer */
	gst_init(NULL, NULL);

	// ClientUI *client;

	client = g_slice_new (ClientUI);
	client->pref = g_slice_new (Preferences);
	Preferences *pref = client->pref;

	initialise_ui();
	state = IDLE;

	/* Initialize MSRP */	
	if(msrp_init(events_msrp) < 0) {
		g_warning("Error initializing the MSRP library.");
	}

	/* Set default preferences and retrieve existing preferences */
	preferences_set_default_preferences(pref);
	
	if (preferences_get_preferences_from_xml_file(pref, preferences_file) != 0) {
		g_warning("Warning: Preference file %s is corrupt or does not exist. Creating file now.\n", preferences_file);
		preferences_write_preferences_to_xml_file(pref, preferences_file);
	}

	preferences_set_preferences_dialog();

	 /* Show window. All other widgets are automatically shown by GtkBuilder */
	 gtk_widget_show(client->mainwindow);

	if (initialise_eXosip(pref) < 0)
	{
		fprintf(stderr, "Cannot initialise SIP stack - Is port 5060 in use?\n");
		return -1;
	}
	else
	{
		imsua_set_message_display("UCT IMS Client Initialised", 1);
		eXosip_set_user_agent("UCT IMS Client");
		g_timeout_add (200, (GSourceFunc)get_exosip_events, NULL);
		// g_timeout_add (200, (GSourceFunc)tester, NULL);

	}

	// testGstreamer(client);
	
	 /* Start main loop */
	gtk_main();

	return 0;	
}
