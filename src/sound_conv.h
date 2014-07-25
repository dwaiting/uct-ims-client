/*
  The UCT IMS Client
  Copyright (C) 2006  
  David Waiting <david@crg.ee.uct.ac.za>
  Richard Good <rgood@crg.ee.uct.ac.za>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#ifndef SOUND_CONV_H
#define SOUND_CONV_H

/**
 * @defgroup uctimsclient_sound_conv Sound converter
 * @ingroup uctimsclient_general
 * @{
 */



int val_seg (int val);

unsigned char s16_to_alaw (int pcm_val);

/*
 * alaw_to_s16() - Convert an A-law value to 16-bit linear PCM
 *
 */
int alaw_to_s16 (unsigned char a_val);

unsigned char s16_to_ulaw (int pcm_val);

int ulaw_to_s16 (unsigned char u_val);

void mulaw_dec (char *mulaw_data, char *s16_data, int size);

void mulaw_enc (char *s16_data, char *mulaw_data, int pcm_size);

void
alaw_dec (char *alaw_data /* contains size char */ ,
          char *s16_data /* contains size*2 char */ ,
          int size);

void
alaw_enc (char *s16_data /* contains 320 char */ ,
          char *alaw_data /* contains 160 char */ ,
          int pcm_size);


/** @} */

#endif

