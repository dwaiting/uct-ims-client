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
#include "media.h"



sdp_media_t *media_eXosip_get_video_media (sdp_message_t * sdp)
{
  int pos = 0;
  sdp_media_t *med = (sdp_media_t *) osip_list_get (&sdp->m_medias, 0);

  while (med != NULL)
    {
      if (med->m_media != NULL && osip_strcasecmp (med->m_media, "video") == 0)
        return med;
      pos++;
      med = (sdp_media_t *) osip_list_get (&sdp->m_medias, pos);
    }

  return NULL;
}



int message_extract_call_info(Call *call, osip_message_t *message)
{
	Preferences *pref = client->pref;
	/* Get the To Header */
	osip_to_t *to_header;
	osip_uri_t *to_uri;
	char *to_str;
	osip_content_type_t *content_type;

	to_header = osip_message_get_to(message);
	to_uri = osip_from_get_url(to_header);

	osip_uri_to_str(to_uri, &to_str);
	strcpy(call->to_uri, to_str);

	/* Get from name and URI */
	osip_from_t *from;
	if (from = osip_message_get_from(message))
	{
		char *from_uri_string, *displayname;
		osip_uri_t *from_uri;

		if(from_uri = osip_from_get_url(from))
		{
			osip_uri_to_str(from_uri, &from_uri_string);
			strcpy(call->from_uri, from_uri_string);
		}
		else
			strcpy(call->from_uri, "Unspecified caller");
	
		if(displayname = osip_from_get_displayname(from))
			strcpy(call->from_name, displayname);
		else
			strcpy(call->from_name, "");
	}


	/* Get remote media settings */
	sdp_message_t *remote_sdp = NULL;

	if ((remote_sdp = eXosip_get_sdp_info(message)) != NULL)
	{
		/* get remote media IP addresses */
		sdp_connection_t *audio_connection, *video_connection;
		
		if(audio_connection = eXosip_get_audio_connection (remote_sdp))
		{
		
			/* Not available in this version of eXosip */
			// video_connection = eXosip_get_video_connection (remote_sdp);

			snprintf (call->remote_audio_ip, 50, "%s", audio_connection->c_addr);
			snprintf (call->remote_video_ip, 50, "%s", audio_connection->c_addr);
		}
		else
		{
			strcpy(call->remote_audio_ip, "0.0.0.0");
			strcpy(call->remote_video_ip, "0.0.0.0");
		}
		
		/* Get remote media ports */
		sdp_media_t *remote_audio_media, *remote_video_media;

		if(remote_audio_media = eXosip_get_audio_media (remote_sdp))
		{
			call->audio_supported = 1;
			call->remote_audio_port = atoi (remote_audio_media->m_port);
		}
		else
			call->remote_audio_port = 0;
	
		if(remote_video_media = media_eXosip_get_video_media (remote_sdp))
		{
			call->video_supported = 1;
			call->remote_video_port = atoi (remote_video_media->m_port);
		}
		else
			call->remote_video_port = 0;

		//get SDP info for MSRP messages
		sdp_media_t* msrp_media;
		msrp_media = eXosip_get_media(remote_sdp,"message");
		if(msrp_media)
		{
			call->im_supported = 1;
			call->remote_msrp_port = atoi(msrp_media->m_port);
						
			sdp_attribute_t *a_attr;
			
			int a = 0;
			while (!osip_list_eol (&msrp_media->a_attributes, a))
			{	
				a_attr = osip_list_get (&msrp_media->a_attributes, a);
				if(!strcmp(a_attr->a_att_field, "path"))
				{
					call->remote_msrp_path = a_attr->a_att_value;
				}
				a ++;
			}
		}
	}

	
	if ((content_type = osip_message_get_content_type (message)) != NULL)
	{
		if (!strcmp(content_type->type, "message") && !strcmp(content_type->subtype, "external-body"))
		{
			call->content_indirected = 1;

			osip_generic_param_t *uri;

			if (osip_content_type_param_get_byname(content_type, "URL", &uri) == 0)
				strcpy(call->content_indirection_uri, imsua_remove_quotes(uri->gvalue));
		}
	}


	return 0;
}


