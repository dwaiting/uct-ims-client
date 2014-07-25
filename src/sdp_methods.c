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
#include "sdp_methods.h"


int sdp_complete_ims(osip_message_t *message, osip_message_t **answer, Call *ca, int session_type)
{

	sdp_message_t *remote_sdp;
  	sdp_media_t *remote_med;
  	char buf[4096];
  	int pos;
	Preferences *pref = client->pref;

	sdp_message_init(&remote_sdp);
	remote_sdp = NULL;

	if (message)
	 	remote_sdp = eXosip_get_sdp_info (message);

	/* if we can't find an SDP body in an INVITE then suggest one to the caller */
 	if ((message == NULL) || (MSG_IS_INVITE(message) && (remote_sdp == NULL)))
    	{

		// g_warning("Can't find SDP body in message... suggesting my own\n");

		char qos_info[200];
		char audio_codecs[20];
		char audio_rtpmaps[200];
		char video_codecs[400] = "";
		char msrp_info[400] = "";

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

	
		/* Assign our codec preferences */
		switch (pref->audio_codec_primary)
		{
			case 0 : strcpy(audio_codecs, "0"); strcpy(audio_rtpmaps, "a=rtpmap:0 PCMU/8000\r\n"); break;
			case 1 : strcpy(audio_codecs, "8"); strcpy(audio_rtpmaps, "a=rtpmap:8 PCMA/8000\r\n"); break;
			case 2 : strcpy(audio_codecs, "3"); strcpy(audio_rtpmaps, "a=rtpmap:3 GSM/8000\r\n");break;
			case 3 : strcpy(audio_codecs, "14"); strcpy(audio_rtpmaps, "a=rtpmap:14 MPA/90000\r\n");break;
			default : strcpy(audio_codecs, "0"); strcpy(audio_rtpmaps, "a=rtpmap:0 PCMU/8000\r\n"); break;
		}
		switch (pref->audio_codec_secondary)
		{
			case 0 : strcat(audio_codecs, " 0"); strcat(audio_rtpmaps, "a=rtpmap:0 PCMU/8000\r\n"); break;
			case 1 : strcat(audio_codecs, " 8"); strcat(audio_rtpmaps, "a=rtpmap:8 PCMA/8000\r\n"); break;
			case 2 : strcat(audio_codecs, " 3"); strcat(audio_rtpmaps, "a=rtpmap:3 GSM/8000\r\n"); break;
			case 3 : strcpy(audio_codecs, " 14"); strcpy(audio_rtpmaps, "a=rtpmap:14 MPA/90000\r\n");break;
			default : break;
		}
		// strcat(audio_codecs, " 101"); 
		// strcat(audio_rtpmaps, "a=rtpmap:101 telephone-event/8000\r\na=fmtp:101 0-11\r\n");

		
		if(pref->session_im_enabled)
		{
			snprintf (msrp_info, 4096,
			"m=message %d TCP/MSRP\r\n"
			"a=accept-types:text/plain\r\n"
			"a=path:%s\r\n", pref->local_msrp_port, msrp_endpoint_get_from_fullpath(client->msrp_endpoint));
		}
	
		if (pref->video_enabled)
		{
			sprintf(video_codecs, 
			"m=video %d RTP/AVP 96\r\n"
			"b=AS:128\r\n"
			"%s"
			"a=rtpmap:96 VP8/90000\r\n"
			"a=fmtp:96 profile-level-id=0\r\n", pref->local_video_port, qos_info);
		}

		/* Check if is a normal session or IM only */
		if (session_type != 1)
		{
			snprintf (buf, 4096,
			"v=0\r\n"
			"o=- 0 0 IN IP4 %s\r\n"
			"s=IMS Call\r\n"
			"c=IN IP4 %s\r\n"
			"t=0 0\r\n"
			"m=audio %d RTP/AVP %s\r\n"
			"b=AS:64\r\n"
			"%s"
			"%s"
			"%s"
			"%s", pref->local_audio_ip, pref->local_audio_ip, pref->local_audio_port, audio_codecs, audio_rtpmaps, qos_info, video_codecs,msrp_info);
		}
		else if (session_type == 1)
		{
			snprintf (buf, 4096,
			"v=0\r\n"
			"o=- 0 0 IN IP4 %s\r\n"
			"s=IMS Call\r\n"
			"c=IN IP4 %s\r\n"
			"t=0 0\r\n"
			"%s", pref->local_msrp_ip, pref->local_msrp_ip, msrp_info);
		
		}

		osip_message_set_body (*answer, buf, strlen(buf));
	  	osip_message_set_content_type (*answer, "application/sdp");
	  	sdp_message_free (remote_sdp);

		// if there is no QoS info in the invite we can assume everything is set up okay
		if (ca)
		{
			ca->qos_remote_preference = QOS_NONE;
			ca->qos_local_state = SENDRECV;
		}

	      	return 1;
    	}
	else if (remote_sdp == NULL)
	{
		sdp_message_free (remote_sdp);
		return 1;
	}

  	snprintf (buf, 4096,
        "v=0\r\n"
        "o=- 0 0 IN IP4 %s\r\n"
        "s=IMS Session\r\n" "c=IN IP4 %s\r\n" "t=0 0\r\n", pref->local_audio_ip, pref->local_audio_ip);

  	pos = 0;

  	while (!osip_list_eol (&remote_sdp->m_medias, pos))
  	{
	
      	char payloads[128];
      	int pos2;

      	memset (payloads, '\0', sizeof (payloads));
      	remote_med = (sdp_media_t *) osip_list_get (&remote_sdp->m_medias, pos);

      	if (0 == osip_strcasecmp (remote_med->m_media, "audio"))
        {
        	pos2 = 0;
          
		while (!osip_list_eol (&remote_med->m_payloads, pos2))
            	{
              		char *audio_codec = (char *) osip_list_get (&remote_med->m_payloads, pos2);
              		
			if (audio_codec != NULL && (!osip_strcasecmp (audio_codec, "0") || !osip_strcasecmp (audio_codec, "8") || !osip_strcasecmp(audio_codec, "3") || !osip_strcasecmp(audio_codec, "14") || !osip_strcasecmp (audio_codec, "101")))
                	{
                  		strcat (payloads, audio_codec);
                  		strcat (payloads, " ");
                	}

              		pos2++;
            	}
        
		strcat (buf, "m=");
          	strcat (buf, remote_med->m_media);

		if (pos2 == 0 || payloads[0] == '\0')
            	{
			g_warning("I found an audio component in this call but did not recognise any of the codecs\n");
              		strcat (buf, " 0 RTP/AVP \r\n");
          	} 
		else
            	{
			strcat (buf, " ");

			char local_audio_port_str[20];
              		sprintf(local_audio_port_str, "%d", ca->local_audio_port);
	      		strcat (buf, local_audio_port_str);
	      		strcat (buf, " RTP/AVP ");
              		strcat (buf, payloads);
              		strcat (buf, "\r\n");

			int pos3 = 0;
			sdp_bandwidth_t *b_bandw;

			while (!osip_list_eol (&remote_med->b_bandwidths, pos3))
			{	
				b_bandw = osip_list_get (&remote_med->b_bandwidths, pos3);
				strcat(buf, "b=");
				strcat(buf, b_bandw->b_bwtype);
				strcat(buf, ":");
				strcat(buf, b_bandw->b_bandwidth);
				strcat(buf, "\r\n");
				pos3 ++;
			}

			char qos_info[200] = "";

			pos3 = 0;
			sdp_attribute_t *a_attrib;
			
			while (!osip_list_eol (&remote_med->a_attributes, pos3))
			{
				a_attrib = osip_list_get (&remote_med->a_attributes, pos3);

				// check what level of QoS is required
				if (strstr(a_attrib->a_att_field, "des"))
				{
					// echo back their QoS state
					if (strstr(a_attrib->a_att_value,"local"))
					{
						if (strstr(a_attrib->a_att_value,"mandatory local sendrecv"))
						{
							ca->qos_remote_preference = QOS_MANDATORY;
							strcat(qos_info, "a=des:qos mandatory remote sendrecv\r\n");
							// ask the remote terminal to inform us when QoS reservation is complete

							if ((ca->caller == 0) && (ca->qos_remote_state != SENDRECV))
								strcat(qos_info, "a=conf:qos remote sendrecv\r\n");
						}
						else if (strstr(a_attrib->a_att_value,"optional local sendrecv"))
						{
							ca->qos_remote_preference = QOS_OPTIONAL;
							strcat(qos_info, "a=des:qos optional remote sendrecv\r\n");
						}
						else if (strstr(a_attrib->a_att_value,"none local sendrecv"))
						{
							ca->qos_remote_preference = QOS_NONE;
							strcat(qos_info, "a=des:qos none remote sendrecv\r\n");
						}
					}
			
					// tell the other UA what level of QoS we require	
					if (strstr(a_attrib->a_att_value,"remote"))
					{
						if (pref->qos_strength == QOS_MANDATORY)
							strcat(qos_info, "a=des:qos mandatory local sendrecv\r\n");
						else if (pref->qos_strength == QOS_OPTIONAL)
							strcat(qos_info, "a=des:qos optional local sendrecv\r\n");
						else if (pref->qos_strength == QOS_NONE)
							strcat(qos_info, "a=des:qos none local sendrecv\r\n");
					}

					// if QoS is e2e agree with other UA
					if (strstr(a_attrib->a_att_value, "e2e"))
					{
						strcat(qos_info, "a=des:");
						strcat(qos_info, a_attrib->a_att_value);
						strcat(qos_info, "\r\n");

						if ((ca->caller == 0) && (ca->qos_remote_state != SENDRECV))
							strcat(qos_info, "a=conf:qos e2e send\r\n");
					}	

				}
				

				// check what type of QoS we have currently
				if (strstr(a_attrib->a_att_field, "curr"))
				{
					if (strstr(a_attrib->a_att_value,"local none"))
					{
						ca->qos_remote_state = NONE;
						strcat(qos_info, "a=curr:qos remote none\r\n");
					}
					
					if (strstr(a_attrib->a_att_value,"local sendrecv"))
					{	
						ca->qos_remote_state = SENDRECV;
						strcat(qos_info, "a=curr:qos remote sendrecv\r\n");
					}

					if (strstr(a_attrib->a_att_value,"remote"))
					{
						if (ca->qos_local_state == NONE)
							strcat(qos_info, "a=curr:qos local none\r\n");
						
						if (ca->qos_local_state == SENDRECV)
							strcat(qos_info, "a=curr:qos local sendrecv\r\n");
					}
		
					// if QoS is end-to-end just agree with other UA
					if (strstr(a_attrib->a_att_value,"e2e"))
					{
						if (ca->qos_local_state == SENDRECV)
						{
							strcat(qos_info, "a=curr:qos e2e sendrecv\r\n");
							ca->qos_remote_state = SENDRECV;
						}
						else
						{
							strcat(qos_info, "a=curr:");
							strcat(qos_info, a_attrib->a_att_value);
							strcat(qos_info, "\r\n");

							if (strstr(a_attrib->a_att_value,"sendrecv"))
							{
								ca->qos_remote_state = SENDRECV;
								ca->qos_local_state = SENDRECV;
							}
						}
	
					}

				}
		
				// the other terminal requires confirmation when QoS reservation is complete		
				if (strstr(a_attrib->a_att_field, "conf") && (strstr(a_attrib->a_att_value,"remote sendrecv") || strstr(a_attrib->a_att_value,"e2e send")))
					ca->qos_confirm = 1;

				pos3++;
			
			}

			strcat(buf, qos_info);

			/*
			if (NULL != strstr (payloads, "101 ")) 
			{
                		strcat (buf, "a=rtpmap:101 telephone-event/8000\r\n");
				strcat (buf, "a=fmtp:101 0-15\r\n");
	      		}
			*/

               		if (NULL != strstr (payloads, " 0 ") || (payloads[0] == '0' && payloads[1] == ' '))
	              		strcat (buf, "a=rtpmap:0 PCMU/8000\r\n");

	       		if (NULL != strstr (payloads, " 8 ") || (payloads[0] == '8' && payloads[1] == ' '))
	              		strcat (buf, "a=rtpmap:8 PCMA/8000\r\n");

			if (NULL != strstr (payloads, " 3 ") || (payloads[0] == '3' && payloads[1] == ' '))
	              		strcat (buf, "a=rtpmap:3 GSM/8000\r\n");
			
			if (NULL != strstr (payloads, " 14 ") || (payloads[0] == '1' && payloads[1] == '4'))
	              		strcat (buf, "a=rtpmap:14 MPA/90000\r\n");		

			/* Choose a codec for this call */
			switch (payloads[0])
			{
				case '0' : ca->audio_codec = 0; break;
				case '3' : ca->audio_codec = 3; break;
				case '8' : ca->audio_codec = 8; break;
				case '1' : ca->audio_codec = 14; break;
				default : ca->audio_codec = 0; break;
			}
      
           	 }
      	}
	else if (0 == osip_strcasecmp (remote_med->m_media, "video"))
        {

		/* Negotiate our video codecs */
        	int num_codecs = 0;
		sdp_attribute_t *a_attrib;
		char h263_avp_value[50];

		pos2 = 0;

		while (!osip_list_eol (&remote_med->a_attributes, pos2))
		{
			a_attrib = osip_list_get (&remote_med->a_attributes, pos2);

			if (strstr(a_attrib->a_att_field, "rtpmap"))
			{

				if (strstr(a_attrib->a_att_value, "vp8") || strstr(a_attrib->a_att_value, "VP8"))
				{
					strcpy(h263_avp_value, a_attrib->a_att_value);
					strcat(payloads, strtok(a_attrib->a_att_value, " "));

					num_codecs++;
				}
			}

			pos2++;
			
		}

		strcat (buf, "m=");
          	strcat (buf, remote_med->m_media);

		if ((num_codecs == 0) || (!pref->video_enabled))
            	{
			g_warning("I found a video component in this call but either I did not recognise any of the codecs or video is disabled\n");
              		strcat (buf, " 0 RTP/AVP \r\n");
          	} 
		else
            	{

              		strcat (buf, " ");

			char local_video_port_str[20];
              		sprintf(local_video_port_str, "%d", ca->local_video_port);
	      		strcat (buf, local_video_port_str);
	      		strcat (buf, " RTP/AVP ");
              		strcat (buf, payloads);
              		strcat (buf, "\r\n");

			int pos3 = 0;
			sdp_bandwidth_t *b_bandw;

			while (!osip_list_eol (&remote_med->b_bandwidths, pos3))
			{	
				b_bandw = osip_list_get (&remote_med->b_bandwidths, pos3);
				strcat(buf, "b=");
				strcat(buf, b_bandw->b_bwtype);
				strcat(buf, ":");
				strcat(buf, b_bandw->b_bandwidth);
				strcat(buf, "\r\n");
				pos3 ++;
			}

			char qos_info[200] = "";

			pos3 = 0;
			sdp_attribute_t *a_attrib;
			
			while (!osip_list_eol (&remote_med->a_attributes, pos3))
			{
				a_attrib = osip_list_get (&remote_med->a_attributes, pos3);

				// check what level of QoS is required
				if (strstr(a_attrib->a_att_field, "des"))
				{
					// echo back their QoS state
					if (strstr(a_attrib->a_att_value,"local"))
					{
						if (strstr(a_attrib->a_att_value,"mandatory local sendrecv"))
						{
							ca->qos_remote_preference = QOS_MANDATORY;
							strcat(qos_info, "a=des:qos mandatory remote sendrecv\r\n");
							// ask the remote terminal to inform us when QoS reservation is complete

							if ((ca->caller == 0) && (ca->qos_remote_state != SENDRECV))
								strcat(qos_info, "a=conf:qos remote sendrecv\r\n");
						}
						else if (strstr(a_attrib->a_att_value,"optional local sendrecv"))
						{
							ca->qos_remote_preference = QOS_OPTIONAL;
							strcat(qos_info, "a=des:qos optional remote sendrecv\r\n");
						}
						else if (strstr(a_attrib->a_att_value,"none local sendrecv"))
						{
							ca->qos_remote_preference = QOS_NONE;
							strcat(qos_info, "a=des:qos none remote sendrecv\r\n");
						}
					}
			
					// tell the other UA what level of QoS we require	
					if (strstr(a_attrib->a_att_value,"remote"))
					{
						if (pref->qos_strength == QOS_MANDATORY)
							strcat(qos_info, "a=des:qos mandatory local sendrecv\r\n");
						else if (pref->qos_strength == QOS_OPTIONAL)
							strcat(qos_info, "a=des:qos optional local sendrecv\r\n");
						else if (pref->qos_strength == QOS_NONE)
							strcat(qos_info, "a=des:qos none local sendrecv\r\n");
					}

					// if QoS is e2e agree with other UA
					if (strstr(a_attrib->a_att_value, "e2e"))
					{
						strcat(qos_info, "a=des:");
						strcat(qos_info, a_attrib->a_att_value);
						strcat(qos_info, "\r\n");

						if ((ca->caller == 0) && (ca->qos_remote_state != SENDRECV))
							strcat(qos_info, "a=conf:qos e2e send\r\n");
							

					}	

				}
				

				// check what type of QoS we have currently
				if (strstr(a_attrib->a_att_field, "curr"))
				{
					if (strstr(a_attrib->a_att_value,"local none"))
					{
						ca->qos_remote_state = NONE;
						strcat(qos_info, "a=curr:qos remote none\r\n");
					}
					
					if (strstr(a_attrib->a_att_value,"local sendrecv"))
					{	
						ca->qos_remote_state = SENDRECV;
						strcat(qos_info, "a=curr:qos remote sendrecv\r\n");
					}

					if (strstr(a_attrib->a_att_value,"remote"))
					{
						if (ca->qos_local_state == NONE)
							strcat(qos_info, "a=curr:qos local none\r\n");
						
						if (ca->qos_local_state == SENDRECV)
							strcat(qos_info, "a=curr:qos local sendrecv\r\n");
					}
		
					// if QoS is end-to-end just agree with other UA
					if (strstr(a_attrib->a_att_value,"e2e"))
					{
						if (ca->qos_local_state == SENDRECV)
						{
							strcat(qos_info, "a=curr:qos e2e sendrecv\r\n");
							ca->qos_remote_state = SENDRECV;
						}
						else
						{
							strcat(qos_info, "a=curr:");
							strcat(qos_info, a_attrib->a_att_value);
							strcat(qos_info, "\r\n");

							if (strstr(a_attrib->a_att_value,"sendrecv"))
							{
								ca->qos_remote_state = SENDRECV;
								ca->qos_local_state = SENDRECV;
							}
						}
						
					}

				}
		
				// the other terminal requires confirmation when QoS reservation is complete		
				if ((strstr(a_attrib->a_att_field, "conf")) && (strstr(a_attrib->a_att_value,"remote sendrecv")))
					ca->qos_confirm = 1;

				pos3++;
			
			}

			strcat(buf, qos_info);

			if (num_codecs > 0)
			{
                		strcat (buf, "a=rtpmap:");
				strcat (buf, h263_avp_value);
				strcat (buf, "\r\na=fmtp:");
				strcat (buf, payloads);
				strcat (buf, " profile-level-id=0\r\n");
	      		}
      
           	 }
      	}
	else if (0 == osip_strcasecmp (remote_med->m_media, "message"))
	{
		if ((!pref->session_im_enabled))
            	{
			g_warning("I found an IM  component in this call but session based IM is disabled\n");
			strcat (buf, "m=message 0 TCP/MSRP\r\n");
          	} 
		else if (!ca->msrp_endpoint)
		{
			g_warning("I found an IM  component in this call but there is an MSRP error\n");
			strcat (buf, "m=message 0 TCP/MSRP\r\n");
		}
		else
		{	
			printf("Found IM Component in remote SDP!\n");
			//convert int to char*
			char local_msrp_port[20];
			sprintf(local_msrp_port,"%i",pref->local_msrp_port);

			strcat (buf, "m=message ");
			strcat (buf, local_msrp_port);
			strcat (buf, " TCP/MSRP\r\n");
			strcat (buf, "a=accept-types:text/plain\r\n");
			strcat (buf,"a=path:");
			strcat (buf,msrp_endpoint_get_from_fullpath(ca->msrp_endpoint));
			strcat (buf,"\r\n");
		}
	}
	else
        {
		g_warning("Don't recongise this media type\n");

        	strcat (buf, "m=");
        	strcat (buf, remote_med->m_media);
        	strcat (buf, " 0 ");
        	strcat (buf, remote_med->m_proto);
        	strcat (buf, " \r\n");
        }
      
	pos++;

    }


	osip_message_set_body (*answer, buf, strlen(buf));
  	osip_message_set_content_type (*answer, "application/sdp");
  	sdp_message_free (remote_sdp);

	
 	return 0;

}
