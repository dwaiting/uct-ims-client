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

#ifndef MEDIA_H
#define MEDIA_H



/*
#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>
#include "imsUA.h"
#include "sound_conv.c"
*/


/**
 * @defgroup uctimsclient_media Media
 * @ingroup uctimsclient_general
 * @{
 */

int message_extract_call_info(Call *call, osip_message_t *message);

void media_start_session(eXosip_event_t *je);

int media_open_nat(char *their_ip, int their_port, int my_port);

/** @} */

#endif
