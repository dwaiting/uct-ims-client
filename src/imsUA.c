/*
  The UCT IMS Client
  Copyright (C) 2006 - 2008 - University of Cape Town
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
#include "imsUA.h"

int initialise_eXosip()
{
	int port = 5060;
	int i = eXosip_init();

	if (i != 0)  {
		fprintf(stderr, "Could not start exosip\n");
		return -1;
	}

	while(eXosip_listen_addr (IPPROTO_UDP, NULL, port, AF_INET, 0) != 0)
	{
		port++;
	}

	printf("UCT IMS Client - SIP port %d - Audio Port: %d - Video Port: %d\n\n", port, pref->local_audio_port, pref->local_video_port);

	return 0;

}


gint get_exosip_events(gpointer main_window)
{
	eXosip_event_t *je;
	char display[500] = "";
 	eXosip_lock();
	eXosip_unlock();

	/* Check for eXosip event - timeout after 50ms */
	if((je = eXosip_event_wait(0,50)) != NULL)
	{
		/* Uncomment the next line for debugging */
		 //fprintf(stderr, "Event type: %d %s\n", je->type, je->textinfo);

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
			else if (MSG_IS_INFO(je->request))
				common_process_info(je);
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
				/* Checks that message is actually destined for user by comparing sending TO field to IMPU */
				char sending_ui[50];
				strcpy(sending_ui,(((je->request)->to)->url)->username);
				strcat(sending_ui,"@");
				strcat(sending_ui,(((je->request)->to)->url)->host);

				char *temp;
				temp = strstr(pref->impu,":") + 1;

				if(strcmp(sending_ui,temp)==0)
				{
					char *null_string = NULL;
					ims_start_im_session(je, null_string);
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

				watchers_remove_all_watchers();

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


int main( int argc, char *argv[] )
{

	/* Set default preferences and retrieve existing preferences */
	preferences_set_default_preferences();
	preferences_get_preferences_from_xml_file("preferences.xml");

	state = IDLE;

	/* Initialize libraries */
	gtk_init (&argc, &argv);
	gst_init(NULL, NULL);
	if(msrp_init(events_msrp) < 0) {
		printf("Error initializing the MSRP library...\n");
	}

	/* Display the main GUI */
	imsUA = create_imsUA ();
	gtk_widget_show (imsUA);

	/* Setup our IPTV window */
	videoWin = create_videoWin();
	gtk_window_set_decorated(GTK_WINDOW(videoWin),FALSE);

	/* Prepare a GST pipeline for the background video */
	backgroundVideoPipeline = NULL;




	if (initialise_eXosip() < 0)
	{
		fprintf(stderr, "Could not initialise - Is port 5060 in use?\n");
	}
	else
	{
		eXosip_set_user_agent("UCT IMS Client");

		presence_get_buddy_list_from_file("buddylist");

		/* Check for incoming eXosip events every 200 ms */
		g_timeout_add (200, get_exosip_events, imsUA);

		/* run the main GUI */
		gtk_main ();

		/* if registered to a proxy deregister on exit*/
		if(registered == REGISTERED)//if client is registered
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

		presence_write_buddy_list_to_file("buddylist");

		preferences_write_preferences_to_xml_file("preferences.xml");

		msrp_quit();
		eXosip_quit();
		return 0;
	}
}
