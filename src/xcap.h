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

#ifndef XCAP_H
#define XCAP_H

/**
 * @defgroup uctimsclient_xcap XCAP
 * @ingroup uctimsclient_general
 * @{
 */

/**
 * Find a user id in a pres-rules document rule.
 * 
 * @param cur - search in this rule
 * @param user - the uri of the user to search for
 * @returns 1 if user found or 0 otherwise
 */
int xcap_find_user_in_rule(xmlNodePtr cur, char *user);

/**
 * Remove a user from a pres-rules document rule.
 * 
 * @param cur - search in this rule
 * @param user - the uri of the user to search for
 * @returns 1 if user found or 0 otherwise
 */
int xcap_remove_user_from_rule(xmlNodePtr cur, char *user);

/**
 * Add a user to a pres-rules document rule.
 * 
 * @param cur - in this rule
 * @param user - the uri of the user
 * @returns 1 if successful
 */
int xcap_add_user_to_rule(xmlNodePtr cur, char *user);

/**
 * Check if there are any users in the current rule.
 * 
 * @param cur - in this rule
 * @returns 1 if not empty or 0 otherwise
 */
int xcap_rule_is_not_empty(xmlNodePtr cur);

/**
 * Check if this is a block rule.
 * 
 * @param cur - the rule to check
 * @returns 1 if true or 0 otherwise
 */
int xcap_rule_is_block(xmlNode *cur);

/**
 * Check if this is a confirm rule.
 * 
 * @param cur - the rule to check
 * @returns 1 if true or 0 otherwise
 */
int xcap_rule_is_confirm(xmlNodePtr cur);

/**
 * Check if this is a polite-block rule.
 * 
 * @param cur - the rule to check
 * @returns 1 if true or 0 otherwise
 */
int xcap_rule_is_politeblock(xmlNodePtr cur);

/**
 * Check if this is an allow rule.
 * 
 * @param cur - the rule to check
 * @returns 1 if true or 0 otherwise
 */
int xcap_rule_is_allow(xmlNodePtr cur);

/**
 * Finds a buddy in the list of buddies.
 * 
 * @param buddy - the uri of the buddy to find
 * @returns the buddy struct if found or NULL if not found
 */
xmlNsPtr xcap_get_namespace(xmlDocPtr doc, xmlNodePtr cur, char *comparehref);

/**
 * Delete all empty rules from a pres-rules doc.
 * 
 * @param doc - the pres-rules document
 */
int xcap_delete_empty_rules(xmlDocPtr doc);

/**
 * Add a new rule to a pres-rules document.
 * 
 * @param doc - the pres-rules document
 * @param rulename - the name of the rule, i.e. pres_whitelist or pres_blacklist
 * @param action - the action of the rule, i.e. allow, block, confirm or polite-block
 * @param user - the first user id of the rule
 */
xmlDocPtr xcap_add_new_rule(xmlDocPtr doc, char *ruleName, char *action, char *user);

/**
 * Allow a user in our presence rules.
 * 
 * @param user - the uri of the user to add
 */
void xcap_allow_user(char *user);

/**
 * Block a user in our presence rules.
 * 
 * @param user - the uri of the user to block
 */
void xcap_block_user(char *user);

/**
 * Reset the presence rules document with no rules.
 * 
 */
void xcap_reset_presrules_doc();

/**
 * Pretty print our rules in the pres-rules tab.
 * 
 * @param cur - the root element of the pres-rules document
 */
void xcap_pretty_print_xml(xmlNodePtr cur);

/**
 * A curl callback for getting pres-rules.
 *
 */
size_t WriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data);

/**
 * Download our pres-rules from the XCAP server.
 * 
 */
void xcap_get_presrules_from_server();

/**
 * A curl callback for putting pres-rules.
 * 
 */
size_t read_callback(void *ptr, size_t size, size_t nmemb, void *stream);

/**
 * Upload our pres-rules to the XCAP server.
 * 
 */
void xcap_put_presrules_to_server();


/**
 * Open the window that allows XDMS downloads
 * 
 */
void xcap_open_xdms_download_window();

/**
 * Open the window that allows XDMS uploads
 * 
 */
void xcap_open_xdms_upload_window();

/**
 * Download generic XML doc from XCAP server
 * 
 */
void xcap_get_xml_from_server();

/**
 * Upload generic XML doc to XCAP server
 * 
 */
void xcap_put_xml_to_server();

/**
 * Download generic XML doc from XCAP address
 * 
 */
xmlDocPtr xcap_get_xml_from_xdms_address(char *full_request_url);

/**
 * Upload generic XML doc to XCAP address
 * 
 */
int xcap_put_xml_to_xdms_address(char *full_request_url, gchar *filename);

/**
 * Opens the file dialog browser
 * 
 */

void xdms_xml_file_dialog_open();

/**
 * Gets the xml file name from the dialog file chooser to upload to the XDMS server
 * 
 */

void xcap_get_xml_filename();

/** @} */

#endif
