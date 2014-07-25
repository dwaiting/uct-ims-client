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

#ifndef IMS_INTERFACE_EVENT_HANDLER_H
#define IMS_INTERFACE_EVENT_HANDLER_H



/*
#include <gtk/gtk.h>
#include <stdlib.h>
#include <eXosip2/eXosip.h>
#include "imsUA.h"
#include "interface.h"
#include "support.h"
#include "useful_methods.h"
#include "common_exosip_event_handler.h"
*/

/**
 * @defgroup uctimsclient_ims_interface_event_handler Methods to handle IMS interface events
 * @ingroup uctimsclient_event_handlers
 * @{
 */


/**
 * Initiates an IMS call by sending an IMS INVITE request
 * 
 */
int ims_call_initiate(char *uri_entry, int session_type);


/**
 * Sends an out of call IMS MESSAGE indicating IsComposing status
 * 
 */
void ims_send_instant_message_status (int status);

/**
 * Sends an out of call IMS MESSAGE
 * 
 */
int ims_send_instant_message (char *uri_entry, char *im_text_entry);

/**
 * Sends an IMS REGISTER request 
 * 
 */
int ims_send_register();

/**
 * Checks for correct URI's,etc. and opens an IM Window if 1 is'nt already open 
 * 
 */
void ims_open_im_window();

/**
 * Answers an IMS call and sends a 200 OK response
 */
void ims_call_answer();

/**
 * Sends an IMS DEREGISTER request 
 */
void ims_send_deregister_message ();

/**
 * Sends an IMS INVITE within a call 
 */
void ims_call_reinvite();

/** @} */

#endif


