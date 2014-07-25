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

#ifndef RTSP_H
#define RTSP_H

/**
 * @defgroup uctimsclient_rtsp RTSP
 * @ingroup uctimsclient_general
 * @{
 */

void rtsp_resume_playback();

void rtsp_pause_playback();

void rtsp_stop_playback();

void rtsp_rewind_playback();

void rtsp_forward_playback();

void rtsp_change_volume();

void rtsp_start_session();

void rtsp_end_session();


/** @} */

#endif
