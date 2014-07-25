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

#ifndef COMMON_EXOSIP_EVENT_HANDLER_H
#define COMMON_EXOSIP_EVENT_HANDLER_H


/*
#include <eXosip2/eXosip.h>
#include <ortp/ortp.h>
#include <stdlib.h>
#include <glib.h>
#include "media.h"
#include "imsUA.h"
#include "useful_methods.h"
*/

/**
 * @defgroup uctimsclient_common_exosip_event_handler Methods to handle both SIP and IMS eXosip events
 * @ingroup uctimsclient_event_handlers
 * @{
 */

/**
 * Finds the call with the given call ID in the call list
 * 
 * @param cid - The call ID to search for
 * @returns the Call struct or NULL if not found  
*/
// Call **find_call(int did);
int find_call(int did, Call **ca);

int find_im_call_by_uri(char *uri, Call **ca);

int find_call_by_cid(int cid, Call **ca);


/**
 * Adds a call to the call_list
 * 
 * @param new_call - The call to add to the list
 */
void add_call(Call **ca);

/**
 * Removes the call with the given call ID from the call list
 * 
 * @param cid - The call ID of the call to be removed
 */
void delete_call(int cid);

void print_calls();

/**
 * Removes a call from the call list as a result of the call being released by the remote host
 * or of a global failure
 * 
 * @param je - a pointer to the event that caused the call to be releaseed 
 */
void call_released(eXosip_event_t *je);

/**
 * Sets up and starts an RTP session
 * 
 * @param je - a pointer to the event that caused the rtp session to be initiated
 */

/**
 * Process a INFO message, used for transmitting DTMF tones
 * 
 * @param je - a pointer to the INFO event
 */
void common_process_info(eXosip_event_t *je);

/** @} */

#endif
