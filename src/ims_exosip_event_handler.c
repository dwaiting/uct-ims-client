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
#include "ims_exosip_event_handler.h"

#define GLADE_HOOKUP_OBJECT(component,widget,name) \
  g_object_set_data_full (G_OBJECT (component), name, \
    gtk_widget_ref (widget), (GDestroyNotify) gtk_widget_unref)

#define GLADE_HOOKUP_OBJECT_NO_REF(component,widget,name) \
  g_object_set_data (G_OBJECT (component), name, widget)



void ims_process_401(eXosip_event_t *je)
{

	int i;
	char reg_expire_str[10];
	char *nonce64, *algorithm, *nonce64_no_quotes;
	int aka_version;
	Preferences *pref = client->pref;

	osip_cseq_t *cseq;
	cseq = osip_message_get_cseq(je->response);

	if (strcmp(cseq->number, "6") == 0)
	{
		set_display("Error with registration.\n\nCheck password.");
		return ;
	}

	osip_www_authenticate_t *www_header;
	osip_message_get_www_authenticate(je->response, 0, &www_header);
	algorithm = osip_www_authenticate_get_algorithm(www_header);

	if (strcmp(algorithm, "AKAv1-MD5") == 0)
		aka_version = 1;
	else if (strcmp(algorithm, "AKAv2-MD5") == 0)
		aka_version = 2;
	else if (strcmp(algorithm, "MD5") == 0)
		aka_version = 0;
	else
	{
		fprintf(stderr, "UCTIMSCLIENT: Authentication scheme %s not supported\n", algorithm);
		return ;
	}

	nonce64 = osip_www_authenticate_get_nonce(www_header);
	nonce64_no_quotes = imsua_remove_quotes(nonce64);

	str response={0,0};
	str nonce={0,0};
	str k={0,0};
	str responseMD5={0,0};
	str username={0,0};
	str realm={0,0};
	str uri={0,0};

	nonce.s = nonce64_no_quotes;
	nonce.len = strlen(nonce64_no_quotes);

	k.s = pref->password;
	k.len = strlen(pref->password);
		
        username.s = (pref->impi);
	username.len = strlen(username.s);

	realm.s = pref->realm;
	realm.len = strlen(realm.s);
	
	char new_realm[128];
	char *sip_str = "sip:";
	char *sip_strs = "sips:";

	if((strstr(pref->realm, sip_str) != pref->realm) && (strstr(pref->realm, sip_strs) != pref->realm))
	{
		strcpy(new_realm, "sip:");
		strcat(new_realm, pref->realm);
	}
	else
		return ;

	uri.s = new_realm;
	uri.len = strlen(uri.s);

	if (aka_version > 0)
		response=AKA(aka_version,nonce,k);
	else
	{
		response = k;
	}

	if (response.s){
	/* response AKA MD5 */
		responseMD5 = MD5(response,username,realm,nonce,uri);	
		// free(response.s);				
	}

	osip_message_t *reg2 = NULL;
	int expires;

	if (is_message_deregister)
		expires = 0;
	else
		expires = REG_EXPIRE;

	eXosip_lock (context_eXosip);
	int r = eXosip_register_build_register(context_eXosip, reg_id, expires, &reg2);
	eXosip_unlock (context_eXosip);

	if (r < 0)
		return ;

	/* Hack to stop eXosip from setting expires to 3600 */
	osip_header_t *expires_header;
	osip_message_get_expires(reg2, 0, &expires_header);

	/* Convert our expire value to a string */
	sprintf(reg_expire_str, "%d", REG_EXPIRE);

	if (!strcmp(expires_header->hvalue, "36,00") && (expires == REG_EXPIRE))
		osip_header_set_value(expires_header, osip_strdup(reg_expire_str));

	osip_authorization_t *auth_header;
	osip_authorization_init(&auth_header);

	osip_authorization_set_auth_type(auth_header, "Digest");
	osip_authorization_set_username(auth_header, add_quotes(username.s));
	osip_authorization_set_realm(auth_header, add_quotes(realm.s));
			
	osip_authorization_set_nonce(auth_header, add_quotes(nonce.s));
	osip_authorization_set_uri(auth_header, add_quotes(uri.s));
	osip_authorization_set_response(auth_header, add_quotes(responseMD5.s));
	osip_authorization_set_algorithm(auth_header, algorithm);

	char *h_value;
	osip_authorization_to_str(auth_header, &h_value);

	if (osip_message_set_authorization(reg2, h_value) != 0)
		printf("Can't set authorisation\n");

	eXosip_lock(context_eXosip);
	i = eXosip_register_send_register (context_eXosip, reg_id, reg2);
	eXosip_unlock(context_eXosip);

	if (is_message_deregister)
		reg_id = -1;


	if (i != 0)
		set_display("Error sending REGISTER");
	else if (!is_message_deregister)
		imsua_set_message_display("REGISTER with credentials", 1);

}


void ims_process_registration_200ok(eXosip_event_t *je)
{

	int i;
	Preferences *pref = client->pref;

	// stop timer for delay tests
	gettimeofday(&end_time, NULL);
	struct timeval result;
	timeval_subtract(&result, &end_time, &start_time);

	sprintf(display, "Registered with %s",pref->realm);
	set_display(display);
	sprintf(display, "\nRegistration delay: %ld.%03lds", result.tv_sec, result.tv_usec/1000);
	imsua_append_text_to_display(display);

	sprintf(display,"%s registered on %s",pref->impu,pref->realm);
	set_status_bar(display);

	// get list of service routes from 200 OK response
	osip_header_t *service_route;
	int num_routes = 0;
	num_service_routes = 0;

	while(osip_message_header_get_byname(je->response, "Service-Route", num_routes, &service_route) >= 0)
	{
		// add this route to our list of service routes if it is not a duplicate
		if ((num_service_routes == 0) || (strcmp(osip_header_get_value(service_route), ims_service_route[num_service_routes - 1]) != 0))
		{		
			ims_service_route[num_service_routes] = osip_header_get_value(service_route);
			num_service_routes ++;
		}

		num_routes ++;
	}

	// get list of associated uris from 200 OK response
	osip_header_t *p_assoc_uri;

	int num_uris = 0;

	char buf[200];
	strcpy(buf, "IMPUs:");

	while(osip_message_header_get_byname(je->response, "P-Associated-URI", num_uris, &p_assoc_uri) >= 0)
	{
		char *assoc_uri;
		assoc_uri = osip_header_get_value(p_assoc_uri);

		/* display associated URIs  */
		strcat(buf, " ");
		strcat(buf, assoc_uri);
		num_uris ++;
		
		/* add this uri to our list of associated uris	*/
		ims_associated_uris[num_associated_uris] = assoc_uri;

		num_associated_uris ++;
	}

	imsua_set_message_display(buf, 0);


	/* Subscribe to the reg event */
	if (!is_message_deregister)
	{
		osip_message_t *subscribe;

		eXosip_lock(context_eXosip);
		i = eXosip_subscribe_build_initial_request(context_eXosip, &subscribe, pref->impu, pref->impu, add_lr_to_route(add_sip_scheme(pref->pcscf)), "reg", 600000);
		eXosip_unlock(context_eXosip);

		if(i != 0)
		{
			set_display("Error setting up\nsubscription to reg event");
			return ;
		}

		// adding the service routes
		int j;
		for (j = num_service_routes; j > 0; j--)
		{
			osip_message_set_route(subscribe, ims_service_route[j-1]);
		}

		eXosip_lock(context_eXosip);
		i = eXosip_subscribe_send_initial_request(context_eXosip, subscribe);
		eXosip_unlock(context_eXosip);

		if (i < 0)
			set_display("Error subscribing to reg event");
		else
			imsua_set_message_display("SUBSCRIBE (reg event)", 1);		
		
	}

	/* publish our own presence and subcribe to our buddies presence */
/*	
	if (pref->presence_enabled && !is_message_deregister)
	{
		presence_publish_presentity("open", "Available");	
		presence_subscribe_to_all_presentities(PRESENCE_EXPIRE);
		watchers_subscribe_to_watcherinfo(PRESENCE_EXPIRE);
	}
*/
}


