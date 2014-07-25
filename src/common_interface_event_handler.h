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

#ifndef COMMON_INTERFACE_EVENT_HANDLER_H
#define COMMON_INTERFACE_EVENT_HANDLER_H



/*
#include <gtk/gtk.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include "imsUA.h"
#include "support.h"
#include "common_exosip_event_handler.h"
*/

/**
 * @defgroup uctimsclient_common_interface_event_handler Methods to handle both SIP and IMS interface events
 * @ingroup uctimsclient_event_handlers
 * @{
 */


/**
 * Sets the mode into either IMS or SIP mode, used only on startup
 */
void set_mode();


/**
 * Terminates a call, method is the same for SIP and IMS mode
 */
void terminate_call();

/**
 * Rejects a call, method is the same for SIP and IMS mode
 */
void reject_call();

/**
 * Starts an Instant Messaging session as a result of sending a MESSAGE
 * Opens an IM Window and brings correct tab to the foreground
 * Method is the same for IMS and SIP mode
 * @param chat_uri_entry - URI to start the IM session with
 */
void common_start_im_session(const gchar *chat_uri_entry);

/**
 * Sends DTMF tones in an NFO message
 * @param val - the value of the DTMF tone to send
 */
void common_send_dtmf(int val);


/** @} */

#endif
