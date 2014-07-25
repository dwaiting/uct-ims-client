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

#ifndef PREFERENCES_H
#define PREFERENCES_H



/*
#include <gtk/gtk.h>
#include <stdlib.h>
#include <glib.h>
#include <string.h>
#include "imsUA.h"
*/

/**
 * @defgroup uctimsclient_preferences Preferences
 * @ingroup uctimsclient_general
 * @{
 */

/**
 * Removes newline char from the end of a string.
 * 
 * @param input - the orginal string
 * @returns the new string without the newline char
 */
char *preferences_remove_newline(char *input);

/**
 * Set all the fields in the preferences dialog.
 * 
 */
void preferences_set_preferences_dialog();

/**
 * Get all the fields from the preferences dialog.
 * 
 * @param button - a pointer to the OK button in the preferences dialog
 */
void preferences_get_preferences_dialog();

/**
 * Fetches the preferences from the specified file.
 * 
 * @param filename - path and filename of preferences file (defaults to ".imsuapref")
 */
int preferences_get_preferences_from_xml_file(Preferences *pref, char *filename);

/**
 * Write the preferences to the specified file.
 * 
 * @param filename - path and filename of preferences file (defaults to ".imsuapref")
 */
int preferences_write_preferences_to_xml_file(Preferences *pref, char *filename);

/**
 * Set the default preferences.
 * 
 */
int preferences_set_default_preferences(Preferences *pref);

/** @} */

#endif
