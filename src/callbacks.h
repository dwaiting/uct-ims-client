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

#ifndef CALLBACKS_H
#define CALLBACKS_H


void on_save_clicked(GtkButton *button, gpointer user_data);
void on_register_button_clicked (GtkButton *button, gpointer user_data);
void on_deregister_button_clicked (GtkButton *button, gpointer user_data);

void on_call_button_clicked (GtkButton *button, gpointer user_data);
void on_hangup_button_clicked (GtkButton *button, gpointer user_data);

void on_register_as_alice_button_clicked (GtkButton *button, gpointer user_data);
void on_register_as_bob_button_clicked (GtkButton *button, gpointer user_data);

void on_call_alice_button_clicked (GtkButton *button, gpointer user_data);
void on_call_bob_button_clicked (GtkButton *button, gpointer user_data);

gboolean on_im_text_entry_key_press_event (GtkWidget *widget, GdkEventKey *event, gpointer user_data);
gboolean on_im_text_entry_key_release_event (GtkWidget *widget, GdkEventKey *event, gpointer user_data);

#endif
