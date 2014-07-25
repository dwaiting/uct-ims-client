/*
  The UCT IMS Client
  Copyright (C) 2006 -2008 - University of Cape Town
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
#include "presence.h"

void presence_change_presentity()
{

	if (!registered)
	{
		set_display("Register before you change your status");
		return ;
	}

	char note[50];

	pref->presence_enabled = 1;

	GtkWidget *new_presentity_widget = lookup_widget(GTK_WIDGET(imsUA), "new_presentity");
	strcpy(note, gtk_entry_get_text(GTK_ENTRY(new_presentity_widget)));
	imsua_set_presentity_display(note);

	presence_publish_presentity("open", note);
}



void presence_publish_presentity(char *basic, char *note)
{

	char presence_expire_str[10];

	if (!registered)
		return ;

	osip_message_t *publish;
	char xmlbody[4096];

	sprintf(xmlbody, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"
			"<presence xmlns=\"urn:ietf:params:xml:ns:pidf\"\r\n"
        		"xmlns:im=\"urn:ietf:params:xml:ns:pidf:im\"\r\n"
       			"entity=\"%s\">\r\n"
			"\t<tuple id=\"UCTIMSClient\">\r\n"
			"\t\t<status>\r\n"
			"\t\t\t<basic>%s</basic>\r\n"
			"\t\t</status>\r\n"
			"\t\t<note>%s</note>\r\n"
			"\t</tuple>\r\n"
			"</presence>\r\n", pref->impu, basic, note);

	sprintf(presence_expire_str, "%d", PRESENCE_EXPIRE);

	eXosip_lock();
	if(eXosip_build_publish(&publish, pref->impu, pref->impu, imsua_add_lr_to_route(add_sip_scheme(pref->pcscf)), "presence", presence_expire_str, "application/pidf+xml", xmlbody))
	{
		fprintf(stderr, "Error building publish\n");
		eXosip_unlock();
		return ;
	}
	eXosip_unlock();

	imsua_add_service_routes(&publish);

	eXosip_lock();
	if(eXosip_publish(publish, "NULL"))
		set_display("Error publishing presence\n");
	else
		imsua_set_message_display("PUBLISH (presence)", 1);

	eXosip_unlock();

}


void presence_get_xml_elements(xmlNode* root_node, char *entity)
{

	buddydata *notify_buddy;

	if(!(notify_buddy = presence_find_buddy(entity)))
		return ;

	xmlNode *cur_node = NULL;
	xmlNode *contents = NULL;

	for (cur_node = root_node; cur_node; cur_node = cur_node->next) 
	{
	        if (cur_node->type == XML_ELEMENT_NODE) 
		{
			if((!xmlStrcmp(cur_node->name, (const xmlChar *)"basic")))
			{
				if (contents = cur_node->children)
					strcpy(notify_buddy->basic, contents->content);
			}

			if((!xmlStrcmp(cur_node->name, (const xmlChar *)"note")))
			{
				if (contents = cur_node->children)
					strcpy(notify_buddy->note, contents->content);
			}

	        }

		presence_get_xml_elements(cur_node->children, entity);
	}
}



void presence_process_notify(eXosip_event_t *je)
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

	// osip_body_to_str(notify_body, &notify_str, &notify_len);

	if (notify_body == NULL)
		return ;

	// notify_str[notify_len] = '\0';

	notify_str = notify_body->body;


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

	if( xmlStrcmp( cur->name, (const xmlChar *) "presence" ) ){
		fprintf( stderr, "UCTIMSCLIENT: XML document of the wrong type, root node != presence\n" );
		xmlFreeDoc( doc );
		return ;
	}

	entity = xmlGetProp(cur, "entity");
	presence_get_xml_elements(cur, entity);
	presence_display_buddies();


	xmlFreeDoc( doc );

	buddydata *notify_buddy = presence_find_buddy(entity);
	
	// if this is my URI then change my status
	if (notify_buddy && !strcmp(entity, pref->impu))
		imsua_set_presentity_display(notify_buddy->note);

}


buddydata *presence_find_buddy(char *buddy)
{

	GList *li;
	buddydata *current;

	for(li = buddy_list; li != NULL; li = li->next)
	{
		current = (buddydata *)li->data;

		if (!strcmp(current->buddy, buddy))
		{
			return current;
			break;
		}
	}

	return NULL;

}


void presence_add_buddy(char *buddy)
{
	if (g_list_length(buddy_list) > 9)
	{
		set_display("You can have a maximum of 10 buddies");
		return ;
	}

	buddydata *new_buddy = (buddydata*)malloc(sizeof(buddydata));

	strcpy(new_buddy->buddy, buddy);
	strcpy(new_buddy->basic, "closed");
	strcpy(new_buddy->note, "");
	new_buddy->subscription_did = -1;

	if(presence_find_buddy(new_buddy->buddy) == NULL)	
	{

		buddy_list = g_list_append(buddy_list, (gpointer)new_buddy);
		presence_display_buddies();

		presence_subscribe_to_presentity(new_buddy->buddy, 600);
	}

}


void presence_remove_buddy(char *buddy)
{

	GList *li;
	buddydata *current;

	current = presence_find_buddy(buddy);

	if (current != NULL)
	{
		if (registered == REGISTERED)
			presence_subscribe_to_presentity(current->buddy, 0);

		buddy_list = g_list_remove(buddy_list, current);
		free(current);
	}
	else
		return ;


	presence_display_buddies();
}



void presence_subscribe_to_presentity(char *buddy, int expires)
{

	if (!registered)
		return ;
	
	osip_message_t *subscribe, *refresh;
	buddydata *current;
	int i;

	if (((current = presence_find_buddy(buddy)) != NULL) && (current->subscription_did > 0))
	{
		printf("Refreshing presence subscription for buddy: %s DID: %d\n", current->buddy, current->subscription_did);

		eXosip_lock();

		if(eXosip_subscribe_build_refresh_request(current->subscription_did, &refresh) < 0)
		{
			fprintf(stderr, "Error building subscription refresh\n");
			eXosip_unlock();
			return ;
		}
		eXosip_unlock();

		osip_message_set_header(refresh, "Event", "presence");

		char expire_str[20];
		sprintf(expire_str, "%d", PRESENCE_EXPIRE); 
		osip_message_set_expires(refresh, expire_str);
		
		eXosip_lock();
		if((i = eXosip_subscribe_send_refresh_request(current->subscription_did, refresh)) < 0)
		{
			fprintf(stderr, "Error sending subscription refresh\n");
			eXosip_unlock();
			return ;
		}

		eXosip_unlock();
	}
	else
	{

		if(eXosip_subscribe_build_initial_request (&subscribe, buddy, pref->impu, imsua_add_lr_to_route(add_sip_scheme(pref->pcscf)), "presence", expires))
		{
			fprintf(stderr, "UCTIMSCLIENT: Error building presence subscribe message. Probably an invalid URI.\n");
			eXosip_unlock();
			return ;
		}

		imsua_add_service_routes(&subscribe);

		eXosip_lock();
		i = eXosip_subscribe_send_initial_request(subscribe);	
		eXosip_unlock();
	}


	if(i < 0)
		fprintf(stderr, "UCTIMSCLIENT: Error sending subscribe\n");
	else
		imsua_set_message_display("SUBSCRIBE (presence)", 1);
	

}


gint presence_subscribe_to_all_presentities(int expires)
{
	
	GList *li;
	void *current;
	
	for(li = buddy_list; li != NULL; li = li->next)
	{
		current = (Call *)li->data;

		presence_subscribe_to_presentity(current, expires);
	}

	return 1;

}


void presence_display_buddies()
{

	GList *li;
	buddydata *current;
	
	imsua_clear_buddy_list_display();

	for(li = buddy_list; li != NULL; li = li->next)
	{
		current = (buddydata *)li->data;
		imsua_append_text_to_buddy_list_display(current->buddy,current->basic,current->note);
	}
}


void presence_get_buddy_list_from_file(char *filename)
{

	FILE *fp;
	
	if (filename == NULL)
		filename = "buddylist";

	if((fp = fopen(imsua_addpath(filename),"rb")) == NULL)
	{
		printf("UCTIMSCLIENT WARNING: Error opening buddy list file for reading: %s\n", imsua_addpath(filename));
		return ;
	}

	char input[50];

	while(fgets(input, 50, fp) != NULL)
	{
		buddydata *new_buddy = (buddydata*)malloc(sizeof(buddydata));
	
		if(presence_find_buddy(preferences_remove_newline(input)) == NULL)
		{
			presence_add_buddy(preferences_remove_newline(input));

		}

	}

	fclose(fp);

	presence_display_buddies();
}



void presence_write_buddy_list_to_file(char *filename)
{

	FILE *fp;

	if (filename == NULL)
		filename = "buddylist";

	if((fp = fopen(imsua_addpath(filename),"wb")) == NULL) 
	{ 
		printf("UCTIMSCLIENT WARNING: Error opening buddy list file for writing: %s\n", imsua_addpath(filename));
 		return ;
	} 

	GList *li;
	buddydata *current;
	
	for(li = buddy_list; li != NULL; li = li->next)
	{
		current = (buddydata *)li->data;
		fprintf(fp, "%s\n", current->buddy);
	}

	fclose(fp);

}


int presence_process_subscription_answered(eXosip_event_t *je)
{

	osip_to_t *to_header;
	osip_uri_t *to_uri;
	char *to_str;
	buddydata *current;

	to_header = osip_message_get_to(je->response);
	to_uri = osip_from_get_url(to_header);

	osip_uri_to_str(to_uri, &to_str);

	if ((current = presence_find_buddy(to_str)) != NULL)
	{
		// fprintf(stderr, "Setting dialog ID of buddy %s to %d\n", to_str, je->did);
		current->subscription_did = je->did;
		return 0;
	}
	else
	{	
		fprintf(stderr, "Received 200 OK for a buddy subscription - but I have no record of this buddy\n");
		return -1;
	}	
}