int ims_message_requires_preconditions(osip_message_t *message)
{

	int pos1 = 0;
	osip_header_t *require = NULL;
	int preconditions = 0;
	
	osip_message_get_require(message, pos1, &require);

	if((require != NULL) && (strstr(require->hvalue, "precondition")))
		preconditions = 1;
	
	while(require != NULL)
	{
		pos1 ++;
		osip_message_get_require(message, pos1, &require);
		
		if((require != NULL) && (strstr(require->hvalue, "precondition")))
			preconditions = 1;

	}	

	return preconditions;

}



int ims_message_requires_100rel(osip_message_t *message)
{

	int pos1 = 0;
	osip_header_t *require = NULL;
	int reliable = 0;
	
	osip_message_get_require(message, pos1, &require);

	if((require != NULL) && (strstr(require->hvalue, "100rel")))
		reliable = 1;
	
	while(require != NULL)
	{
		pos1 ++;
		osip_message_get_require(message, pos1, &require);
		
		if((require != NULL) && (strstr(require->hvalue, "100rel")))
			reliable = 1;

	}	

	return reliable;

}



void ims_process_incoming_invite(eXosip_event_t *je)
{

	Preferences *pref = client->pref;

	// Send "Busy Here" if we are in a call already and return
	if (state != IDLE)
	{
		eXosip_lock(context_eXosip);
		eXosip_call_send_answer(context_eXosip, je->tid, 486, NULL);
		eXosip_unlock(context_eXosip);
		return ;
	}


	// if this INVITE is not addressed to one of associated URIs then exit this method
	char *to_uri;
	osip_uri_to_str(((je->request)->to)->url, &to_uri);

	if(!imsua_is_associated_uri(to_uri))
		return ;

	Call *ca;

	if (find_call(je->did, &ca) < 0)
		add_call(&ca);

	// setup the call struct
	ca->tid = je->tid;
	ca->did = je->did;
	ca->cid = je->cid;
	current_dialog_id = je->did;
	ca->most_recent_message = je->request;

	ca->video_supported = 0;

	// Extract call information from message
	message_extract_call_info(ca, je->request);

	ca->qos_local_state = NONE;
	ca->caller = 0;
	ca->call_is_active = 0;

	//If message had an MSRP component must initialise an end point
/*
	if(ca->remote_msrp_port && pref->session_im_enabled)
	{
		local_msrp_endpoint = endpointmsrp_create_endpoint(je->cid,pref->local_audio_ip,pref->local_msrp_port,1);
	}
*/

	
	/* If IM is supported then create an MSRP endpoint now */
	if (ca->im_supported)
	{
		ca->msrp_endpoint = endpointmsrp_create_endpoint(ca->cid, pref->local_msrp_ip, pref->local_msrp_port, 0);

		/* Create an active peer (If we are passive then the peer must be active and vice versa) */
		if(endpointmsrp_start_session(ca->msrp_endpoint, ca->remote_msrp_path, 1) == 1)
			printf("Started IM Session!\n");
		else
			g_warning("Could not start IM Session!\n");

	}


	/* If this call only has a message component then we can send a 200 OK immediately */
	if (ca->im_supported && !ca->audio_supported && !ca->video_supported)
	{
		osip_message_t *answer;

		eXosip_lock (context_eXosip);
		eXosip_call_build_answer (context_eXosip, ca->tid, 200, &answer);
		eXosip_unlock (context_eXosip);
		
		sdp_complete_ims(je->request, &answer, ca, 0);

		osip_message_set_allow(answer, "INVITE, ACK, CANCEL, BYE, PRACK, UPDATE, REFER, MESSAGE");

		eXosip_lock (context_eXosip);
		eXosip_call_send_answer (context_eXosip, je->tid, 200, answer);
		eXosip_unlock (context_eXosip);

		imsua_set_message_display("200 OK (INVITE)", 1);
		
	}
	// If this call requires preconditions send 183 provisional response else send a ringing response
	else if (!ims_message_requires_preconditions(je->request))
	{

		sprintf(display, "Incoming call from\n\n%s <%s>", ca->from_name, ca->from_uri);
		set_display(display);
		osip_message_t *ringing;

		eXosip_lock (context_eXosip);
		eXosip_call_build_answer(context_eXosip, je->tid, 180, &ringing);
		eXosip_unlock (context_eXosip);

		sdp_complete_ims(je->request, &ringing, ca, 0);

		osip_message_set_require(ringing, "100rel");

		// Support for RFC 3262
		osip_message_set_header((osip_message_t *)ringing,(const char *)"RSeq","1");

		// Support for RFC 3455 - Private Header (P-Header) Extensions to the Session Initiation
	    	// Protocol (SIP) for the 3rd-Generation Partnership Project (3GPP)
		osip_message_set_header((osip_message_t *)ringing,(const char *)"P-Access-Network-Info",access_networks[pref->access_network]);

		osip_message_set_allow(ringing, "INVITE, ACK, CANCEL, BYE, PRACK, UPDATE, REFER, MESSAGE");

		eXosip_lock (context_eXosip);
		eXosip_call_send_answer (context_eXosip, je->tid, 180, ringing);
		eXosip_unlock (context_eXosip);

		state = LOCAL_RINGING;

		// create a thread to make the ringing noise
		
		// initialiseRingingPipeline(ca);

		imsua_set_message_display("180 Ringing (INVITE)", 1);
	}
	else
	{

		osip_message_t *session_progress;

		eXosip_lock (context_eXosip);
		eXosip_call_build_answer(context_eXosip, je->tid, 183, &session_progress);
		eXosip_unlock (context_eXosip);

		sdp_complete_ims(je->request, &session_progress, ca, 0);

		osip_message_set_require(session_progress, "100rel");

		// Support for RFC 3262
		osip_message_set_header((osip_message_t *)session_progress,(const char *)"RSeq","1");

		// Support for RFC 3455 - Private Header (P-Header) Extensions to the Session Initiation
	    	// Protocol (SIP) for the 3rd-Generation Partnership Project (3GPP)
		osip_message_set_header((osip_message_t *)session_progress,(const char *)"P-Access-Network-Info",access_networks[pref->access_network]);

		osip_message_set_allow(session_progress, "INVITE, ACK, CANCEL, BYE, PRACK, UPDATE, REFER, MESSAGE");

		eXosip_lock (context_eXosip);
		eXosip_call_send_answer(context_eXosip, je->tid, 183, session_progress);
		eXosip_unlock (context_eXosip);

		state = IDLE;

		imsua_set_message_display("183 Session Progress (INVITE)", 1);
	}
}



