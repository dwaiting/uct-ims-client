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

#ifndef WATCHERS_H
#define WATCHERS_H

/**
 * @defgroup uctimsclient_watchers Watchers
 * @ingroup uctimsclient_general
 * @{
 */


/**
 * Subscribe to the watcher info event.
 * @param expires - length that subscriptions are valid
 * 
 */
gint watchers_subscribe_to_watcherinfo(int expires);

/**
 * Parses the XML from NOTIFY messages.
 * 
 * @param root_node - the root node of the XML document
 */
void watchers_get_xml_elements(xmlNode* root_node);

/**
 * Handle an incoming NOTIFY event.
 * 
 * @param je - the eXosip event
 */
void watchers_process_notify(eXosip_event_t *je);

/**
 * Find a particular watcher in the list of watchers.
 * 
 * @param uri - the uri of the watcher to find
 */
watcherdata *watchers_find_watcher(char *uri);

/**
 * Add a watcher to the list of watchers.
 * 
 * @param watcher - a new watcher struct
 */
void watchers_add_watcher(watcherdata *watcher);

/**
 * Remove a watcher from the list of watchers.
 * 
 * @param uri - the uri of the watcher to remove
 */
void watchers_remove_watcher(char *uri);

/**
 * Removes all watchers from the watcher list.
 * 
 */
void watchers_remove_all_watchers();

/**
 * Displays all watchers in the watchers tab.
 * 
 */
void watchers_display_watchers();

/**
 * Get the Dialog ID of our outgoing presence.winfo subscription
 * 
 */
int watchers_process_subscription_answered(eXosip_event_t *je);


/** @} */

#endif
