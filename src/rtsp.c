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

/* 
 *This is an attempt at a very basic RTSP client - certainly does not comply with RFC 2326
 *
 */


#include "includes.h"
#include "rtsp.h"

void rtsp_start_session(char *rtsp_address)
{
	
	/**Need this to instantiate the vlc media control object*/
	 libvlc_exception_t excp;
	 libvlc_instance_t *inst;
	 const char *vlc_args[] = { "./uctimsclient" };
	 inst = libvlc_new (1, vlc_args, &excp);


	 /*check if XDMS upload window is open and if not open it*/
	if(vod_state != IDLE)
	{
		//already in an RTSP session
	}
 	else if(vod_window_open == 0)
 	{
 		vod_window = GTK_WINDOW(create_vod_window());
		GtkWidget *vod_out = lookup_widget(GTK_WIDGET(vod_window), "vod_out");
		gtk_drawing_area_size(GTK_DRAWING_AREA(vod_out),640 ,480);
 		gtk_widget_show (GTK_WIDGET(vod_window));
 		vod_window_open = 1;
		
	        mediacontrol_exception_init(media_excp);
		media_inst = mediacontrol_new_from_instance(inst, media_excp);
        	mediacontrol_set_mrl (media_inst,rtsp_address,media_excp);	
				
		/**setting the volume to the correct value*/
		int volume = 0;
		volume = mediacontrol_sound_get_volume(media_inst,media_excp);
		//normalize
		div_t divresult;
  		divresult = div (volume,10);
		int norm_volume = divresult.quot;	
		GtkWidget *volume_control_widget = lookup_widget(GTK_WIDGET(vod_window), "volume_control");	
		/**TO FIX A hack, not sure why entering in a variable read from VLC causes seg fault*/
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(volume_control_widget),25);	
		
		libvlc_video_set_parent(inst,GDK_WINDOW_XWINDOW(vod_out->window), &excp);
		libvlc_video_set_size(inst, 640, 480, &excp);
		
   		

		mediacontrol_Position media_pos;
		/**TO FIX A hack, not sure why param 2 works by pointing to non initialised space*/
   		mediacontrol_start(media_inst,&media_pos,media_excp);
		vod_state = PLAY;
	}
 	else /* If already open bring focus to it*/
 	{
		gtk_window_present(vod_window);
 	}
}

void rtsp_resume_playback()
{
	if(vod_state == PLAY)
	{
		//ignore
	}
	else if(vod_state == FORWARD)
	{
		mediacontrol_set_rate(media_inst,100,media_excp);
		vod_state = PLAY;
        }
	else if(vod_state == REWIND)
	{
		vod_state = PLAY;
	}
	else if(vod_state == PAUSE)
	{
		mediacontrol_resume(media_inst,media_excp);
		vod_state = PLAY;	
	}
	else if(vod_state == IDLE)
	{
		int pos = 0;
		/**A hack, not sure why param 2 works by pointing to non initialised space*/
   		mediacontrol_Position media_pos;
		mediacontrol_start(media_inst,&media_pos,media_excp);
		vod_state = PLAY;
	}
}

void rtsp_pause_playback()
{
	if(vod_state == PAUSE)
	{
		//ignore
	}
	else if(vod_state == FORWARD)
	{
		vod_state = PAUSE;
		mediacontrol_set_rate(media_inst,100,media_excp);
		mediacontrol_pause(media_inst,media_excp);
        }
	else if(vod_state == REWIND)
	{
		vod_state = PAUSE;
		mediacontrol_pause(media_inst,media_excp);
	}
	else if(vod_state == PLAY)
	{
		vod_state = PAUSE;
		mediacontrol_pause(media_inst,media_excp);	
	} 
}

void rtsp_stop_playback()
{
	if(vod_state == IDLE)
	{
		//ignore
	}
	else if(vod_state == FORWARD)
	{
		vod_state = IDLE;
		mediacontrol_set_rate(media_inst,100,media_excp);
		mediacontrol_stop(media_inst,media_excp);
        }
	else if(vod_state == REWIND)
	{
		vod_state = IDLE;
		mediacontrol_stop(media_inst,media_excp);
	}
	else if(vod_state == PLAY)
	{
		vod_state = IDLE;
		mediacontrol_stop(media_inst,media_excp);
	} 
	else if(vod_state == PAUSE)
	{
		vod_state = IDLE;
		mediacontrol_stop(media_inst,media_excp);
	}
}

void rtsp_rewind_playback()
{

// 	vod_state = REWIND;
// 	mediacontrol_Position *media_pos;
// 	media_pos = mediacontrol_get_media_position(media_inst,mediacontrol_ModuloPosition,mediacontrol_MediaTime,media_excp);
// 	mediacontrol_pause(media_inst,NULL,media_excp);
// 	usleep(1000000);
// 	while(vod_state == REWIND)
// 	{
// 		usleep(5000000);
// 		if(media_pos !=  NULL)
// 		{
// 			media_pos->value = media_pos->value - 1000;
// 			mediacontrol_set_media_position(media_inst,media_pos,media_excp);
// 		}
// 	}
// 	usleep(1000000);
// 	mediacontrol_resume(media_inst,NULL,media_excp);
}

void rtsp_forward_playback()
{
	vod_state = FORWARD;
	int current_rate = mediacontrol_get_rate(media_inst,media_excp);
	int new_rate = 25;
	//if the rate drops below 25% we have crashing problems
	if(current_rate > 25 )
	{
		div_t divresult;
		divresult = div (current_rate,2);
		new_rate = divresult.quot;
	}
	mediacontrol_set_rate(media_inst,new_rate,media_excp);
}

void rtsp_change_volume()
{
	GtkWidget *volume_control_widget = lookup_widget(GTK_WIDGET(vod_window), "volume_control");
	int new_vol;
	new_vol =  gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(volume_control_widget));
	/**change the volume*/
	new_vol = new_vol*10;
	mediacontrol_sound_set_volume(media_inst, new_vol, media_excp);
}

void rtsp_end_session()
{
	rtsp_stop_playback();
        //need to allow vlc video out to shut before window is destroyed or seg fault
	usleep(500000);
	gtk_widget_hide (GTK_WIDGET(vod_window));
 	vod_window_open = 0;
	vod_state = IDLE;
}
