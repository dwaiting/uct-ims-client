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

#ifndef SDP_METHODS
#define SDP_METHODS



/*
#include <eXosip2/eXosip.h>
#include "imsUA.h"
*/

/**
 * @defgroup uctimsclient_sdp_methods SDP body helpers
 * @ingroup uctimsclient_general
 * @{
 */


/**
 * Completes the reply SDP body of an IMS message
 * 
 * @param message - The received message
 * @param answer - The osip message with the completed SDP body
 * @param ca - The current call
 * @param session_type - 1 = audio/video; 0 = IM
*/
int sdp_complete_ims(osip_message_t *message, osip_message_t **answer, Call *ca, int session_type);

/** @} */

#endif