void media_start_session(eXosip_event_t *je)
{
	Preferences *pref = client->pref;
	GstState state;

	Call *ca;

	if (find_call(je->did, &ca) < 0)
		return ;

	ca->call_is_active = 1;

	/* Destroy any existing pipeline */
	if (GST_IS_ELEMENT(ca->ringingPipeline))
	{
		destroyRingingPipeline(ca);
	}

	if (GST_IS_ELEMENT(backgroundVideoPipeline))
	{
		destroyBackgroundVideoPipeline();
	}
	
	/* Initialise the media pipelines if they are not already playing */
	/* FUTURE WORK FOR RICHARD: otherwise simply change properties */

	if (GST_IS_ELEMENT(ca->audioRxPipeline))
	{
		printf("Destroying existing audio Rx pipeline\n");
		destroyAudioRxPipeline(ca);
	}

	if (GST_IS_ELEMENT(ca->audioTxPipeline))
	{
		printf("Destroying existing audio Tx pipeline\n");
		destroyAudioTxPipeline(ca);
	}

	if (GST_IS_ELEMENT(ca->videoRxPipeline))
	{
		printf("Destroying existing video Rx pipeline\n");
		destroyVideoRxPipeline(ca);
	}

	if (GST_IS_ELEMENT(ca->videoTxPipeline))
	{
		printf("Destroying existing video Tx pipeline\n");
		destroyVideoTxPipeline(ca);
	}	/* Check whether we have an IPTV session or a normal call */


	/* Must send a small packet to the NAT to open it up from the inside */
	// media_open_nat(ca->remote_audio_ip, ca->remote_audio_port, ca->local_audio_port);
	// media_open_nat(ca->remote_video_ip, ca->remote_video_port, ca->local_video_port);

	// fprintf(stderr, "Starting media session on the following ports:\n  video Tx: %d video Rx: %d\n  Audio Tx: %d Audio Rx: %d\n", ca->remote_video_port, ca->local_video_port, ca->remote_audio_port, ca->local_audio_port);

	/* Start our audio RTP streams */
	if (ca->audio_supported)
	{
		initialiseAudioRxPipeline(ca);
		initialiseAudioTxPipeline(ca);
	}

	printf("Video enabled %d  Video supported %d \n", pref->video_enabled, ca->video_supported);

	/* Start our video RTP streams */
	if (pref->video_enabled && ca->video_supported)
	{
		initialiseVideoTxPipeline(ca);
		initialiseVideoRxPipeline(ca);		
	}

	/* Send the buffer message if there is one */
	if (ca->im_supported)
	{

		if (ca->caller && strcmp(client->im_buffer, ""))
		{
			ims_send_instant_message(ca->to_uri, client->im_buffer);
			strcpy(client->im_buffer, "");
		}
	
	}
}

int media_open_nat(char *their_ip, int their_port, int my_port)
{

	int sockfd;
	struct sockaddr_in their_addr, my_addr; // connector's address information
	struct hostent *he;
	int numbytes;

	char *message = "This packet opens port restricted cone NATs\0";
/*
	if ((he=gethostbyname(their_ip) == NULL) {  // get the host info
		herror("gethostbyname");
		return FALSE;
	}
*/
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("socket");
		return FALSE;
	}

	my_addr.sin_family = AF_INET;         // host byte order
	my_addr.sin_port = htons(my_port);     // short, network byte order
	my_addr.sin_addr.s_addr = INADDR_ANY;
	memset(my_addr.sin_zero, '\0', sizeof my_addr.sin_zero);

	bind(sockfd, (struct sockaddr *)&my_addr, sizeof my_addr);

	their_addr.sin_family = AF_INET;     // host byte order
	their_addr.sin_port = htons(their_port); // short, network byte order
	// their_addr.sin_addr = *((struct in_addr *)he->h_addr);

	their_addr.sin_addr.s_addr = inet_addr(their_ip);

	memset(their_addr.sin_zero, '\0', sizeof their_addr.sin_zero);

	if ((numbytes = sendto(sockfd, message, strlen(message), 0, (struct sockaddr *)&their_addr, sizeof their_addr)) == -1) {
		// printf("WARNING: Failed to send NAT opening ping\n");
		close(sockfd);
        	return FALSE;
	}

	// printf("sent %d bytes to %s\n", numbytes, inet_ntoa(their_addr.sin_addr));

	close(sockfd);

	return TRUE;

}

