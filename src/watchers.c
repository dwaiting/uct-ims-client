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
#include "watchers.h"


gint watchers_subscribe_to_watcherinfo(int expires)
{

	if (!registered)
		return 1;
	
	osip_message_t *subscribe, *refresh;

	if (winfo_subscription_did < 0)
	{

		if(eXosip_subscribe_build_initial_request (&subscribe, pref->impu, pref->impu, imsua_add_lr_to_route(add_sip_scheme(pref->pcscf)), "presence.winfo", expires))
		{
			fprintf(stderr, "UCTIMSCLIENT: Error building presence.winfo subscribe message. Probably an invalid URI.\n");
			eXosip_unlock();
			return 1;
		}

		imsua_add_service_routes(&subscribe);

		eXosip_lock();
		int i = eXosip_subscribe_send_initial_request(subscribe);
		eXosip_unlock();	
	
		if(i < 0)
			fprintf(stderr, "UCTIMSCLIENT: Error sending subscribe to presence.winfo\n");
	}
	else
	{
		printf("Refreshing subscription to winfo.\n");

		eXosip_lock();
		if(eXosip_subscribe_build_refresh_request(winfo_subscription_did, &refresh) < 0)
		{
			fprintf(stderr, "Error building presence.winfo subscribe refresh request.\n");
			eXosip_unlock();
			return 1;
		}
		eXosip_unlock();

		osip_message_set_header(refresh, "Event", "presence.winfo");

		char expire_str[20];
		sprintf(expire_str, "%d", PRESENCE_EXPIRE); 
		osip_message_set_expires(refresh, expire_str);

		eXosip_lock();
		if(eXosip_subscribe_send_refresh_request(winfo_subscription_did, refresh) < 0)
		{
			fprintf(stderr, "Error sending presence.winfo subscribe refresh request.\n");
			eXosip_unlock();
			return 1;
		}
		eXosip_unlock();
	}
	

	imsua_set_message_display("SUBSCRIBE (presence.winfo)", 1);

}


void watchers_get_xml_elements(xmlNode* root_node)
{

	xmlNodePtr cur_node = NULL;
	xmlNode *contents = NULL;

	for (cur_node = root_node; cur_node; cur_node = cur_node->next) 
	{
	        if (cur_node->type == XML_ELEMENT_NODE) 
		{
			if((!xmlStrcmp(cur_node->name, (const xmlChar *)"watcher")))
			{

				watcherdata *watch = (watcherdata *)malloc(sizeof(watcherdata));

				if(watchers_find_watcher(xmlNodeGetContent(cur_node)) == NULL)
					watchers_add_watcher(watch);
				else
					watch = watchers_find_watcher(xmlNodeGetContent(cur_node));

				strcpy(watch->watcher, xmlNodeGetContent(cur_node));
				strcpy(watch->status, xmlGetProp(cur_node, "status"));
								
			}

	        }

		watchers_get_xml_elements(cur_node->children);
	}
}


void watchers_process_notify(eXosip_event_t *je)
{

	char *notify_str, *entity;
	osip_body_t *notify_body;
	int notify_len;
	xmlChar *txt;
   	xmlDocPtr doc;
	xmlNodePtr cur;

	// if we are not currently subscribed exit (otherwise osip seg-faults)
	if (je->ss_status != 2)
		return ;

	if(osip_message_get_body(je->request, 0, &notify_body))
	{
		return ;
	}

	osip_body_to_str(notify_body, &notify_str, &notify_len);

	if (notify_body == NULL)
		return ;

	notify_str[notify_len] = '\0';

	if(!(doc = xmlParseDoc(notify_str)))
	{
		fprintf(stderr, "UCTIMSCLIENT: Presence NOTIFY XML not parsed successfully.\n" );
		xmlFreeDoc( doc );
		return ;
	}

	if(!(cur = xmlDocGetRootElement(doc)))
	{
		fprintf( stderr, "UCTIMSCLIENT: XML document has no root element.\n" );
		xmlFreeDoc( doc );
		return ;
	}

	if( xmlStrcmp( cur->name, (const xmlChar *) "watcherinfo" ) ){
		fprintf( stderr, "UCTIMSCLIENT: XML document of the wrong type, root node != watcherinfo\n" );
		xmlFreeDoc( doc );
		return ;
	}

	watchers_get_xml_elements(cur);

	xmlFreeDoc( doc );

	watchers_display_watchers();


}

