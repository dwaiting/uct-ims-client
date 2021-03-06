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
#include "useful_methods.h"

char *add_sip_scheme (char *in)
{
	char *response = malloc(128);

	/* add sip: to URI if it is not there already */
	if (strstr(in, "sip:") || strstr(in, "sips:"))
	{
		strcpy(response, in);
		return response;
	}
	else
	{
		strcpy(response, "sip:");
		strcat(response, in);
		return response;
	}
}

void set_display(char *text)
{
	gtk_label_set_text(GTK_LABEL(client->call_status_label), text);
}

void imsua_set_message_display(char *text, int color)
{
	char buf[500];

	GtkTextBuffer *message_output_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW(client->log_view));
	GtkTextTag *tag;

	GtkTextTagTable *tag_table = gtk_text_buffer_get_tag_table(message_output_buffer);
	if(gtk_text_tag_table_lookup(tag_table,"bold") == NULL)
	{
		// Tag with weight bold and tag name "bold"
		gtk_text_buffer_create_tag (message_output_buffer,"bold","weight", PANGO_WEIGHT_BOLD, NULL);
	}
	if(gtk_text_tag_table_lookup(tag_table,"font") == NULL)
	{
		// Tag with font fixed and tag name "font". 
		gtk_text_buffer_create_tag (message_output_buffer, "font", "font", "fixed", NULL);
	}
	if(gtk_text_tag_table_lookup(tag_table,"italic") == NULL)
	{
		// Tag with font fixed and tag name "italic". 
		gtk_text_buffer_create_tag (message_output_buffer, "italic", "style", PANGO_STYLE_ITALIC, NULL);
	}
	if(gtk_text_tag_table_lookup(tag_table,"red") == NULL)
	{
		// Tag with font fixed and tag name "italic". 
		gtk_text_buffer_create_tag (message_output_buffer, "red", "foreground", "darkred", NULL);
	}
	if(gtk_text_tag_table_lookup(tag_table,"green") == NULL)
	{
		// Tag with font fixed and tag name "italic". 
		gtk_text_buffer_create_tag (message_output_buffer, "green", "foreground", "darkgreen", NULL);
	}


	//This writes the time stamp in bold for display on the screen
	GtkTextIter start_message_iter;
	gtk_text_buffer_get_end_iter(message_output_buffer, &start_message_iter);
	GtkTextMark *start_message_mark = gtk_text_buffer_create_mark(message_output_buffer,"start_message_mark",&start_message_iter,TRUE);
	
	sprintf(buf, "%s> ", imsua_get_time());
	gtk_text_buffer_insert(message_output_buffer,&start_message_iter,buf,strlen(buf));
	GtkTextIter end_message_iter;
	gtk_text_buffer_get_end_iter(message_output_buffer, &end_message_iter);
	gtk_text_buffer_get_iter_at_mark(message_output_buffer,&start_message_iter,start_message_mark);
	//Apply the BOLD tag to the selected text.
	gtk_text_buffer_apply_tag_by_name (message_output_buffer, "bold", &start_message_iter, &end_message_iter);
	
	//This writes the message portion is normal font for display on the screen
	gtk_text_buffer_get_end_iter(message_output_buffer, &start_message_iter);
	start_message_mark = gtk_text_buffer_create_mark(message_output_buffer,"start_message_mark",&start_message_iter,TRUE);
	sprintf(buf, "%s\n", text);
	gtk_text_buffer_insert(message_output_buffer,&start_message_iter,buf,strlen(buf));
	gtk_text_buffer_get_end_iter(message_output_buffer, &end_message_iter);
	gtk_text_buffer_get_iter_at_mark(message_output_buffer,&start_message_iter,start_message_mark);
	//Apply the normal FONT tag to the selected text.
	gtk_text_buffer_apply_tag_by_name (message_output_buffer, "font", &start_message_iter, &end_message_iter);

	if (color == 1)
		gtk_text_buffer_apply_tag_by_name (message_output_buffer, "red", &start_message_iter, &end_message_iter);
	else if (color == 2)
		gtk_text_buffer_apply_tag_by_name (message_output_buffer, "green", &start_message_iter, &end_message_iter);	
	
	// scroll to bottom of display
	GtkTextIter output_end_iter;
	gtk_text_buffer_get_end_iter(message_output_buffer, &output_end_iter);
	GtkTextMark *last_pos = gtk_text_buffer_create_mark (message_output_buffer, "last_pos", &output_end_iter, FALSE);	
	gtk_text_view_scroll_mark_onscreen (GTK_TEXT_VIEW(client->log_view), last_pos);


}

void imsua_append_text_to_display(char *text)
{	
	char display[500];
	// GtkWidget *label = lookup_widget(GTK_WIDGET(imsUA), "display");
	strcpy(display, gtk_label_get_text(GTK_LABEL(client->call_status_label)));
	strcat(display, text);
	gtk_label_set_text(GTK_LABEL(client->call_status_label), display);
}

