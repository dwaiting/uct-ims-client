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
 
/**
 * \file
 * 
 * SIP-to-IMS Gateway - Rijndael
 * 
 *  \note Taken from 3GPP's TS 35.206
 * 
 *                      Rijndael Implementation
 *
 *  A sample 32-bit orientated implementation of Rijndael, the
 *  suggested kernel for the example 3GPP authentication and key
 *  agreement functions.
 *
 *  This implementation draws on the description in section 5.2 of
 *  the AES proposal and also on the implementation by
 *  Dr B. R. Gladman <brg@gladman.uk.net> 9th October 2000.
 *  It uses a number of large (4k) lookup tables to implement the
 *  algorithm in an efficient manner.
 *
 *  Note: in this implementation the State is stored in four 32-bit
 *  words, one per column of the State, with the top byte of the
 *  column being the _least_ significant byte of the word.
 *
 */

#ifndef RIJNDAEL_H
#define RIJNDAEL_H


void RijndaelKeySchedule( u8 key[16] );
void RijndaelEncrypt( u8 input[16], u8 output[16] );


#endif