void ims_process_18x(eXosip_event_t *je)
{

	Call *ca;

	Preferences *pref = client->pref;

	if (find_call(je->did, &ca) < 0)
		add_call(&ca);
		

	ca->did = je->did;
	ca->tid = je->tid;
	ca->cid = je->cid;

	printf("Call ID in process 18x: %d\n", ca->cid);

	current_dialog_id = je->did;
	ca->prov_resp = je->response;

	ca->qos_local_state = NONE;
	ca->caller = 1;
	ca->qos_confirm = 0;

	osip_header_t *user_agent;
	osip_message_t *p_response;
	p_response = je->response;

	/* Extract information from message */
	message_extract_call_info(ca, je->response);

	/* Session-based IM component exists */
	if(ca->im_supported && pref->session_im_enabled)
	{

		/* If we haven't created our MSRP session yet, do it now */
		if (!ca->msrp_endpoint)
		{
			ca->msrp_endpoint = client->msrp_endpoint;

			char cid_str[20];
			sprintf(cid_str, "%d", ca->cid);
			msrp_endpoint_set_callid(ca->msrp_endpoint, cid_str);

			/* Create a passive MSRP Peer (If we are active peer must be passive and vice versa) */
			if(endpointmsrp_start_session(ca->msrp_endpoint, ca->remote_msrp_path, 0) == 1)
				printf("Started IM Session!\n");
			else
				g_warning("Could not start IM Session!\n");
		}
	}


	// send a PRACK if the provisional response requires reliable provisional responses
	if (ims_message_requires_100rel(je->response))
	{
		osip_message_t *prack;

		eXosip_lock (context_eXosip);
		eXosip_call_build_prack(context_eXosip, je->tid, &prack);
		eXosip_unlock (context_eXosip);

		sdp_complete_ims(je->response, &prack, ca, 0);

		// check to see if other client requires preconditions
		if (ims_message_requires_preconditions(je->response))
			osip_message_set_require(prack, "precondition");

		osip_message_set_require(prack, "sec-agree");

		// Support for RFC 3455 - Private Header (P-Header) Extensions to the Session Initiation
	    	// Protocol (SIP) for the 3rd-Generation Partnership Project (3GPP)
		osip_message_set_header((osip_message_t *)prack,(const char *)"P-Access-Network-Info",access_networks[pref->access_network]);

		eXosip_lock (context_eXosip);
		eXosip_call_send_prack (context_eXosip, je->tid, prack);
		eXosip_unlock (context_eXosip);

		imsua_set_message_display("PRACK", 1);
	}


	if (p_response->status_code == 183)
	{
		set_display("Provisional response (183) received");
		ca->sent_update = 0;
	}
	else if (p_response->status_code == 180)
	{
		set_display("Remote Client is ringing...");

		// stop timer for delay tests
		gettimeofday(&end_time, NULL);
		struct timeval result, temp_start;
		temp_start = start_time;
		timeval_subtract(&result, &end_time, &temp_start);

		sprintf(display, "\n\nCall setup delay: %ld.%03lds", result.tv_sec, result.tv_usec/1000);
		imsua_append_text_to_display(display);
		
		state = REMOTE_RINGING;
		// osip_thread_create (20000, start_ringing, NULL);

		initialiseRingingPipeline(ca);

	}

}


void ims_process_prack(eXosip_event_t *je)
{

	Call *ca;
	Preferences *pref = client->pref;	

	if (find_call(je->did, &ca) < 0)
		return ;
	
	if (eXosip_get_sdp_info (je->request))
		ca->most_recent_message = je->request;

	// Extract information from message
	message_extract_call_info(ca, je->request);

	osip_message_t *answer_200ok;

	eXosip_lock (context_eXosip);
	eXosip_call_build_answer(context_eXosip, je->tid, 200, &answer_200ok);  	
	eXosip_unlock (context_eXosip);
	
	if (!ca->media_negotiated)
		sdp_complete_ims(je->request, &answer_200ok, ca, 0);
	
	osip_message_set_require(answer_200ok, "sec-agree");
	osip_message_set_proxy_require(answer_200ok, "sec-agree");

	eXosip_lock (context_eXosip);
	eXosip_call_send_answer(context_eXosip, je->tid, 200, answer_200ok);
	eXosip_unlock (context_eXosip);
	imsua_set_message_display("200 OK (PRACK)", 1);



	// if both our and the remote terminal's QoS requirements have been met we can start ringing
	if ((ca->qos_remote_state == SENDRECV || ca->qos_remote_preference == QOS_NONE || ca->qos_remote_preference == QOS_OPTIONAL) && (pref->qos_strength == QOS_NONE || pref->qos_strength == QOS_OPTIONAL || ca->qos_local_state == SENDRECV) && state == IDLE)
	{

		osip_message_t *ringing;

		eXosip_lock (context_eXosip);
		eXosip_call_build_answer(context_eXosip, ca->tid, 180, &ringing);
		eXosip_unlock (context_eXosip);

		// support for RFC 3262
		osip_message_set_require(ringing, "100rel");
		osip_message_set_header((osip_message_t *)ringing,(const char *)"RSeq","2");

		eXosip_lock (context_eXosip);
		eXosip_call_send_answer(context_eXosip, ca->tid, 180, ringing);
		eXosip_unlock (context_eXosip);

		state = LOCAL_RINGING;
		
		initialiseRingingPipeline(ca);

		sprintf(display, "Incoming call from\n\n%s <%s>", ca->from_name, ca->from_uri);
		set_display(display);
		imsua_set_message_display("180 Ringing", 1);
	}
	
}


void ims_process_update(eXosip_event_t *je)
{
	
	set_display("UPDATE received");

	Call *ca;

	if (find_call(je->did, &ca) < 0)
		return ;

	// Extract information from message 
	message_extract_call_info(ca, je->request);

	ca->media_negotiated = 1;

	if (eXosip_get_sdp_info (je->request))
		ca->most_recent_message = je->request;

	// reply to UPDATE message
	// Set local QOS to SENDRECV - WARNING: We should usually should get authorisation from PDF!
	ca->qos_local_state = SENDRECV;	

	osip_message_t *answer_200ok;

	eXosip_lock (context_eXosip);
	eXosip_call_build_answer(context_eXosip, je->tid, 200, &answer_200ok);  	
	eXosip_unlock (context_eXosip);
	
	sdp_complete_ims(je->request, &answer_200ok, ca, 0);
	
	osip_message_set_require(answer_200ok, "sec-agree");
	osip_message_set_proxy_require(answer_200ok, "sec-agree");

	osip_message_t *ringing;
	eXosip_lock (context_eXosip);
	eXosip_call_build_answer(context_eXosip, ca->tid, 180, &ringing);
	eXosip_unlock (context_eXosip);

	// support for RFC 3262
	osip_message_set_require(ringing, "100rel");
	osip_message_set_header((osip_message_t *)ringing,(const char *)"RSeq","2");

	// sdp_complete_ims(je->request, &ringing, ca, 0);

	eXosip_lock (context_eXosip);
	eXosip_call_send_answer(context_eXosip, je->tid, 200, answer_200ok);
	eXosip_unlock (context_eXosip);
	imsua_set_message_display("200 OK (UPDATE)", 1);


	// if we have confirmation of remote client QoS we can start ringing
	if (ca->qos_remote_state == SENDRECV && state == IDLE)
	{
		eXosip_lock (context_eXosip);
		eXosip_call_send_answer(context_eXosip, ca->tid, 180, ringing);
		eXosip_unlock (context_eXosip);

		state = LOCAL_RINGING;
		
		// create a thread to make the ringing noise			
		// osip_thread_create (20000, start_ringing, NULL);
		initialiseRingingPipeline(ca);

		sprintf(display, "Incoming call from\n\n%s <%s>", ca->from_name, ca->from_uri);
		set_display(display);
		imsua_set_message_display("180 Ringing", 1);
	}
		

}


void ims_process_2xx(eXosip_event_t *je)
{

	Call *ca;

	if (find_call(je->did, &ca) < 0)
		return ;

	// Extract information from message
	// message_extract_call_info(ca, je->response);

	// send an UPDATE if the remote client requested one
	if (ca->qos_confirm && !ca->sent_update)
	{
		ca->qos_local_state = SENDRECV;

		osip_message_t *update;

		eXosip_lock(context_eXosip);
		eXosip_call_build_update(context_eXosip, ca->did, &update);
		eXosip_unlock(context_eXosip);
		
		sdp_complete_ims(ca->prov_resp, &update, ca, 0);

		eXosip_lock(context_eXosip);
		eXosip_call_send_request(context_eXosip,ca->did, update);
		eXosip_unlock(context_eXosip);

		ca->sent_update = 1;

		imsua_set_message_display("UPDATE", 1);
	}
}