void imsua_display_event_info(eXosip_event_t *je)
{

	char output[500];

	if (je->type == EXOSIP_REGISTRATION_FAILURE)
	{
		if (je->request && je->response)
			sprintf(output, "%s (for %s)", osip_message_get_reason_phrase(je->response), osip_message_get_method(je->request));
		else if(je->request)
			sprintf(output, "Registration Failure (for %s)", osip_message_get_method(je->request));
	}
	else if (je->type == EXOSIP_CALL_CLOSED)
	{
		return ;
		// sprintf(output, "%s", osip_message_get_method(je->request));
	}
	else if ((je->type == EXOSIP_CALL_MESSAGE_NEW) && (MSG_IS_BYE(je->request)))
	{
		sprintf(output, "%s", osip_message_get_method(je->request));
	}
	else if ((je->type == EXOSIP_CALL_INVITE) || (je->type == EXOSIP_CALL_REINVITE))
	{
		sprintf(output, "%s", osip_message_get_method(je->request));
	}
	else if ((je->type == EXOSIP_CALL_SERVERFAILURE) || (je->type == EXOSIP_CALL_GLOBALFAILURE) || (je->type == EXOSIP_CALL_REQUESTFAILURE))
	{
		sprintf(output, "%s (for %s)", osip_message_get_reason_phrase(je->response), osip_message_get_method(je->request));
	}
	else if (je->ack)
	{
		sprintf(output, "%s", osip_message_get_method(je->ack));

	}
	else if (je->request && je->response)
	{	
		sprintf(output, "%s (for %s)", osip_message_get_reason_phrase(je->response), osip_message_get_method(je->request));
	}
	else if (je->request)
	{
		sprintf(output, "%s", osip_message_get_method(je->request));
	}	
	else
		return	;
	
	imsua_set_message_display(output, 2);


}

void set_status_bar(char *text)
{
	// GtkWidget *label = lookup_widget(GTK_WIDGET(imsUA), "status_bar");

	gtk_label_set_text(GTK_LABEL(client->registration_status_label), text);
}

char *get_uuid(uuid_t u)
{
	int i;
	char *response = malloc(128);
/*
	printf("%8.8x-%4.4x-%4.4x-%2.2x%2.2x-", u.time_low, u.time_mid,
	u.time_hi_and_version, u.clock_seq_hi_and_reserved,
	u.clock_seq_low);
*/

	sprintf(response, "%8.8x-%4.4x-%4.4x-%2.2x%2.2x-%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x", (unsigned int)u.time_low, (unsigned int)u.time_mid,
		(unsigned int)u.time_hi_and_version, (unsigned int)u.clock_seq_hi_and_reserved, (unsigned int)u.clock_seq_low,
		(unsigned int)u.node[0], (unsigned int)u.node[1], (unsigned int)u.node[2], (unsigned int)u.node[3], (unsigned int)u.node[4], (unsigned int)u.node[5]);

/*
	for (i = 0; i < 6; i++)
	printf("%2.2x", u.node[i]);
*/

	return response;
}


int imsua_is_associated_uri(char *uri)
{
	int i;
	int found = 0;

	for (i = 0; i < num_associated_uris; i++)
	{
		if (strstr(ims_associated_uris[i], uri))
			found = 1;

	}

	return found;
}

char* add_lr_to_route(char *route)
{
	
	char *response = malloc(128);

	osip_route_t *rt;
	osip_route_init(&rt);

	if (osip_route_parse(rt,route) != 0)
	{
		printf("Route does not parse!\n");
		return NULL;
	}
	else
	{
		osip_uri_uparam_add(rt->url,osip_strdup("lr"),NULL);
		osip_route_to_str(rt,&response);
	}

	osip_route_free(rt);

	return response;
}


char* add_quotes(char *in)
{
	char *response = malloc(128);
	strcpy(response, "\"");
	strcat(response, in);
	strcat(response, "\"");
	return response;	
}

char *imsua_remove_quotes(char *text)
{
	char *response = strtok(text, "\"");
	return osip_strdup(response);
}

char* imsua_get_time()
{
	char hour[50];
	char min[50];
	char sec[50];
	char time_str[200];
	char *return_val;
	
	struct tm *now = NULL;
	time_t time_value = 0;
	time_value = time(NULL);
	
	now = localtime(&time_value);
	if(now->tm_hour < 10)
	{
		sprintf(hour,"0%d",now->tm_hour);
	}
	else
	{
		sprintf(hour,"%d",now->tm_hour);
	}
	if(now->tm_min < 10)
	{
		sprintf(min,"0%d",now->tm_min);
	}
	else
	{
		sprintf(min,"%d",now->tm_min);
	}
	if(now->tm_sec < 10)
	{
		sprintf(sec,"0%d",now->tm_sec);
	}
	else
	{
		sprintf(sec,"%d",now->tm_sec);
	}

	
	sprintf(time_str, "%s:%s:%s",hour, min,sec);
	return_val = strdup(time_str);

	return return_val;
}

int timeval_subtract (result, x, y) 
struct timeval *result, *x, *y;
{
	/* Perform the carry for the later subtraction by updating y. */
	if (x->tv_usec < y->tv_usec) 
	{
		int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
		y->tv_usec -= 1000000 * nsec;
		y->tv_sec += nsec;
	}
	if (x->tv_usec - y->tv_usec > 1000000) 
	{
		int nsec = (y->tv_usec - x->tv_usec) / 1000000;
		y->tv_usec += 1000000 * nsec;
		y->tv_sec -= nsec;
	}
	
	/* Compute the time remaining to wait.
	tv_usec  is certainly positive. */
	result->tv_sec = x->tv_sec - y->tv_sec;
	result->tv_usec = x->tv_usec - y->tv_usec;
	
	/* Return 1 if result is negative. */
	return x->tv_sec < y->tv_sec;
}

char *imsua_addpath(char *filename)
{

	char fullpath[100];
	char *return_val;
	
	sprintf(fullpath, "%s%s", filepath, filename);
	return_val = strdup(fullpath);

	return return_val;

}

int imsua_regex_match(char *string, char *pattern)
{
	int    status;
	regex_t re;

	if (regcomp(&re, pattern, REG_EXTENDED|REG_NOSUB) != 0) {
	        return(0);      /* Report error. */
	}

	status = regexec(&re, string, (size_t) 0, NULL, 0);	
 	regfree(&re);
	
	if (status != 0) 
	{
		return(0);      /* Report error. */
	}

	return(1);
}