watcherdata *watchers_find_watcher(char *uri)
{

	GList *li;
	watcherdata *current;

	for(li = watcher_list; li != NULL; li = li->next)
	{
		current = (watcherdata *)li->data;

		if (!strcmp(current->watcher, uri))
		{
			return current;
			break;
		}
	}

	return NULL;

}


void watchers_add_watcher(watcherdata *watcher)
{
	if (g_list_length(watcher_list) > 9)
	{
		set_display("You can have a maximum of 5 watchers");
		return ;
	}

	watcherdata *new_watcher = (watcherdata*)malloc(sizeof(watcherdata));

	new_watcher = watcher;

	if(watchers_find_watcher(new_watcher->watcher) == NULL)	
	{
		watcher_list = g_list_append(watcher_list, (gpointer)new_watcher);
	}

}


void watchers_remove_watcher(char *uri)
{

	GList *li;
	watcherdata *current;

	current = watchers_find_watcher(uri);

	if (current != NULL)
	{

		watcher_list = g_list_remove(watcher_list, current);
		free(current);
	}
	else
		return ;

	watchers_display_watchers();
}


void watchers_remove_all_watchers()
{

	watcher_list = NULL;
	watchers_display_watchers();
}


void watchers_display_watchers()
{

	GList *li;
	watcherdata *current;
	GtkWidget *widget;

	int counter;
	char widget_name[50];

	/* First clear the list of watchers */
	for (counter = 1; counter < 6; counter++)
	{
		sprintf(widget_name, "watcher%d_uri", counter);
		GtkWidget *widget1 = lookup_widget(GTK_WIDGET(imsUA), widget_name);
		gtk_label_set_text(GTK_LABEL(widget1), "");
		
		sprintf(widget_name, "watcher%d_status", counter);
		GtkWidget *widget2 = lookup_widget(GTK_WIDGET(imsUA), widget_name);
		gtk_label_set_text(GTK_LABEL(widget2), "");

		sprintf(widget_name, "watcher%d_approve", counter);
		GtkWidget *widget3 = lookup_widget(GTK_WIDGET(imsUA), widget_name);
		gtk_widget_hide (widget3);

		sprintf(widget_name, "watcher%d_reject", counter);
		GtkWidget *widget4 = lookup_widget(GTK_WIDGET(imsUA), widget_name);
		gtk_widget_hide (widget4);
	}


	/* Now add the watchers and display the approve and reject buttons */
	counter = 1;
	for(li = watcher_list; li != NULL; li = li->next)
	{
		current = (watcherdata *)li->data;

		sprintf(widget_name, "watcher%d_uri", counter);
		GtkWidget *label = lookup_widget(GTK_WIDGET(imsUA), widget_name);
		gtk_label_set_text(GTK_LABEL(label), current->watcher);

		sprintf(widget_name, "watcher%d_status", counter);
		GtkWidget *label2 = lookup_widget(GTK_WIDGET(imsUA), widget_name);
		gtk_label_set_text(GTK_LABEL(label2), current->status);

		sprintf(widget_name, "watcher%d_approve", counter);
		GtkWidget *widget = lookup_widget(GTK_WIDGET(imsUA), widget_name);

		sprintf(widget_name, "watcher%d_reject", counter);
		GtkWidget *widget2 = lookup_widget(GTK_WIDGET(imsUA), widget_name);

		gtk_widget_show (widget);
		gtk_widget_show (widget2);
		
		counter ++;

		if (counter == 6)
			break ;

	}
}


int watchers_process_subscription_answered(eXosip_event_t *je)
{
	winfo_subscription_did = je->did;
}