void ims_process_200ok(eXosip_event_t *je)
{

	Preferences *pref = client->pref;
	Call *ca;


	if (find_call(je->did, &ca) < 0)
		add_call(&ca);
	

	osip_message_t *ack;

	ca->did = je->did;
	ca->cid = je->cid;
	current_dialog_id = je->did;
	
	message_extract_call_info(ca, je->response);

	// Check to see if the server wants to redirect us elsewhere
	if (ca->content_indirected)
	{
		sprintf(display, "Content indirected to:\n%s\n\n", ca->content_indirection_uri);

		// Check what type of content it wants us to open
		if (strstr(ca->content_indirection_uri, "http:") || strstr(ca->content_indirection_uri, "https:"))
		{
			strcat(display, "Attempting to open Firefox ...\n");

			// We must fork because execl will overlay the current process
			if (fork() == 0)
				execl("/usr/bin/firefox", "firefox", ca->content_indirection_uri, 0, NULL);
			
		}
		else if (strstr(ca->content_indirection_uri, "rtsp:"))
		{
			strcat(display, "Attempting to open RTSP stream ...\n");
			printf("Attempting to open RTSP stream ...\n");
			// rtsp_start_session(ca->content_indirection_uri);
			sprintf(display, "200 OK Received\n\nRTSP session established with:\n<%s>", ca->content_indirection_uri);
		}
	}
	else
	{
		sprintf(display, "200 OK Received\n\nCall established with:\n<%s>", ca->to_uri);
	}

	/* Session-based IM component exists */
	if(ca->im_supported && pref->session_im_enabled)
	{

		/* If we haven't created our MSRP session yet, do it now */
		if (!ca->msrp_endpoint)
		{
			ca->msrp_endpoint = client->msrp_endpoint;

			char cid_str[20];
			sprintf(cid_str, "%d", ca->cid);
			msrp_endpoint_set_callid(ca->msrp_endpoint, cid_str);

			if(endpointmsrp_start_session(ca->msrp_endpoint, ca->remote_msrp_path, 0) == 1)
				printf("Started IM Session!\n");
			else
				g_warning("Could not start IM Session!\n");

			
		}

/*
		if(local_msrp_endpoint)
		{
			eXosip_event_t *null_event = NULL;
			char *null_string = NULL;

			if(endpointmsrp_start_session(je, ca->remote_msrp_path ,0) == 1)
			{
				// ims_start_im_session(null_event, null_string);
			}
			// ims_start_im_session(null_event, null_string);
		}
		else
		{
			g_warning("MRSP endpoint error - MSRP not enabled in this session\n");
		}
*/

	}

	ca->qos_local_state = SENDRECV;
	ca->sent_update = 0;
	ca->caller = 1;

	eXosip_lock(context_eXosip);
	eXosip_call_build_ack(context_eXosip, je->did, &ack);
	eXosip_unlock(context_eXosip);

	if (!ca->media_negotiated)
		sdp_complete_ims(je->response, &ack, ca, 0);

	eXosip_lock(context_eXosip);
	eXosip_call_send_ack(context_eXosip, je->did, ack);
	eXosip_unlock(context_eXosip);
	
	set_display(display);
	imsua_set_message_display("ACK", 1);

	if(state == MOD_SESSION)
	{
		// stop timer for delay tests
		gettimeofday(&end_time, NULL);
		struct timeval result, temp_start;
		temp_start = start_time;
		timeval_subtract(&result, &end_time, &temp_start);
		sprintf(display, "\nCall modification delay: %ld.%03lds", result.tv_sec, result.tv_usec/1000);
		imsua_append_text_to_display(display);
	}

	
	if (ca->audio_supported || ca->video_supported)
		state = IN_CALL;

	if (!ca->content_indirected)//only start an RTP session if this wasn't a redirected call
	{
		media_start_session(je);
	}
	
	//remove previous msrp session - close IMs window
	if(im_window_open == 1)
	{
		gtk_widget_destroy(GTK_WIDGET(im_window));
		im_window_open = 0;
 		num_im_tabs=0;
	}


}


void ims_process_ack(eXosip_event_t *je)
{
	
	Call *ca;
	sdp_message_t *remote_sdp;

	if (find_call(je->did, &ca) < 0)
		return ;

	message_extract_call_info(ca, je->ack);

	sprintf(display, "ACK Received\n\nCall established with:\n%s <%s>", ca->from_name, ca->from_uri);
	set_display(display);

	gtk_entry_set_text(GTK_ENTRY(client->uri_entry), ca->from_uri);

	if (ca->audio_supported || ca->video_supported)
		state = IN_CALL;

	media_start_session(je);

	/*
	//remove previous msrp session - close IMs window
	if(im_window_open == 1)
	{
		gtk_widget_destroy(GTK_WIDGET(im_window));
		im_window_open = 0;
 		num_im_tabs=0;
	}
	if(ca->remote_msrp_port && pref->session_im_enabled) //session based IM component exists
	{
		if(local_msrp_endpoint)
		{
			eXosip_event_t *null_event = NULL;
			char *null_string = NULL;
			if(endpointmsrp_start_session(je, ca->remote_msrp_path ,0) == 1)
			{
				ims_start_im_session(null_event, null_string);
			}
		}
		else
		{
			g_warning("MRSP endpoint error - MSRP not enabled in this session\n");
		}
	}
	*/
}


void ims_process_released_call(eXosip_event_t *je)
{
	Preferences *pref = client->pref;
	Call *ca;

	if (find_call(current_dialog_id, &ca) < 0)
	{
		g_warning("Can't find this call to end \n");
		return ;
	}

	printf("Call ID:  %d   Event ID:  %d   Current Dialog ID: %d  Event Dialog ID: %d\n", ca->cid, je->cid, current_dialog_id, je->did);

	// check if this is the current call and end it
	if ((ca->cid == je->cid) || (current_dialog_id == je->did))
	{
		set_display("Call released");

		// Destroy all media pipelines
		destroyRingingPipeline(ca);

		destroyAudioTxPipeline(ca);
		destroyAudioRxPipeline(ca);

		destroyVideoTxPipeline(ca);
		destroyVideoRxPipeline(ca);

		destroyBackgroundVideoPipeline();

		destroyIptvVideoPipeline(ca);

		if (ca->im_supported)
		{
			if(ca->caller == 1)
			{
				msrp_send_text(ca->msrp_endpoint, " ", 0);
			}

			if(ca->msrp_endpoint && (msrp_endpoint_destroy(ca->msrp_endpoint) < 0))
			{
				g_warning("MSRP Error: Couldn't destroy endpoint... (Release call)\n");
			}
			else
				printf("Destroyed MSRP endpoint\n");
		}


		ca->call_is_active = 0;

		/* Delete this call from our list of active dialogs */
		delete_call(current_dialog_id);
		state = IDLE;

	}

}

