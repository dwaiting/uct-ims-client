/*
  The UCT IMS Client
  Copyright (C) University of Cape Town
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

#ifndef GSTREAMER_H
#define GSTREAMER_H

/**
 * @defgroup uctimsclient_gstreamer GStreamer
 * @ingroup uctimsclient_general
 * @{
 */

GtkWidget* create_videoWin (void);

int testGstreamer();

int initialiseRingingPipeline();

int initialiseBackgroundVideoPipeline();

int initialiseVideoTxPipeline(Call *ca);

int initialiseVideoRxPipeline(Call *ca);

int initialiseAudioTxPipeline(Call *ca);

int initialiseAudioRxPipeline(Call *ca);

int destroyVideoTxPipeline(Call *call);

int destroyVideoRxPipeline(Call *call);

int destroyAudioTxPipeline(Call *ca);

int destroyAudioRxPipeline(Call *ca);

/** @} */

#endif
