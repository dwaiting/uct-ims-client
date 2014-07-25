/*
  The UCT IMS Client
  Copyright (C) 2006-2012 University of Cape Town
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
#include "callbacks.h"

void on_save_clicked (GtkButton *button, gpointer user_data)
{

	/* Save the preferences */
	preferences_get_preferences_dialog();

}


void on_register_button_clicked (GtkButton *button, gpointer user_data)
{
	printf ("Register Button Clicked\n");
	ims_send_register ();
	
}

void on_deregister_button_clicked (GtkButton *button, gpointer user_data)
{

	
	printf ("Deregister Button Clicked\n");
	ims_send_deregister_message ();


}

void on_call_button_clicked (GtkButton *button, gpointer user_data)
{
	if (state == IDLE)
	{	
		/* 
			Initiate a normal call to the URI in the URI entry field 
		*/
		ims_call_initiate((char*)gtk_entry_get_text(GTK_ENTRY(client->uri_entry)), 0);
	}
	else if(state == LOCAL_RINGING)
	{
		ims_call_answer();
	}
	else if(state == IN_CALL)
	{		
		// ims_call_reinvite(client);
	}
	
	printf ("Call Button Clicked\n");


}

void on_hangup_button_clicked (GtkButton *button, gpointer user_data)
{
	printf ("Hangup Button Clicked\n");
	terminate_call();
}


void on_register_as_alice_button_clicked (GtkButton *button, gpointer user_data)
{

	Preferences *pref = client->pref;

	strcpy(pref->name, "Alice");
	strcpy(pref->impu, "sip:alice@");
	strcat(pref->impu, pref->realm);
	strcpy(pref->impi, "alice@");
	strcat(pref->impi, pref->realm);
	strcpy(pref->password, "alice");

	strcpy(pref->xdms_username, "alice");
	strcpy(pref->xdms_password, "alice");
	ims_send_register(client);

	preferences_set_preferences_dialog();
}


void on_register_as_bob_button_clicked (GtkButton *button, gpointer user_data)
{
	Preferences *pref = client->pref;

	strcpy(pref->name, "Bob");
	strcpy(pref->impu, "sip:bob@");
	strcat(pref->impu, pref->realm);
	strcpy(pref->impi, "bob@");
	strcat(pref->impi, pref->realm);
	strcpy(pref->password, "bob");

	strcpy(pref->xdms_username, "bob");
	strcpy(pref->xdms_password, "bob");
	ims_send_register(client);

	preferences_set_preferences_dialog();

}

void on_call_alice_button_clicked (GtkButton *button, gpointer user_data)
{

	Preferences *pref = client->pref;

	if (state == IDLE)
	{
		/* Set the URI entry to sip:alice@[realm] */
		char uri[100];
		sprintf(uri, "sip:alice@%s", pref->realm);	
		gtk_entry_set_text(GTK_ENTRY(client->uri_entry), uri);

		/* Initiate a normal IMS call */
		ims_call_initiate(uri, 0);
	}
	else
	{
		set_display("Already in a call");
	}
}

void on_call_bob_button_clicked (GtkButton *button, gpointer user_data)
{
	Preferences *pref = client->pref;

	if (state == IDLE)
	{
		/* Set the URI entry to sip:bob@[realm] */
		char uri[100];
		sprintf(uri, "sip:bob@%s", pref->realm);	
		gtk_entry_set_text(GTK_ENTRY(client->uri_entry), uri);

		/* Initiate a normal IMS call */
		ims_call_initiate(uri, 0);
	}
	else
	{
		set_display("Already in a call");
	}
}

gboolean on_im_text_entry_key_press_event (GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
	
	const gchar *im_input = gtk_entry_get_text(GTK_ENTRY(client->im_text_entry));
	int length = strlen(im_input);
	
	/* 
		Detect the first key press and send a typing status message
	*/
	if(length == 0 && !(event-> keyval == 0xFF0D || event-> keyval == 0xFF8D))
	{
		// ims_send_instant_message_status (1);
	}


	/* 
		If enter is pressed without alt or control send message
		NOTE the keyval codes can be found in gdkkeysyms.h
	*/
	if ((event-> keyval == 0xFF0D || event-> keyval == 0xFF8D)&&(!(event->state & GDK_CONTROL_MASK)&&!(event->state & GDK_SHIFT_MASK)&&!(event->state & GDK_MOD1_MASK) ))	
	{
		printf("Enter Pressed!\n");

		ims_send_instant_message ((char*)gtk_entry_get_text(GTK_ENTRY(client->uri_entry)), (char*)gtk_entry_get_text(GTK_ENTRY(client->im_text_entry)));
		
	}
  
	return FALSE;
}


gboolean on_im_text_entry_key_release_event (GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
	const gchar *im_input = gtk_entry_get_text(GTK_ENTRY(client->im_text_entry));
	int length = strlen(im_input);
	
	/* Check for empty text entry */
	if (length  == 0 && !(event-> keyval == 0xFF0D || event-> keyval == 0xFF8D)) 
	{
		// ims_send_instant_message_status (0);
	}
  
	return FALSE;
}