int ims_process_message (MsrpEndpoint *endpoint, char *msrp_message)
{
	Call *ca;

	printf("Call ID of MSRP session: %s\n", msrp_endpoint_get_callid(endpoint));
	
	if (find_call_by_cid(atoi(msrp_endpoint_get_callid(endpoint)), &ca) < 0)
	{
		g_warning("Received MSRP message not associated to any current calls");
		return -1;
	}

	printf("MSRP Call To: %s\nMSRP Call From: %s\n", ca->to_uri, ca->from_uri);

	int i;
	// int tab_exists = 0;
	GtkWidget *im_page;
	GtkTextBuffer *im_output_buffer;

	for (i = 0; i < gtk_notebook_get_n_pages(GTK_NOTEBOOK(client->im_notebook)); i++)
	{

		GtkWidget *scrolled_window = gtk_notebook_get_nth_page(GTK_NOTEBOOK(client->im_notebook), i);
		im_page = gtk_bin_get_child(GTK_BIN(scrolled_window));
		GtkWidget *im_tab_label = gtk_notebook_get_tab_label(GTK_NOTEBOOK(client->im_notebook), scrolled_window);
		const char *im_tab_label_str  = gtk_label_get_text(GTK_LABEL(im_tab_label));

		printf("Tab label: %s\n", im_tab_label_str);

		/*
			Check to see if we already have a tab open for this converstion
		 */
		if (((ca->caller == 1) && !strcmp(im_tab_label_str, ca->to_uri)) || ((ca->caller == 0) && (!strcmp(im_tab_label_str, ca->from_uri))))
		{

			printf("Found tab for %s\n", im_tab_label_str);

			im_output_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW(im_page));
			break ;
		}
	}


	/* No tab exists for this conversation */
	if (!im_output_buffer)
	{
		printf("No tab found for. Creating new tab.\n");

		GtkWidget *im_text_view;

		if (!(im_text_view = gtk_text_view_new()))
		{
			printf("Could not create text view!!!???\n");
			return -1;
		}

		printf("Debug 1\n");

		GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);

		printf("Debug 2\n");

		gtk_container_add(GTK_CONTAINER(scrolled_window), im_text_view);

		GtkWidget *im_label;
		
		if (ca->caller)
			im_label = gtk_label_new(ca->to_uri);
		else
			im_label = gtk_label_new(ca->from_uri);

		gtk_notebook_append_page(GTK_NOTEBOOK(client->im_notebook),GTK_WIDGET(scrolled_window),GTK_WIDGET(im_label));



		printf("Debug 3\n");



		printf("Debug 4\n");

			
		gtk_text_view_set_editable(GTK_TEXT_VIEW(im_text_view),FALSE);
		gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(im_text_view),FALSE);




		gtk_widget_show_all (GTK_WIDGET(scrolled_window));
		gtk_notebook_set_current_page(GTK_NOTEBOOK(client->im_notebook), -1);

		im_output_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW(im_text_view));

		im_page = im_text_view;

	}


	printf("Updating IM text %s\n", msrp_message);

	/* Update the display */
	char *uri_name;

	gchar buf [200]= "";
	gchar buf1 [200]= "";
	
	GtkTextTag *tag;

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
	if(gtk_text_tag_table_lookup(tag_table,"green") == NULL)
	{
		// Tag with font fixed and tag name "italic". 
		gtk_text_buffer_create_tag (im_output_buffer, "green", "foreground", "darkgreen", NULL);
	}


	//This writes the time stamp in bold for display on the screen
	GtkTextIter start_message_iter;
	gtk_text_buffer_get_end_iter(im_output_buffer, &start_message_iter);
	GtkTextMark *start_message_mark = gtk_text_buffer_create_mark(im_output_buffer,"start_message_mark",&start_message_iter,TRUE);
	
	if (ca->caller)
		sprintf(buf, "%s: ", ca->to_uri);
	else if (strcmp(ca->from_name, ""))
		sprintf(buf, "%s: ", imsua_remove_quotes(ca->from_name));
	else
		sprintf(buf, "%s: ", ca->from_uri);


	gtk_text_buffer_insert(im_output_buffer,&start_message_iter,buf,strlen(buf));
	GtkTextIter end_message_iter;
	gtk_text_buffer_get_end_iter(im_output_buffer, &end_message_iter);
	gtk_text_buffer_get_iter_at_mark(im_output_buffer,&start_message_iter,start_message_mark);
	//Apply the BOLD tag to the selected text.
	gtk_text_buffer_apply_tag_by_name (im_output_buffer, "bold", &start_message_iter, &end_message_iter);
	gtk_text_buffer_apply_tag_by_name (im_output_buffer, "green", &start_message_iter, &end_message_iter);	
	
	//This writes the message portion is normal font for display on the screen
	gtk_text_buffer_get_end_iter(im_output_buffer, &start_message_iter);
	start_message_mark = gtk_text_buffer_create_mark(im_output_buffer,"start_message_mark",&start_message_iter,TRUE);
	sprintf(buf, "%s\n", msrp_message);
	gtk_text_buffer_insert(im_output_buffer,&start_message_iter,buf,strlen(buf));
	gtk_text_buffer_get_end_iter(im_output_buffer, &end_message_iter);
	gtk_text_buffer_get_iter_at_mark(im_output_buffer,&start_message_iter,start_message_mark);
	//Apply the normal FONT tag to the selected text.
	gtk_text_buffer_apply_tag_by_name (im_output_buffer, "font", &start_message_iter, &end_message_iter);
	
	// scroll to bottom of display
	GtkTextIter output_end_iter;
	gtk_text_buffer_get_end_iter(im_output_buffer, &output_end_iter);
	GtkTextMark *last_pos = gtk_text_buffer_create_mark (im_output_buffer, "last_pos", &output_end_iter, FALSE);	
	gtk_text_view_scroll_mark_onscreen (GTK_TEXT_VIEW(im_page), last_pos);

	
	return 0;
}



