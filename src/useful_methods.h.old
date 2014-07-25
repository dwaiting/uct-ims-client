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

#ifndef USEFUL_METHODS_H
#define USEFUL_METHODS_H


/*
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <eXosip2/eXosip.h>
#include <gtk/gtk.h>
#include "imsUA.h"
*/

/**
 * @defgroup uctimsclient_useful_methods Useful methods
 * @ingroup uctimsclient_general
 * @{
 */


/**
 * Encapsulates a string in quotes.
 * 
 * @param in - the orginal string
 * @returns the new string encapsulated in quotes
 */
char* add_quotes(char *in);

/**
 * Removes outer quotes from a string.
 * 
 * @param in - the orginal string
 * @returns the new string without quotes
 */
char *imsua_remove_quotes(char *text);

/**
 * Adds the sip: scheme to a string
 * 
 * @param in - the orginal string
 * @returns the new string with sip: prepended
 */
char* add_sip_scheme(char *in);


/**
 * Adds the lr parameter to a route string.
 * 
 * @param route - the orginal route
 * @returns the new route with the lr parameter
 */
char *imsua_add_lr_to_route(char *route);


char *get_uuid(uuid_t u);

/**
 * Sets the text of the Display widget.
 * 
 * @param text - the text
 */
void set_display(char *text);

/**
 * Sets the text of the reg event display widget.
 * 
 * @param text - the text
 */
void set_reg_event_display(char *text);

/**
 * Appends text to the Display widget.
 * 
 * @param text - the text value to append
 */
void imsua_append_text_to_display(char *text);

/**
 * Appends text to the reg event display widget.
 * 
 * @param text - the text value to append
 */
void imsua_append_text_to_reg_event_display(char *text);

/**
 * Appends text to the assoc uri display widget.
 * 
 * @param text - the text value to append
 */
// void imsua_append_text_to_assoc_uri_display(char *text);


/**
 * Sets presentity display.
 * 
 * @param new_presentity - the text value to set
 */
void imsua_set_presentity_display(char *new_presentity);


/**
 * Clears the buddy list display
 * 
*/
void imsua_clear_buddy_list_display();

/**
 * Appends text to buddy list display.
 * 
 * @param buddy - the buddy address to append
 * @param basic - the basic parameter to append 
 * @param note - the note parameter to append
*/
void imsua_append_text_to_buddy_list_display(char *buddy, char *basic, char *note);


/**
 * Appends text to message display.
 * 
 * @param text - the text value to append
 * @param color - the font color - 1 for red and 2 for green
 */
void imsua_set_message_display(char *text, int color);

/**
 * Clears the pres-rules display.
 * 
 */
void imsua_clear_presrules_display();

/**
 * Appends text to pres-rules display.
 * 
 * @param text - the text value to append
 */
void imsua_set_presrules_display(char *text);

/**
 * Show the event info in the message display.
 * 
 * @param je - the eXosip event
 */
void imsua_display_event_info(eXosip_event_t *je);

/**
 * Check to see if this uri is one of our associated uris.
 * 
 * @param uri - the uri to check
 */
int imsua_is_associated_uri(char *uri);

/**
 * Add the service routes learnt during registration.
 * 
 * @param message - the osip message to add the service routes
 */
void imsua_add_service_routes(osip_message_t **message);

/**
 * Sets the text of the Status Bar widget.
 * 
 * @param text - the text value
 */
void set_status_bar(char *text);

/**
 * Writes the clock time to a string
 * 
 * @param time_str - the pointer to store the time in
 * 
 */
char* imsua_get_time();

/**
 * Subtract y from x timevals and return the answer. This is useful for timing network delays.
 * 
 * @param result - the difference between the two times
 * @param x - value at end of timer
 * @param y - value at start of timer
 * 
 */
int timeval_subtract (struct timeval *result, struct timeval *x, struct timeval *y);

/**
 * Add file path to filename.
 * 
 * @param result - the full path and filename
 * @param filename - filename without path
 * 
 */
char *imsua_addpath(char *filename);


/**
 * Match regular expression.
 * 
 * @param result - returns 1 on success, 0 otherwise
 * @param string - the string
 * @param pattern - the regular expression
 * 
 */
int imsua_regex_match(char *string, char *pattern);


/**
 * Print debugging information to stderr.
 * 
 * @param message - the debugging message
 * 
 */
void imsua_log(char *message);

/**
 * Set the text in the XDMS download display area
 * 
 * @param text - the text message
 * 
 */
void set_xdms_download_display(char *text);

/**
 * Set the text in the XDMS upload display area
 * 
 * @param text - the text message
 * 
 */

void set_xdms_upload_display(char *text);

/** @} */

#endif
