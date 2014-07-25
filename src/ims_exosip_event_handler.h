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

#ifndef IMS_EXOSIP_EVENT_HANDLER_H
#define IMS_EXOSIP_EVENT_HANDLER_H



/*
#include <eXosip2/eXosip.h>
#include <gtk/gtk.h>
#include "interface.h"
#include "base64.h"
#include "milenage.h"
#include "rfc2617.h"
#include "md5.h"
#include "DigestAKAv1MD5.h"
#include "useful_methods.h"
#include "common_exosip_event_handler.h"
#include "support.h"
*/

/**
 * @defgroup uctimsclient_ims_exosip_event_handler Methods to handle IMS eXosip events
 * @ingroup uctimsclient_event_handlers
 * @{
 */


/**
 * Processes an IMS 401 response
 * 
 * @param je - a pointer to the 401 event 
 */
void ims_process_401(eXosip_event_t *je);

/**
 * Processes an 200 OK resonse to a register request
 * 
 * @param je - a pointer to the 200 OK event
 */
void ims_process_registration_200ok(eXosip_event_t *je);

/**
 * Determines if an IMS message has preconditions
 * 
 * @param message - a pointer to the message to check
 * @returns 0 if no preconditions and 1 if there are preconditions 
 * 
 */
int ims_message_requires_preconditions(osip_message_t *message);

/**
 * Determines if an IMS message requires reliable responses
 * 
 * @param message - a pointer to the message to check
 * @returns 0 if no reliable responses are required and 1 otherwise
 * 
 */
int ims_message_requires_100rel(osip_message_t *message);

/**
 * Processes an IMS INVITE request
 * 
 * @param je - a pointer to the INVITE event
 */
void ims_process_incoming_invite(eXosip_event_t *je);

/**
 * Processes an IMS 18X response
 * 
 * @param je - a pointer to the 18X event
 */
void ims_process_18x(eXosip_event_t *je);

/**
 * Processes a IMS PRACK response
 * 
 * @param je - a pointer to the PRACK event
 */
void ims_process_prack(eXosip_event_t *je);

/**
 * Processes a IMS UPDATE request
 * 
 * @param je - a pointer to the UPDATE event
 */
void ims_process_update(eXosip_event_t *je);

/**
 * Processes a IMS 2XX response
 * 
 * @param je - a pointer to the 2XX event
 */
void ims_process_2xx(eXosip_event_t *je);

/**
 * Processes a IMS 200 OK response
 * 
 * @param je - a pointer to the 200 OK event
 */
void ims_process_200ok(eXosip_event_t *je);

/**
 * Processes a IMS ACK response
 * 
 * @param je - a pointer to the ACK event
 */
void ims_process_ack(eXosip_event_t *je);

/**
 * Processes a released call
 * 
 * @param je - a pointer to the exosip event
 */
void ims_process_released_call(eXosip_event_t *je);


/**
 * Processes a IMS out of call MESSAGE
 * 
 * @param je - a pointer to the MESSAGE event
 */
int ims_process_message (MsrpEndpoint *endpoint, char *msrp_message);

/**
 * Starts an Instant Messaging session as a result of receiving an IMS out of call MESSAGE
 * 
 * @param je - a pointer to the MESSAGE event
 */
void ims_start_im_session(eXosip_event_t *je, char *message);

void ims_process_notify(eXosip_event_t *je);

/**
 * Process a 200 OK message for an outgoing subscription
 * 
 * @param je - a pointer to the MESSAGE event
 */
int ims_process_subscription_answered(eXosip_event_t *je);

/**
 * Processes an INVITE within a call
 * 
 * @param je - a pointer to the INVITE event
 */
void ims_process_incoming_reinvite (eXosip_event_t *je);

/**
 * Processes an 302 Temporarily Moved Response
 * 
 * @param je - a pointer to the 302 event
 */
void ims_process_302 (eXosip_event_t *je);


/** @} */

#endif