/*
void ims_process_message(eXosip_event_t *je, char *msrp_message)
{
	Call *ca;

	//session based IM component exists
	if (find_call(current_dialog_id, &ca) >= 0 && ca->remote_msrp_port && pref->session_im_enabled) 
	{
		//this gets the URI that is not your own URI be it TO or From from the Call struct
		char from_uri[50];
		sprintf(from_uri,"%s",ca->from_uri);
		if(strcmp(from_uri,pref->impu) == 0)
		{
			sprintf(from_uri,"%s",ca->to_uri);
		}

		// writes the text to the IM output screen
		GtkWidget *im_output_text_view = lookup_widget(GTK_WIDGET(im_window),from_uri);
		GtkTextBuffer *im_output_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW(im_output_text_view));
	
		gchar buf [10000]= ""; //sender portion
		gchar buf1 [10000]= ""; //message portion
		
		//this removes "sip:" or "sips:"from from_uri
		//It is used for displaying message sender 
		char *temp;
		temp = strstr(from_uri,":") + 1;
		strcat (buf, temp);
	
		
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
		if(gtk_text_tag_table_lookup(tag_table,"green") == NULL)
		{
			// Tag with font fixed and tag name "italic". 
			gtk_text_buffer_create_tag (im_output_buffer, "green", "foreground", "darkgreen", NULL);
		}
		
		//if time stamp visible this prints the time the message was sent
		if(time_stamps == VISIBLE)
		{
			gchar buf2 [10000]= "";
			GtkTextIter start_message_iter;
			gtk_text_buffer_get_end_iter(im_output_buffer, &start_message_iter);
			GtkTextMark *start_message_mark = gtk_text_buffer_create_mark(im_output_buffer,"start_message_mark",&start_message_iter,TRUE);
			strcat (buf2, "(");
			char time[50];
			strcpy(time, imsua_get_time());
			strcat (buf2, time);
			strcat (buf2, ") ");
			gtk_text_buffer_insert(im_output_buffer,&start_message_iter,buf2,strlen(buf2));
			GtkTextIter end_message_iter;
			gtk_text_buffer_get_end_iter(im_output_buffer, &end_message_iter);
			gtk_text_buffer_get_iter_at_mark(im_output_buffer,&start_message_iter,start_message_mark);
			//Apply the GREEN tag to the selected text.
			gtk_text_buffer_apply_tag_by_name (im_output_buffer, "green", &start_message_iter, &end_message_iter);
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
		//Apply the BOLD tag to the selected text 
		gtk_text_buffer_apply_tag_by_name (im_output_buffer, "bold", &start_send_iter, &end_send_iter);
		//Apply the GREEN tag to the selected text.
		gtk_text_buffer_apply_tag_by_name (im_output_buffer, "green", &start_send_iter, &end_send_iter);
	
		//This writes the message portion is normal font for display on the screen
		GtkTextIter start_message_iter;
		gtk_text_buffer_get_end_iter(im_output_buffer, &start_message_iter);
		GtkTextMark *start_message_mark = gtk_text_buffer_create_mark(im_output_buffer,"start_message_mark",&start_message_iter,TRUE);
		strcat (buf1, msrp_message);
		strcat (buf1, "\n");
		gtk_text_buffer_insert(im_output_buffer,&start_message_iter,buf1,strlen(buf1));
		GtkTextIter end_message_iter;
		gtk_text_buffer_get_end_iter(im_output_buffer, &end_message_iter);
		gtk_text_buffer_get_iter_at_mark(im_output_buffer,&start_message_iter,start_message_mark);
		//Apply the normal FONT tag to the selected text 
		gtk_text_buffer_apply_tag_by_name (im_output_buffer, "font", &start_message_iter, &end_message_iter);
	
		//scrolls the viewing window down
		GtkTextIter output_end_iter;
		gtk_text_buffer_get_end_iter(im_output_buffer, &output_end_iter);
		GtkTextMark *last_pos = gtk_text_buffer_create_mark (im_output_buffer, "last_pos", &output_end_iter, FALSE);	
		gtk_text_view_scroll_mark_onscreen (GTK_TEXT_VIEW(im_output_text_view), last_pos);
		return ;
	}
	// get from_name and uri 
	osip_uri_t *uri;

	osip_from_t *from;
	osip_message_t *request = je->request;
	from = request->from;

	char *from_name;
	char *from_uri;
	char *message;
	
	osip_body_t *body;
	int i = osip_message_get_body(request,0,&body);
	message = body->body;
	
	from_name = osip_from_get_displayname(from);
	uri = osip_from_get_url(from);
	osip_uri_to_str(uri, &from_uri);

	// send 200OK in response to message received
	eXosip_lock (context_eXosip);
	eXosip_message_send_answer (context_eXosip, je->tid, 200, NULL);
	eXosip_unlock (context_eXosip);


	// writes the text to the IM output screen
	GtkWidget *im_output_text_view = lookup_widget(GTK_WIDGET(im_window),from_uri);
	GtkTextBuffer *im_output_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW(im_output_text_view));

	gchar buf [10000]= ""; //sender portion
	gchar buf1 [10000]= ""; //message portion
	
	if((from_name == NULL))
	{
		//this removes "sip:" or "sips:"from from_uri
		//It is used for displaying message sender 
		char *temp;
		temp = strstr(from_uri,":") + 1;
		strcat (buf, temp);
	}
	else
	{
		//this removes the inverted commas from around the name variable
		//It is used for displaying message sender 
		char *temp;
		temp = strstr(from_name,"\"") + 1;
		temp = strtok(temp,"\"");
		strcat (buf,temp);
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
	if(gtk_text_tag_table_lookup(tag_table,"green") == NULL)
	{
		// Tag with font fixed and tag name "italic". 
		gtk_text_buffer_create_tag (im_output_buffer, "green", "foreground", "darkgreen", NULL);
	}
	
	//if time stamp visible this prints the time the message was sent
	if(time_stamps == VISIBLE)
	{
		gchar buf2 [10000]= "";
		GtkTextIter start_message_iter;
		gtk_text_buffer_get_end_iter(im_output_buffer, &start_message_iter);
		GtkTextMark *start_message_mark = gtk_text_buffer_create_mark(im_output_buffer,"start_message_mark",&start_message_iter,TRUE);
		strcat (buf2, "(");
		char time[50];
		strcpy(time, imsua_get_time());
		strcat (buf2, time);
		strcat (buf2, ") ");
		gtk_text_buffer_insert(im_output_buffer,&start_message_iter,buf2,strlen(buf2));
		GtkTextIter end_message_iter;
		gtk_text_buffer_get_end_iter(im_output_buffer, &end_message_iter);
		gtk_text_buffer_get_iter_at_mark(im_output_buffer,&start_message_iter,start_message_mark);
		//Apply the GREEN tag to the selected text.
		gtk_text_buffer_apply_tag_by_name (im_output_buffer, "green", &start_message_iter, &end_message_iter);
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
	//Apply the BOLD tag to the selected text 
	gtk_text_buffer_apply_tag_by_name (im_output_buffer, "bold", &start_send_iter, &end_send_iter);
	//Apply the GREEN tag to the selected text.
	gtk_text_buffer_apply_tag_by_name (im_output_buffer, "green", &start_send_iter, &end_send_iter);

	//This writes the message portion is normal font for display on the screen
	GtkTextIter start_message_iter;
	gtk_text_buffer_get_end_iter(im_output_buffer, &start_message_iter);
	GtkTextMark *start_message_mark = gtk_text_buffer_create_mark(im_output_buffer,"start_message_mark",&start_message_iter,TRUE);
	strcat (buf1, message);
	strcat (buf1, "\n");
	gtk_text_buffer_insert(im_output_buffer,&start_message_iter,buf1,strlen(buf1));
	GtkTextIter end_message_iter;
	gtk_text_buffer_get_end_iter(im_output_buffer, &end_message_iter);
	gtk_text_buffer_get_iter_at_mark(im_output_buffer,&start_message_iter,start_message_mark);
	//Apply the normal FONT tag to the selected text 
	gtk_text_buffer_apply_tag_by_name (im_output_buffer, "font", &start_message_iter, &end_message_iter);

	//scrolls the viewing window down
	GtkTextIter output_end_iter;
	gtk_text_buffer_get_end_iter(im_output_buffer, &output_end_iter);
	GtkTextMark *last_pos = gtk_text_buffer_create_mark (im_output_buffer, "last_pos", &output_end_iter, FALSE);	
	gtk_text_view_scroll_mark_onscreen (GTK_TEXT_VIEW(im_output_text_view), last_pos);
	
}
*/

