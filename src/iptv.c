/*
  The UCT IMS Client
  Copyright (C) 2006 - University of Cape Town
  David Waiting <david@crg.ee.uct.ac.za>
  Richard Good <rgood@crg.ee.uct.ac.za>
  Robert Marston <rmarston@crg.ee.uct.ac.za> (this file)

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

#include "includes.h"
#include "iptv.h"

void iptv_invite_iptvAS(char *content_sip_url) {
    if (state == IDLE) {
        /* Set the URI bar to the channel address */
        gchar *uri;
        uri = g_strdup_printf("sip:%s@%s", content_sip_url, pref->iptv_server);
        GtkWidget *uri_entry_widget = lookup_widget(GTK_WIDGET(imsUA), "uri_entry");
        gtk_entry_set_text(GTK_ENTRY(uri_entry_widget), uri);

        ims_call_initiate();
    }
    else {
        set_display("Already in a call");
    }
}

void iptv_request_rtsp(char *content_rtsp_uri) {
        /* Open VLC and play rtsp stream */
        if (fork() == 0)/*fork so as not to overtake the current process*/
            execl("/usr/bin/vlc", "vlc", content_rtsp_uri, 0, NULL);
}
