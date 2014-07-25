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

char *add_sip_scheme(char *in);

void set_display(char *text);
void imsua_set_message_display(char *text, int color);
void imsua_append_text_to_display(char *text);
void imsua_display_event_info(eXosip_event_t *je);
void set_status_bar(char *text);
char *get_uuid(uuid_t u);
int imsua_is_associated_uri(char *uri);
char* add_lr_to_route(char *route);
char *add_quotes(char *in);
char *imsua_remove_quotes(char *text);
char *imsua_get_time();
int timeval_subtract (struct timeval *result, struct timeval *x, struct timeval *y);
char *imsua_addpath(char *filename);
int imsua_regex_match(char *string, char *pattern);

#endif