/*
void ims_start_im_session(eXosip_event_t *je, char *message)
{

	Call *ca;

	//session based IM component exists
	if (find_call(current_dialog_id, &ca) >= 0 && ca->remote_msrp_port && pref->session_im_enabled) 
	{
		//this gets the URI that is not your own URI be it TO or From from the Call struct
		char from_uri[50];
		sprintf(from_uri,"%s",ca->from_uri);
		if(strcmp(from_uri,pref->impu) == 0)
		{
			sprintf(from_uri,"%s",ca->to_uri);
		}
		// Create new IM window
		if(im_window_open == 0)
		{
			im_window = GTK_WINDOW(create_im_window());
			gtk_widget_show (GTK_WIDGET(im_window));
			im_window_open = 1;
			time_stamps = NOT_VISIBLE;
		}
		// Bring focus to already created IM window
		else
		{
				gtk_window_present(im_window);
		}
		
		// first time window is open special case
		if(num_im_tabs == 0)
		{
			num_im_tabs = 1;
			GtkWidget *im_tab_label = lookup_widget(GTK_WIDGET(im_window), "im_tab_1");
			GtkWidget *im_text = lookup_widget(GTK_WIDGET(im_window), "im_text_view");
			// Store pointers to all widgets, for use by lookup_widget().
			GLADE_HOOKUP_OBJECT_NO_REF (GTK_WIDGET(im_window), GTK_WIDGET(im_text), from_uri);
			gtk_label_set_text(GTK_LABEL(im_tab_label),from_uri);
		}
		// Decides which IM tab to bring focus to or to open a new one
		else
		{
			GtkWidget *im_label = gtk_label_new(from_uri);
			GtkWidget *notebook = lookup_widget(GTK_WIDGET(im_window), "im_notebook");
							
			int i = 0;
			int found_tab = 0;
			GtkWidget *im_tab = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook),i);
			GtkWidget *im_tab_label = gtk_notebook_get_tab_label(GTK_NOTEBOOK(notebook),im_tab);
			for(i = 0; i < num_im_tabs; i++)
			{
				
				const char *tab_label = gtk_label_get_text(GTK_LABEL(gtk_notebook_get_tab_label(GTK_NOTEBOOK(notebook),gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook),i))));
				if(strcmp(from_uri,tab_label) == 0)
				{
					found_tab = 1;
					break;
				}
			}	
			if(found_tab == 1)	//bring tab to focus
			{
				gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook),i);
			}
			else			//create new tab
			{
				GtkWidget *im_text = gtk_text_view_new();
				// Store pointers to all widgets, for use by lookup_widget().
				GLADE_HOOKUP_OBJECT_NO_REF (GTK_WIDGET(im_window), GTK_WIDGET(im_text), from_uri);
							
				gtk_text_view_set_editable(GTK_TEXT_VIEW(im_text),FALSE);
				gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(im_text),FALSE);
				
				GtkWidget *im_label = gtk_label_new(from_uri);
				gtk_notebook_append_page(GTK_NOTEBOOK(notebook),GTK_WIDGET(im_text),GTK_WIDGET(im_label));
				gtk_widget_show (GTK_WIDGET(im_text));
				num_im_tabs++;
				gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook),num_im_tabs-1);
			}
			
		}
		if(message)
		{
			ims_process_message(je, message);
			
			
		}
		return ;
	}
	
	int status;
	
	GtkWidget *is_writing;
	
	// Create new IM window
	if(im_window_open == 0)
	{
		im_window = GTK_WINDOW(create_im_window());
		gtk_widget_show (GTK_WIDGET(im_window));
		im_window_open = 1;
		time_stamps = NOT_VISIBLE;
	}
	// Bring focus to already created IM window
	else
	{
			gtk_window_present(im_window);
	}
	
	//get 'from uri'
	osip_uri_t *uri;
	osip_from_t *from;
	osip_message_t *request = je->request;
	from = request->from;
	char *from_uri;
	uri = osip_from_get_url(from);
	osip_uri_to_str(uri, &from_uri);

	osip_content_type_t *content_type;
	content_type = osip_message_get_content_type(request);

	//check if message received is a status message
	if ((osip_strcasecmp(content_type->type, "application")==0 && osip_strcasecmp(content_type->subtype, "im-iscomposing+xml")==0 ))
	{
		
		char *message;
		char state[100];
	
		osip_body_t *body;
		int i = osip_message_get_body(request,0,&body);
		message = body->body;

		xmlDocPtr doc;
		xmlNodePtr cur, child;

		if(!(doc = xmlParseMemory(message, strlen(message))))
		{
			g_warning("Error opening XML doc");
			xmlFreeDoc( doc );
		}
		if(!(cur = xmlDocGetRootElement(doc)))
		{
			g_warning("IM Status XML document has no root element");
			xmlFreeDoc( doc );
		}
		cur = cur->xmlChildrenNode;
		while(cur)
		{
			if (cur->type == XML_ELEMENT_NODE)
			{	
				if(child = cur->xmlChildrenNode)
				{
	
					if(!xmlStrcmp(cur->name, (const xmlChar *)"state"))
						strcpy(state, child->content);
				}
			}
			cur = cur->next;
		}
		
		if(strcmp(state,"active") == 0)
		{
			status = 1;
		}
		else if(strcmp(state,"idle") == 0)
		{
			status = 0;
		}
		

		is_writing = lookup_widget(GTK_WIDGET(im_window), "is_writing");
		
		// send 200OK in response to message received
		eXosip_lock (context_eXosip);
		eXosip_message_send_answer (context_eXosip, je->tid, 200, NULL);
		eXosip_unlock (context_eXosip);

	}
	
	
	// first time window is open special case
	if(num_im_tabs == 0)
	{
		num_im_tabs = 1;
		GtkWidget *im_tab_label = lookup_widget(GTK_WIDGET(im_window), "im_tab_1");
		GtkWidget *im_text = lookup_widget(GTK_WIDGET(im_window), "im_text_view");
		// Store pointers to all widgets, for use by lookup_widget().
		GLADE_HOOKUP_OBJECT_NO_REF (GTK_WIDGET(im_window), GTK_WIDGET(im_text), from_uri);
		gtk_label_set_text(GTK_LABEL(im_tab_label),from_uri);

		if(status == 1)
		{
			// writes the text to the IM output screen
			GtkWidget *im_output_text_view = lookup_widget(GTK_WIDGET(im_window),from_uri);
			GtkTextBuffer *im_output_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW(im_output_text_view));
		
			//create BOLD font tag
			GtkTextTagTable *tag_table = gtk_text_buffer_get_tag_table(im_output_buffer);
			if(gtk_text_tag_table_lookup(tag_table,"bold") == NULL)
			{
				// Tag with weight bold and tag name "bold"
				gtk_text_buffer_create_tag (im_output_buffer,"bold","weight", PANGO_WEIGHT_BOLD, NULL);
			}
			//This writes the sender portion of the message in bold for display on the screen
			GtkTextIter start_send_iter;
			gtk_text_buffer_get_end_iter(im_output_buffer, &start_send_iter);
			GtkTextMark *start_send_mark = gtk_text_buffer_create_mark(im_output_buffer,"start_send_mark",&start_send_iter,TRUE);
			char buf [256];
			sprintf(buf, "%s is typing a message to you...\n", from_uri);
			gtk_text_buffer_insert(im_output_buffer,&start_send_iter,buf,strlen(buf));
			GtkTextIter end_send_iter;
			gtk_text_buffer_get_end_iter(im_output_buffer, &end_send_iter);	
			gtk_text_buffer_get_iter_at_mark(im_output_buffer,&start_send_iter,start_send_mark);
			//Apply the BOLD tag to the selected text 
			gtk_text_buffer_apply_tag_by_name (im_output_buffer, "bold", &start_send_iter, &end_send_iter);
			gtk_widget_show (GTK_WIDGET(is_writing));
		}
	}
	// Decides which IM tab to bring focus to or to open a new one
	else
	{
		GtkWidget *im_label = gtk_label_new(from_uri);
		GtkWidget *notebook = lookup_widget(GTK_WIDGET(im_window), "im_notebook");
						
		int i = 0;
		int found_tab = 0;
		GtkWidget *im_tab = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook),i);
		GtkWidget *im_tab_label = gtk_notebook_get_tab_label(GTK_NOTEBOOK(notebook),im_tab);
		for(i = 0; i < num_im_tabs; i++)
		{
			
			const char *tab_label = gtk_label_get_text(GTK_LABEL(gtk_notebook_get_tab_label(GTK_NOTEBOOK(notebook),gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook),i))));
			if(strcmp(from_uri,tab_label) == 0)
			{
				found_tab = 1;
				break;
			}
		}	
		if(found_tab == 1)	//bring tab to focus
		{
			gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook),i);
 			if(status == 1)
			{
				gtk_widget_show (GTK_WIDGET(is_writing));
			}
			else if(status == 0)
			{
				is_writing = lookup_widget(GTK_WIDGET(im_window), "is_writing");
				gtk_widget_hide (GTK_WIDGET(is_writing));
			}
		}
		else			//create new tab
		{
			GtkWidget *im_text = gtk_text_view_new();
			// Store pointers to all widgets, for use by lookup_widget().
  			GLADE_HOOKUP_OBJECT_NO_REF (GTK_WIDGET(im_window), GTK_WIDGET(im_text), from_uri);
						
			gtk_text_view_set_editable(GTK_TEXT_VIEW(im_text),FALSE);
			gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(im_text),FALSE);
			
			GtkWidget *im_label = gtk_label_new(from_uri);
			gtk_notebook_append_page(GTK_NOTEBOOK(notebook),GTK_WIDGET(im_text),GTK_WIDGET(im_label));
			gtk_widget_show (GTK_WIDGET(im_text));
			num_im_tabs++;
			gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook),num_im_tabs-1);
			if(status == 1)
			{
				// writes the text to the IM output screen
				GtkWidget *im_output_text_view = lookup_widget(GTK_WIDGET(im_window),from_uri);
				GtkTextBuffer *im_output_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW(im_output_text_view));
			
				//create BOLD font tag
				GtkTextTagTable *tag_table = gtk_text_buffer_get_tag_table(im_output_buffer);
				if(gtk_text_tag_table_lookup(tag_table,"bold") == NULL)
				{
					// Tag with weight bold and tag name "bold"
					gtk_text_buffer_create_tag (im_output_buffer,"bold","weight", PANGO_WEIGHT_BOLD, NULL);
				}
				//This writes the sender portion of the message in bold for display on the screen
				GtkTextIter start_send_iter;
				gtk_text_buffer_get_end_iter(im_output_buffer, &start_send_iter);
				GtkTextMark *start_send_mark = gtk_text_buffer_create_mark(im_output_buffer,"start_send_mark",&start_send_iter,TRUE);
				char buf [256];
				sprintf(buf, "%s is typing a message to you...\n", from_uri);
				gtk_text_buffer_insert(im_output_buffer,&start_send_iter,buf,strlen(buf));
				GtkTextIter end_send_iter;
				gtk_text_buffer_get_end_iter(im_output_buffer, &end_send_iter);	
				gtk_text_buffer_get_iter_at_mark(im_output_buffer,&start_send_iter,start_send_mark);
				//Apply the BOLD tag to the selected text 
				gtk_text_buffer_apply_tag_by_name (im_output_buffer, "bold", &start_send_iter, &end_send_iter);
				gtk_widget_show (GTK_WIDGET(is_writing));
			}
		}
		
	}
	if (osip_strcasecmp(content_type->type, "text")==0 && osip_strcasecmp(content_type->subtype, "plain")==0 )
	{
		ims_process_message(je, message);
		is_writing = lookup_widget(GTK_WIDGET(im_window), "is_writing");
		gtk_widget_hide ((GTK_WIDGET(is_writing)));
	}
}
*/

