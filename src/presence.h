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

#ifndef PRESENCE_H
#define PRESENCE_H

/**
 * @defgroup uctimsclient_presence Presence
 * @ingroup uctimsclient_general
 * @{
 */


/**
 * Upate presentity display and publish new presence information.
 * 
 */
void presence_change_presentity();


/**
 * Publish presentity.
 * 
 */
void presence_publish_presentity(char *basic, char *note);

/**
 * Get the <basic> and <note> elements from an XML document.
 * 
 * @param root_node - the root node of the XML document
 * @param entity - the URI of the presentity
 */
void presence_get_xml_elements(xmlNode* root_node, char *entity);

/**
 * Process an incoming presence notification.
 * 
 * @param je - the eXosip event
 */
void presence_process_notify(eXosip_event_t *je);

/**
 * Finds a buddy in the list of buddies.
 * 
 * @param buddy - the uri of the buddy to find
 * @returns the buddy struct if found or NULL if not found
 */
buddydata *presence_find_buddy(char *buddy);

/**
 * Add a buddy to the buddy list.
 * 
 * @param buddy - the uri of the buddy to add
 */
void presence_add_buddy(char *buddy);

/**
 * Remove a buddy from the buddy list.
 * 
 * @param buddy - the uri of the buddy to remove
 */
void presence_remove_buddy(char *buddy);

/**
 * Send an eXosip subscribe for a presentity.
 * 
 * @param buddy - the uri of the buddy to watch
 * @param expires - length that subscription is valid
 */
void presence_subscribe_to_presentity(char *buddy, int expires);

/**
 * Subscribe to the presentity of all on the buddy list.
 * @param expires - length that subscriptions are valid
 * 
 */
gint presence_subscribe_to_all_presentities(int expires);


/**
 * Displays a list of all buddies.
 * 
 */
void presence_display_buddies();

/**
 * Gets list of buddies from file.
 * 
 * @param filename - filename where the buddies are kept - default to .buddylist
 */
void presence_get_buddy_list_from_file(char *filename);

/**
 * Writes list of buddies to file.
 * 
 * @param filename - filename where the buddies are kept - default to .buddylist
 */
void presence_write_buddy_list_to_file(char *filename);


int presence_process_subscription_answered(eXosip_event_t *je);

/** @} */

#endif
