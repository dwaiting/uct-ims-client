/*
  The UCT IMS Client
  Copyright (C) 2006 - 2012 - University of Cape Town
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


#ifndef MAIN_H
#define MAIN_H

/**
 * @defgroup uctimsclient_main Main
 * @ingroup uctimsclient_general
 * @{
 */


/**
 * Initialise eXosip
 */
int initialise_eXosip();


/**
 * Fetch any waiting eXosip events
 * 
 * @param main_window - a pointer to the main window widget
 */
gint get_exosip_events();

int initialise_ui();

gboolean shutdown_ui(GtkWidget *widget, GdkEvent *event, gpointer userdata);

int main( int argc, char *argv[] );


#endif