void ims_process_notify(eXosip_event_t *je)
{

	if(!je->request)
		return ;

	imsua_set_message_display("OK (NOTIFY)", 1);

	osip_header_t *event_header = NULL;

	if(!osip_message_header_get_byname(je->request, "Event", 0, &event_header))
	{
		fprintf(stderr, "UCTIMSCLIENT: Received unknown NOTIFY message\n");
		return ;
	}

	// decide if we have a reg event or a presence notify 
	if (!strcmp(event_header->hvalue, "presence"))
	{
		//presence_process_notify(je);
	}
	else if (!strcmp(event_header->hvalue, "presence.winfo"))
	{
		//watchers_process_notify(je);
	}
	else if (!strcmp(event_header->hvalue, "reg"))
	{

		osip_body_t *notify_body = NULL;

		// if we are not currently subscribed exit (otherwise osip seg-faults)
		if (je->ss_status != 2)
		{
			//set_reg_event_display("No active subscriptions");
			return ;
		}
	
		// get the XML body from the NOTIFY message

		osip_message_get_body(je->request, 0, &notify_body);

		char *notify_str = NULL;
		size_t reg_len;

		osip_body_to_str(notify_body, &notify_str, &reg_len);

	
		// display in reg event tab -- future work: parse this XML
		// set_reg_event_display(notify_str);
	}
	else
	{
		fprintf(stderr, "UCTIMSCLIENT: Unknown NOTIFY event type\n");
		return ;
	}

}


int ims_process_subscription_answered(eXosip_event_t *je)
{

	osip_header_t *event_header;

	if(!osip_message_header_get_byname(je->request, "Event", 0, &event_header))
		return -1;

	// decide what type of event we have
	if (!strcmp(event_header->hvalue, "presence"))
	{
		//presence_process_subscription_answered(je);
	}
	else if (!strcmp(event_header->hvalue, "presence.winfo"))
	{
		//watchers_process_subscription_answered(je);
	}

}

void ims_process_incoming_reinvite (eXosip_event_t *je)
{
	Call *ca;
	Preferences *pref = client->pref;

	if (find_call(je->did, &ca) < 0)
		return ;
	// Extract call information from message
	message_extract_call_info(ca, je->request);
	
	if(ca->remote_msrp_port && local_msrp_endpoint)
	{
		//endpointmsrp_end_session(0);
	}
	//If message had an MSRP component must initialise an end point
	if(ca->remote_msrp_port && pref->session_im_enabled)
	{

		//local_msrp_endpoint = endpointmsrp_create_endpoint(je->cid,pref->local_audio_ip,pref->local_msrp_port,1);
	}
		
	ca->qos_local_state = NONE;
	ca->caller = 0;

	sprintf(display, "Re- received from\n\n%s <%s>", ca->from_name, ca->from_uri);
	set_display(display);

	// Send 200 OK with local SDP parameters
	osip_message_t *answer_200ok;

	eXosip_lock (context_eXosip);
	eXosip_call_build_answer(context_eXosip, je->tid, 200, &answer_200ok);
	eXosip_unlock (context_eXosip);
	
	sdp_complete_ims(je->request, &answer_200ok, ca, 0);

	eXosip_lock (context_eXosip);
	eXosip_call_send_answer(context_eXosip, je->tid, 200, answer_200ok);
	eXosip_unlock (context_eXosip);
	

}

void ims_process_302 (eXosip_event_t *je)
{
	Preferences *pref = client->pref;
	char display[500];

	//Get CONTACT URI from 302 response
	char *contact;
	osip_contact_t *contact_header;
	osip_message_get_contact((osip_message_t *)je->response,0, &contact_header);
	if(contact_header)
	{
		osip_contact_to_str(contact_header, &contact);
	}

	sprintf(display, "Received 302 Temporarily\n Moved Response \n\nRedirecting to:\n  %s" , contact);
	set_display(display);

	//send new INVITE to new CONTACT URI
	// the invite message
	osip_message_t *invite = NULL;

	
	char *uri_entry;
        uri_entry = (char*)malloc(sizeof(contact));

	sprintf(uri_entry,"%s",contact);
	char from_string[200];

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
	else if (eXosip_call_build_initial_invite (context_eXosip, &invite, uri_entry, from_string, add_lr_to_route(add_sip_scheme(pref->pcscf)), "IMS Call"))
	{
		set_display("Invalid Destination URI or PCSCF\n\nCheck both start with \"sip:\"");
		return ;
	}

	// add the service route from (obtained during registration)
	int j;
	for (j = num_service_routes; j > 0; j--)
	{
		osip_message_set_route(invite, ims_service_route[j-1]);
	}
	
	// Support for RFC 3325 - Private Extensions to the Session Initiation Protocol (SIP)
	// for Asserted Identity within Trusted Networks
	osip_message_set_header((osip_message_t *)invite,(const char *)"P-Preferred-Identity",from_string);

	osip_message_set_header((osip_message_t *)invite,(const char *)"Privacy","none");

	// Support for RFC 3455 - Private Header (P-Header) Extensions to the Session Initiation
    	// Protocol (SIP) for the 3rd-Generation Partnership Project (3GPP)
	osip_message_set_header((osip_message_t *)invite,(const char *)"P-Access-Network-Info",access_networks[pref->access_network]);

	// require preconditions if QoS required
	if (pref->qos_strength != QOS_NONE)
		osip_message_set_require(invite, "precondition");

	osip_message_set_require(invite, "sec-agree");
	osip_message_set_proxy_require(invite, "sec-agree");
	osip_message_set_supported(invite, "100rel");

	// Put in our media preferences
	sdp_complete_ims(NULL, &invite, NULL, 0);

	// send the invite
	eXosip_lock (context_eXosip);
	int i = eXosip_call_send_initial_invite (context_eXosip, invite);
	eXosip_unlock (context_eXosip);

	if (i == 0)
		set_display("Error sending IMS invite");
	else
	{
		imsua_set_message_display("INVITE", 1);
	}
}


