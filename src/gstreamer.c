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

#include "includes.h"
#include "gstreamer.h"
#include <fcntl.h>



/* Create our own video window - Glade adds widgets that block the video */
/*
GtkWidget*
create_videoWin (void)
{
  GtkWidget *videoWin;

  videoWin = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_events (videoWin, GDK_BUTTON_PRESS_MASK);
  gtk_window_set_title (GTK_WINDOW (videoWin), "IPTV Display");

  g_signal_connect ((gpointer) videoWin, "delete_event",
                    G_CALLBACK (on_videoWin_delete_event),
                    NULL);
  g_signal_connect ((gpointer) videoWin, "key_press_event",
                    G_CALLBACK (on_videoWin_key_press_event),
                    NULL);
  g_signal_connect ((gpointer) videoWin, "button_press_event",
                    G_CALLBACK (on_videoWin_button_press_event),
                    NULL);

  return videoWin;
}
*/

/* Callback to be called when the screen-widget is exposed */
static gboolean expose_cb(GtkWidget * widget, GdkEventExpose * event, gpointer data)
{

	/* Tell the xvimagesink/ximagesink the x-window-id of the screen
	 * widget in which the video is shown. After this the video
	 * is shown in the correct widget */
	// gst_x_overlay_set_xwindow_id(GST_X_OVERLAY(data), GDK_WINDOW_XWINDOW(widget->window));

	// printf("window exposed !\n");

	gst_x_overlay_expose (GST_X_OVERLAY(data));
	return FALSE;

}

static gboolean gstreamer_bus_callback(GstBus *bus, GstMessage *message, GstElement *pipeline)
{
	gchar *message_str;
	const gchar *message_name;
	GError *error;
    gchar*  pipeline_name;

    pipeline_name = "Non pipeline source";
    if (pipeline)
    {
        pipeline_name = gst_element_get_name(pipeline);
    }

    /* Report errors to the console */
	if(GST_MESSAGE_TYPE(message) == GST_MESSAGE_ERROR)
	{
		gst_message_parse_error(message, &error, &message_str);
		g_warning("GStreamer error on pipeline %s : %s\n",pipeline_name,message_str);
		g_error_free(error);
		g_free(message_str);
	}

	/* Report warnings to the console */
	if(GST_MESSAGE_TYPE(message) == GST_MESSAGE_WARNING)
	{
		gst_message_parse_warning(message, &error, &message_str);
		g_warning("GStreamer warning on pipeline %s : %s\n",pipeline_name,message_str);
		g_error_free(error);
		g_free(message_str);
	}

	if(GST_MESSAGE_TYPE(message) == GST_MESSAGE_EOS)
	{
	    if (strcmp("ringing_pipeline",pipeline_name) == 0)
	    {
            /* Pause for a second then rewind the ringing tone */
            sleep(1);
            if (!gst_element_seek (pipeline, 1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
                             GST_SEEK_TYPE_SET, 0,
                             GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE))
                    g_print ("Seek failed!\n");
	    }
	}
	return TRUE;
}

static void new_pad (GstElement *element, GstPad *pad, gpointer data)
{
	/* Simple function that links the new pad to the sink of the element
	   passed in */
	GstPad *sinkpad;
	sinkpad = gst_element_get_pad (data, "sink");
	gst_pad_link (pad, sinkpad);
	gst_object_unref (sinkpad);
}


int initialiseRingingPipeline(Call *call)
{
	char *ringing_tone = "media/ctu24ringtone.mp3";

	GstElement *pipeline, *file_source, *decoder_bin, *alsa_sink, *audio_converter, *audio_resampler;
	GstBus *bus;
	GstCaps *caps;

	pipeline = gst_pipeline_new("ringing_pipeline");
	bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
	gst_bus_add_watch(bus, (GstBusFunc)gstreamer_bus_callback, pipeline);
	gst_object_unref(GST_OBJECT(bus));

	call->ringingPipeline = pipeline;

	file_source = gst_element_factory_make("filesrc", "file_source");
	if (!file_source)
	{
	    g_critical("Couldn't create the file source element - missing the filesrc gstreamer element?");
	    return FALSE;
	}

	g_object_set (G_OBJECT (file_source), "location", imsua_addpath(ringing_tone), NULL);
	decoder_bin = gst_element_factory_make("decodebin", "decoder_bin");
	alsa_sink = gst_element_factory_make("alsasink", "alsa_sink");

    /* Check that elements are correctly initialized */
	if(!(pipeline && file_source && decoder_bin && alsa_sink))
	{
		g_critical("Couldn't create elements for the ringing pipeline. You are most likely missing some gstreamer plugins.");
		return FALSE;
	}

	g_signal_connect(decoder_bin, "pad-added", G_CALLBACK (new_pad), alsa_sink);
	gst_bin_add_many(GST_BIN(pipeline), file_source, decoder_bin, alsa_sink, NULL);

	if(!gst_element_link(file_source, decoder_bin))
	{
		g_critical("Problem linking the file source to the decode bin for the ringing pipeline\n");
		return FALSE;
	}

	gst_element_set_state(pipeline, GST_STATE_PLAYING);

}



int initialiseBackgroundVideoPipeline()
{

	char *bg_video = "rings/movie.wmv";

	GstElement *pipeline, *file_src, *decode_bin, *csp_filter, *video_scale, *screen_sink;
	GstCaps *caps;
	GstBus *bus;

	pipeline = gst_pipeline_new("background_video_pipeline");
	bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
	gst_bus_add_watch(bus, (GstBusFunc)gstreamer_bus_callback, pipeline);
	gst_object_unref(GST_OBJECT(bus));

	backgroundVideoPipeline = pipeline;

	file_src = gst_element_factory_make("filesrc", "file_src");
    if (!file_src)
	{
	    g_critical("Couldn't create the file source element - missing the filesrc gstreamer element?");
	    return FALSE;
	}

	g_object_set (G_OBJECT (file_src), "location", imsua_addpath(bg_video), NULL);
	decode_bin = gst_element_factory_make("decodebin", "decode_bin");
	csp_filter = gst_element_factory_make("ffmpegcolorspace", "csp_filter");
	video_scale = gst_element_factory_make("videoscale", "video_scaler");
	screen_sink = gst_element_factory_make("ximagesink", "screen_sink");

    if(!(pipeline && file_src && decode_bin && csp_filter && video_scale && screen_sink))
	{
		g_critical("Couldn't create pipeline elements for the background video. You are most likely missing some gstreamer plugins.");
		return FALSE;
	}

	g_signal_connect(decode_bin, "pad-added", G_CALLBACK (new_pad), csp_filter);
	gst_bin_add_many(GST_BIN(pipeline), file_src, decode_bin, csp_filter, video_scale, screen_sink, NULL);

	if(!gst_element_link(file_src, decode_bin))
	{
		g_critical("Problem linking elements for the background video pipeline\n");
		return FALSE;
	}

	if(!gst_element_link(csp_filter, video_scale))
	{
		g_critical("Problem linking elements for the background video pipeline\n");
		return FALSE;
	}

	caps = gst_caps_new_simple("video/x-raw-rgb",
			"width", G_TYPE_INT, 384,
			"height", G_TYPE_INT, 288,
			NULL);

	if(!gst_element_link_filtered(video_scale, screen_sink, caps))
	{
	    g_critical("Problem linking elements for the background video pipeline\n");
		return FALSE;
	}
	gst_caps_unref(caps);

	// GtkWidget *local_cam_widget = client->local_cam;
	// local_cam_widget = lookup_widget(GTK_WIDGET(imsUA), "local_cam");

	GdkWindow *window = gtk_widget_get_window (client->local_cam);
	gulong window_xid = GDK_WINDOW_XID (window);

	/* As soon as screen is exposed, window ID will be advised to the sink */
	// g_signal_connect(local_cam_widget, "expose-event", G_CALLBACK(expose_cb), screen_sink);
	// gst_x_overlay_set_xwindow_id(GST_X_OVERLAY(screen_sink), window_xid);

	gst_element_set_state(pipeline, GST_STATE_PLAYING);
}

int test_for_webcam_v4l(int version)
{
    // Test for compatabile camera
    GstElement *webcam_test_pipeline,*fakesink, *camera_src;
    GError *webcam_test_err;
    GstStateChangeReturn webcam_test_ret;
    GstMessage *webcam_test_msg;
    GstBus *webcam_test_bus;

    webcam_test_err = NULL;

    webcam_test_pipeline = gst_pipeline_new ("test_webcam");
    if (version == 1)
    {
        camera_src = gst_element_factory_make("v4lsrc", "camera_src");
    }
    else if (version == 2)
    {
        camera_src = gst_element_factory_make("v4l2src", "camera_src");
    }
    fakesink = gst_element_factory_make("fakesink", "fakesink");

    if (!(camera_src &&fakesink))
    {
        g_critical("Couldn't create pipeline elements for testing existence of webcam. You are most likely missing some gstreamer plugins.");
        return FALSE;
    }
    gst_bin_add_many(GST_BIN(webcam_test_pipeline), camera_src, fakesink,NULL);
    gst_element_link_many(camera_src,fakesink, NULL);

    /* Start the pipeline and wait for max. 1 second for it to start up */
    gst_element_set_state (webcam_test_pipeline, GST_STATE_PLAYING);
    webcam_test_ret = gst_element_get_state (webcam_test_pipeline, NULL, NULL, 1 * GST_SECOND);

   /* Check if any error messages were posted on the bus */
    webcam_test_bus = gst_element_get_bus (webcam_test_pipeline);
    webcam_test_msg = gst_bus_poll (webcam_test_bus, GST_MESSAGE_ERROR, 0);
    gst_object_unref (webcam_test_bus);
    gst_element_set_state (webcam_test_pipeline, GST_STATE_NULL);
    gst_object_unref (webcam_test_pipeline);

    if ((webcam_test_msg == NULL) && (webcam_test_ret == GST_STATE_CHANGE_SUCCESS))
    {
       	//printf("Finished looking for webcam, going to use Video for Linux version %d\n",version);
       return 0;
    }
    printf("Finished looking for webcam, didn't find Video for Linux version %d\n",version);
    return -1;
}

int initialiseVideoTxPipeline(Call *ca)
{
	// printf("Starting video TX pipeline\n");

	Preferences *pref = client->pref;

	GstElement *pipeline, *screen_sink,*camera_src, *videorate;
	GstElement *csp_filter, *csp_filter2, *tee;
	GstElement *h263_encoder, *rtp_h263_payloader, *udp_sink, *udp_queue, *screen_queue;
	GstCaps *caps;
	GstBus *bus;

	/* Create pipeline and attach a callback to it's
	 * message bus */
	pipeline = gst_pipeline_new("video_transmit_pipeline");
	bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
	gst_bus_add_watch(bus, (GstBusFunc)gstreamer_bus_callback, pipeline);
	gst_object_unref(GST_OBJECT(bus));

	ca->videoTxPipeline = pipeline;

	/* Create elements */
	/* Camera video stream comes from a Video4Linux driver */

    /* Creates the file lock for exclusive access to webcam */
    const char *lock_file = "/tmp/uct_ims_webcam.lck";
    int file_desc;
    struct flock region;
    int res;

    file_desc = open(lock_file, O_RDWR | O_CREAT,00777);
    if (!file_desc)
    {
        g_warning("Error: Unable to get file descriptor from webcam lock file\n");
    }

    region.l_type = F_WRLCK;
    region.l_whence = SEEK_SET;
    region.l_start = 0;
    region.l_len = 0;

    res = fcntl(file_desc,F_SETLK,&region);

	if (res !=-1)
    	{   
		printf("Set file lock\n");

		if (test_for_webcam_v4l(2)==0)
		{
		    printf("Setting Video for Linux version 2\n");
		    camera_src = gst_element_factory_make("v4l2src","camera_src");
		}
		else if (test_for_webcam_v4l(1)==0)
		{
		    printf("Setting Video for Linux version 1\n");
		    camera_src = gst_element_factory_make("v4lsrc","camera_src");
		}
    	}
	else
	{   printf("Camera source in use by another UCT IMS Client instance\n");
	    printf("Setting Video Test Pattern\n");

		return FALSE;
	    camera_src = gst_element_factory_make("videotestsrc","camera_src");
	}


	videorate = gst_element_factory_make("videorate","videorate");

	/* Colorspace filter is needed to make sure that sinks understands
	 * the stream coming from the camera */
	csp_filter = gst_element_factory_make("ffmpegcolorspace", "csp_filter");
	csp_filter2 = gst_element_factory_make("ffmpegcolorspace", "csp_filter2");

	/* Tee that copies the stream to multiple outputs */
	tee = gst_element_factory_make("tee", "tee");

	/* Queue creates new thread for the stream */
	screen_queue = gst_element_factory_make("queue", "screen_queue");
	udp_queue = gst_element_factory_make("queue", "udp_queue");
	screen_sink = gst_element_factory_make("xvimagesink", "screen_sink");
	//screen_sink = gst_element_factory_make("autovideosink", "screen_sink");

	// h263_encoder = gst_element_factory_make("ffenc_h263p", "h233_encoder");

	// h263_encoder = gst_element_factory_make("ffenc_mpeg4", "h233_encoder");
	// rtp_h263_payloader = gst_element_factory_make("rtpmp4vpay", "rtp_payloader");

	rtp_h263_payloader = gst_element_factory_make("rtpvp8pay", "h263_payloader");
	h263_encoder = gst_element_factory_make("vp8enc", "h263_encoder");
	// rtp_h263_payloader = gst_element_factory_make("rtph263ppay", "rtp_payloader");

	/* Set this paramter to change the quality (and bandwidth required) of the video stream */
	// g_object_set (G_OBJECT (h263_encoder), "bitrate", (200000 + pref->video_bw * 100000), NULL);

	//rtp_h263_payloader = gst_element_factory_make("rtph263ppay", "rtp_payloader");
	udp_sink = gst_element_factory_make("udpsink", "udp_sink");

	g_object_set (G_OBJECT (udp_sink), "host", ca->remote_video_ip, NULL);
	g_object_set (G_OBJECT (udp_sink), "port", ca->remote_video_port, NULL);


	/* Check that elements are correctly initialized */
	if(!(pipeline && camera_src && videorate && csp_filter && tee && screen_sink && h263_encoder && rtp_h263_payloader && udp_sink && screen_queue && udp_queue))
	{
		g_critical("Couldn't create pipeline elements for the video transmitting pipeline. You are most likely missing some gstreamer plugins.");
		return FALSE;
	}

	/* Add elements to the pipeline. This has to be done prior to
	 * linking them */
	//gst_bin_add_many(GST_BIN(pipeline), camera_src, tee, csp_filter, udp_queue, h263_encoder, rtp_h263_payloader, udp_sink, screen_sink, NULL);

	// gst_bin_add_many(GST_BIN(pipeline), camera_src, tee, csp_filter, csp_filter2, screen_sink, udp_queue, h263_encoder, rtp_h263_payloader, udp_sink, NULL);

	gst_bin_add_many(GST_BIN(pipeline), camera_src, tee, csp_filter, screen_sink, h263_encoder, rtp_h263_payloader, udp_sink, udp_queue, screen_queue, videorate, NULL);


	/* Specify what kind of video is wanted from the camera */
	// caps = gst_caps_new_simple("video/x-raw-rgb", "width", G_TYPE_INT, 320, "height", G_TYPE_INT, 240, NULL);
	caps = gst_caps_from_string("video/x-raw-yuv, width=320, height=240, framerate=(fraction)15/1");

	/* Link the camera source and colorspace filter using capabilities
	 * specified */

	if(!gst_element_link_many(camera_src, tee, screen_queue, csp_filter, screen_sink, NULL))
	{
		g_warning("Problem linking elements for the video transmitting preview section1\n");
		return FALSE;
	}


	if(!gst_element_link_many(tee, udp_queue, videorate, NULL))
	{
		g_warning("Problem linking elements for the video transmitting sending section4\n");
		return FALSE;
	}
	
	if(!gst_element_link_filtered(videorate, h263_encoder, caps))
	{
		g_warning("Problem linking elements for the video transmitting sending section4\n");
		return FALSE;
	}

	if(!gst_element_link_many(h263_encoder, rtp_h263_payloader, udp_sink, NULL))
	{
		g_warning("Problem linking elements for the video transmitting sending section4\n");
		return FALSE;
	}
	
	gst_x_overlay_set_window_handle(GST_X_OVERLAY(screen_sink), GDK_WINDOW_XID (gtk_widget_get_window (client->local_cam)));

	gst_element_set_state(pipeline, GST_STATE_PLAYING);

	return TRUE;
}



int testGstreamer()
{
	// printf("Starting video TX pipeline\n");

	Preferences *pref = client->pref;

	GstElement *pipeline, *screen_sink,*camera_src, *videoscale, *videorate;
	GstElement *csp_filter, *csp_filter2, *tee;
	GstElement *h263_encoder, *rtp_h263_payloader, *udp_sink, *udp_queue, *screen_queue, *src_queue;
	GstCaps *caps, *caps2;
	GstBus *bus;

	/* Create pipeline and attach a callback to it's
	 * message bus */
	pipeline = gst_pipeline_new("video_transmit_pipeline");
	bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
	gst_bus_add_watch(bus, (GstBusFunc)gstreamer_bus_callback, pipeline);
	gst_object_unref(GST_OBJECT(bus));

	//ca->videoTxPipeline = pipeline;

	/* Create elements */
	/* Camera video stream comes from a Video4Linux driver */

    /* Creates the file lock for exclusive access to webcam */
    const char *lock_file = "/tmp/uct_ims_webcam.lck";
    int file_desc;
    struct flock region;
    int res;

	camera_src = gst_element_factory_make("v4l2src","camera_src");
		
	videoscale = gst_element_factory_make("videoscale","videoscale");
	videorate = gst_element_factory_make("videorate","videorate");

	/* Colorspace filter is needed to make sure that sinks understands
	 * the stream coming from the camera */
	csp_filter = gst_element_factory_make("ffmpegcolorspace", "csp_filter");
	csp_filter2 = gst_element_factory_make("ffmpegcolorspace", "csp_filter2");

	/* Tee that copies the stream to multiple outputs */
	tee = gst_element_factory_make("tee", "tee");

	/* Queue creates new thread for the stream */
	screen_queue = gst_element_factory_make("queue", "screen_queue");
	udp_queue = gst_element_factory_make("queue2", "udp_queue");
	src_queue = gst_element_factory_make("queue2", "src_queue");
	screen_sink = gst_element_factory_make("xvimagesink", "screen_sink");
	//screen_sink = gst_element_factory_make("autovideosink", "screen_sink");

	// h263_encoder = gst_element_factory_make("ffenc_mpeg4", "h233_encoder");
	// h263_encoder = gst_element_factory_make("ffenc_h263p", "h233_encoder");
	// h263_encoder = gst_element_factory_make("ffenc_h263", "h233_encoder");
	// rtp_h263_payloader = gst_element_factory_make("rtph263ppay", "rtp_payloader");

	h263_encoder = gst_element_factory_make("vp8enc", "h233_encoder");
	rtp_h263_payloader = gst_element_factory_make("rtpvp8pay", "rtp_payloader");

	/* Set this paramter to change the quality (and bandwidth required) of the video stream */
	// g_object_set (G_OBJECT (h263_encoder), "bitrate", (200000 + pref->video_bw * 100000), NULL);

	// g_object_set (G_OBJECT (h263_encoder), "bitrate", 102400, NULL);

	// rtp_h263_payloader = gst_element_factory_make("rtpmp4vpay", "rtp_payloader");
	// rtp_h263_payloader = gst_element_factory_make("rtph263ppay", "rtp_payloader");

	udp_sink = gst_element_factory_make("udpsink", "udp_sink");

	g_object_set (G_OBJECT (udp_sink), "host", "127.0.0.1", NULL);
	g_object_set (G_OBJECT (udp_sink), "port", 10013, NULL);


	/* Check that elements are correctly initialized */
	if(!(pipeline && camera_src && videorate && csp_filter && tee && screen_sink && h263_encoder && rtp_h263_payloader && udp_sink && screen_queue && udp_queue))
	{
		g_critical("Couldn't create pipeline elements for the video transmitting pipeline. You are most likely missing some gstreamer plugins.");
		return FALSE;
	}

	/* Add elements to the pipeline. This has to be done prior to
	 * linking them */
	// gst_bin_add_many(GST_BIN(pipeline), camera_src, videorate, tee, csp_filter2, csp_filter, udp_queue, h263_encoder, rtp_h263_payloader, udp_sink, screen_queue, screen_sink, NULL);

	// gst_bin_add_many(GST_BIN(pipeline), camera_src, tee, csp_filter, csp_filter2, screen_sink, h263_encoder, rtp_h263_payloader, udp_sink, screen_queue, udp_queue, NULL);

	gst_bin_add_many(GST_BIN(pipeline), camera_src, tee, csp_filter, screen_queue, screen_sink, h263_encoder, rtp_h263_payloader, udp_queue, udp_sink, videorate, NULL);

	/* Specify what kind of video is wanted from the camera */
	// caps = gst_caps_new_simple("video/x-raw-yuv", "width", G_TYPE_INT, 640, "height", G_TYPE_INT, 480, NULL);
	// caps2 = gst_caps_new_simple("video/x-raw-yuv", "width", G_TYPE_INT, 320, "height", G_TYPE_INT, 240, NULL);
     	caps = gst_caps_from_string("video/x-raw-yuv, width=320, height=240, framerate=(fraction)15/1");

	if(!gst_element_link_many(camera_src, tee, screen_queue, csp_filter, screen_sink, NULL))
	{
		g_warning("Problem linking elements for the video transmitting preview section1\n");
		return FALSE;

	}

	if(!gst_element_link_many(tee, udp_queue, videorate, NULL))
	{
		g_warning("Problem linking elements for the video transmitting sending section4\n");
		return FALSE;
	}
	
	if(!gst_element_link_filtered(videorate, h263_encoder, caps))
	{
		g_warning("Problem linking elements for the video transmitting sending section4\n");
		return FALSE;
	}

	if(!gst_element_link_many(h263_encoder, rtp_h263_payloader, udp_sink, NULL))
	{
		g_warning("Problem linking elements for the video transmitting sending section4\n");
		return FALSE;
	}


	gst_caps_unref(caps);

	gst_x_overlay_set_window_handle(GST_X_OVERLAY(screen_sink), GDK_WINDOW_XID (gtk_widget_get_window (client->remote_cam)));
	
// gst_x_overlay_set_window_handle(GST_X_OVERLAY(screen_sink), 0);

	printf("Before playing\n");
  	// gst_element_set_state (pipeline, GST_STATE_PAUSED);
	gst_element_set_state(pipeline, GST_STATE_PLAYING);

	printf("After playing\n");

	return TRUE;
}

int initialiseVideoRxPipeline(Call *ca)
{

	// printf("Starting video RX pipeline\n");

	GstElement *pipeline, *udp_src, *rtp_h263_depayloader, *h263_decoder, *csp_filter, *screen_sink;
	GstCaps *caps;
	GstBus *bus;

	/* Create pipeline and attach a callback to it's
	 * message bus */
	pipeline = gst_pipeline_new("video_receive_pipeline");
	bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
	gst_bus_add_watch(bus, (GstBusFunc)gstreamer_bus_callback, pipeline);
	gst_object_unref(GST_OBJECT(bus));

	ca->videoRxPipeline = pipeline;

	printf("Ready to accept video on port %d\n", ca->local_video_port);

	/* Create elements */
	udp_src = gst_element_factory_make("udpsrc", "udp_src");
	g_object_set (G_OBJECT (udp_src), "port", (int)ca->local_video_port, NULL);
	//rtp_h263_depayloader = gst_element_factory_make("rtph263pdepay", "h263_deplayloader");

	// rtp_h263_depayloader = gst_element_factory_make("rtph263depay", "h263_deplayloader");
	// h263_decoder = gst_element_factory_make("ffdec_h263", "h263_decoder");

	rtp_h263_depayloader = gst_element_factory_make("rtpvp8depay", "h263_deplayloader");
	h263_decoder = gst_element_factory_make("vp8dec", "h263_decoder");

	csp_filter = gst_element_factory_make("ffmpegcolorspace", "csp_filter");
	screen_sink = gst_element_factory_make("ximagesink", "screen_sink");
	// g_object_set (G_OBJECT (screen_sink), "sync", FALSE, NULL);

	/* Check that elements are correctly initialized */
	if(!(pipeline && udp_src && rtp_h263_depayloader && h263_decoder && csp_filter && screen_sink))
	{
		g_critical("Couldn't create pipeline elements for the video receiving pipeline. You are most likely missing some gstreamer plugins.");
		return FALSE;
	}

	/* Add elements to the pipeline. This has to be done prior to
	 * linking them */
	gst_bin_add_many(GST_BIN(pipeline), udp_src, rtp_h263_depayloader, h263_decoder, csp_filter, screen_sink, NULL);

	/* Specify the RTP stream that we expect */	


	caps = gst_caps_new_simple("application/x-rtp",
			"media", G_TYPE_STRING, "video",
			"clock-rate", G_TYPE_INT, 90000,
			"payload", G_TYPE_INT, 96,
			"encoding-name", G_TYPE_STRING, "VP8-DRAFT-IETF-01",
			NULL);


	// caps = gst_caps_new_simple("application/x-rtp", NULL);

	/* Link the UDP source with the RTP decoder */
	if(!gst_element_link_filtered(udp_src, rtp_h263_depayloader, caps))
	{
		g_critical("Can't link elements for the video receiving pipeline. 1\n");
		return FALSE;
	}
	gst_caps_unref(caps);

	if(!gst_element_link_many(rtp_h263_depayloader, h263_decoder, csp_filter, screen_sink, NULL))
	{
		g_critical("Can't link elements for the video receiving pipeline. 2\n");
		return FALSE;
	}


	/* Overlay the video sink onto the remote_cam widget from the GUI */
	gst_x_overlay_set_window_handle(GST_X_OVERLAY(screen_sink), GDK_WINDOW_XID (gtk_widget_get_window (client->remote_cam)));

	gst_element_set_state(pipeline, GST_STATE_PLAYING);

	return TRUE;

}

int initialiseAudioTxPipeline(Call *ca)
{
	// g_print("Entering AudioTxPipeline Function\n");
	GstElement *pipeline, *alsa_src, *audio_convert, *audio_resample, *encoder, *payloader, *udp_sink;
	GstCaps *caps;
	GstBus *bus;
	gchar *encoder_name, *payloader_name;

	if (ca->audio_codec == 0)
	{
		encoder_name="mulawenc";
		payloader_name="rtppcmupay";
	}
	else if (ca->audio_codec == 8)
	{
		encoder_name="alawenc";
		payloader_name="rtppcmapay";
	}
	else if (ca->audio_codec == 3)
	{
		encoder_name="gsmenc";
		payloader_name="rtpgsmpay";
	}
	else if (ca->audio_codec == 14)
	{
		encoder_name="ffenc_mp2";
		payloader_name="rtpmpapay";
	}

	pipeline = gst_pipeline_new("audio_transmit_pipeline");
	bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
	gst_bus_add_watch(bus, (GstBusFunc)gstreamer_bus_callback, NULL);
	gst_object_unref(GST_OBJECT(bus));
	ca->audioTxPipeline = pipeline;

	alsa_src = gst_element_factory_make("alsasrc", "alsa_src");
	audio_convert = gst_element_factory_make("audioconvert", "audio_convert");
	audio_resample = gst_element_factory_make("audioresample", "audio_resample");
	encoder = gst_element_factory_make(encoder_name,encoder_name);
	payloader = gst_element_factory_make(payloader_name,payloader_name);
	udp_sink = gst_element_factory_make("udpsink", "udp_sink");

	/* Check that elements are correctly initialized */
	if(!(pipeline && alsa_src && audio_convert && audio_resample && encoder && payloader && udp_sink))
	{
		g_critical("Couldn't create pipeline elements for the transmitting audio pipeline. You are most likely missing some gstreamer plugins.");
		return FALSE;
	}

	g_object_set (G_OBJECT (udp_sink), "host", ca->remote_audio_ip, NULL);
	g_object_set (G_OBJECT (udp_sink), "port", ca->remote_audio_port, NULL);
	/* This line seems to break the audio on some systems */
	// g_object_set (G_OBJECT (udp_sink), "sync", FALSE, NULL);

	gst_bin_add_many(GST_BIN(pipeline), alsa_src,audio_convert,audio_resample,encoder, payloader, udp_sink, NULL);

    if(!gst_element_link_many(alsa_src, encoder, payloader, udp_sink, NULL))
	{
		g_critical("Can't link elements for the audio transmitting pipeline.\n");
		return FALSE;
	}

	gst_element_set_state(pipeline, GST_STATE_PLAYING);
	// g_print("Exiting AudioTxPipeline Function\n");
}


int initialiseAudioRxPipeline(Call *ca)
{
	// g_print("Entering AudioRxPipeline Function\n");
	GstElement *pipeline, *udp_src, *depayloader, *decoder, *alsa_sink;
	GstElement *audioresample, *audioconvert;
	GstCaps *caps;
	gchar *dp_name,*decoder_name;
	GstBus *bus;

	/* Create pipeline and attach a callback to it's
	 * message bus */
	pipeline = gst_pipeline_new("audio_receive_pipeline");
	bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
	gst_bus_add_watch(bus, (GstBusFunc)gstreamer_bus_callback, NULL);
	gst_object_unref(GST_OBJECT(bus));

	ca->audioRxPipeline = pipeline;

	if (ca->audio_codec == 0)
	{
		dp_name= "rtppcmudepay";
		decoder_name="mulawdec";
		caps = gst_caps_new_simple("application/x-rtp",
			"clock-rate", G_TYPE_INT, 8000,
			"payload", G_TYPE_INT, 0,
			NULL);
	}
	else if (ca->audio_codec == 8)
	{
		dp_name="rtppcmadepay";
		decoder_name="alawdec";
		caps = gst_caps_new_simple("application/x-rtp",
			"clock-rate", G_TYPE_INT, 8000,
			"payload", G_TYPE_INT, 8,
			NULL);
	}
	else if (ca->audio_codec == 3)
	{
		dp_name="rtpgsmdepay";
		decoder_name="gsmdec";
		caps = gst_caps_new_simple("application/x-rtp",
			"clock-rate", G_TYPE_INT, 8000,
			"payload", G_TYPE_INT, 3,
			NULL);
	}
	else if (ca->audio_codec == 14)
	{
		dp_name="rtpmpadepay";
		decoder_name="mad";
		caps = gst_caps_from_string("application/x-rtp,media=(string)audio, clock-rate=(int)90000, encoding-name=(string)MPA, payload=(int)96");
	}
	// g_print("About to create elements\n");
	// g_print("Depayloader is %s , Decoder is %s \n",dp_name,decoder_name);

	/* Create elements */
	udp_src = gst_element_factory_make("udpsrc", "udp_src");
	g_object_set (G_OBJECT (udp_src), "port", ca->local_audio_port, NULL);
	depayloader = gst_element_factory_make(dp_name,dp_name);
	decoder = gst_element_factory_make(decoder_name,decoder_name);
	alsa_sink = gst_element_factory_make("alsasink", "alsa_sink");
	audioconvert = gst_element_factory_make("audioconvert", "audio_convert");
	audioresample = gst_element_factory_make("audioresample", "audio_resample");

	/* Check that elements are correctly initialized */
	if(!(pipeline && udp_src && depayloader && decoder && audioconvert && audioresample &&  alsa_sink))
	{
		g_critical("Couldn't create pipeline elements for the audio receiving pipeline. You are most likely missing some gstreamer plugins.");
		return FALSE;
	}

	g_object_set (G_OBJECT (alsa_sink), "sync", FALSE, NULL);

	gst_bin_add_many(GST_BIN(pipeline), udp_src, depayloader, decoder,audioconvert,audioresample, alsa_sink, NULL);

	/* Link the UDP source with the RTP decoder */
	if(!gst_element_link_filtered(udp_src, depayloader, caps))
	{
		g_critical("Can't link elements for the audio receiving pipeline.\n");
		return FALSE;
	}
	gst_caps_unref(caps);

	if(!gst_element_link_many(depayloader, decoder,audioresample,audioconvert, alsa_sink, NULL))
	{
		g_critical("Can't link elements for the audio receiving pipeline.\n");
		return FALSE;
	}
	// g_print("About to start pipeline\n");
	gst_element_set_state(pipeline, GST_STATE_PLAYING);
	// g_print("Exiting AudioRxPipeline Function\n");
}

/* This method taken from Richard Spiers thesis on IPTV */
int initialiseIptvVideoPipeline(Call *ca)
{

	Preferences *pref = client->pref;
	// printf("Starting iptv pipeline\n");

	GstElement *pipeline, *udp_src, *rtp_h263_depayloader, *h263_decoder, *video_scaler, *csp_filter, *screen_sink;
	GstCaps *caps;
	GstBus *bus;

	/* Create pipeline and attach a callback to it's
	 * message bus */
	pipeline = gst_pipeline_new("IPTV_video_receive_pipeline");
	bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
	gst_bus_add_watch(bus, (GstBusFunc)gstreamer_bus_callback, pipeline);
	gst_object_unref(GST_OBJECT(bus));

	ca->iptvVideoPipeline = pipeline;

	/* Create elements */
	udp_src = gst_element_factory_make("udpsrc", "udp_src");
	rtp_h263_depayloader = gst_element_factory_make("rtph263pdepay", "h263_deplayloader");
	h263_decoder = gst_element_factory_make("ffdec_h263", "h263_decoder");
	video_scaler = gst_element_factory_make("videoscale", "videoscale");
	csp_filter = gst_element_factory_make("ffmpegcolorspace", "csp_filter");

	/* Some HW can't handle xvimagesink so give the option to use ximagesink instead */
	if (pref->iptv_hw_acceleration == 1)
		screen_sink = gst_element_factory_make("xvimagesink", "screen_sink");
	else
		screen_sink = gst_element_factory_make("ximagesink", "screen_sink");

    /* Check that elements are correctly initialized */
	if(!(pipeline && udp_src && rtp_h263_depayloader && h263_decoder && video_scaler && csp_filter && screen_sink))
	{
		g_critical("Couldn't create pipeline elements. You are most likely missing some gstreamer plugins.");
		return FALSE;
	}

    g_object_set (G_OBJECT (udp_src), "port", (int)ca->local_video_port, NULL);
	g_object_set (G_OBJECT (screen_sink), "sync", FALSE, NULL);
	g_object_set (G_OBJECT (screen_sink), "force-aspect-ratio", 1, NULL);



	/* Add elements to the pipeline. This has to be done prior to
	 * linking them */
	gst_bin_add_many(GST_BIN(pipeline), udp_src, rtp_h263_depayloader, h263_decoder, video_scaler, csp_filter, screen_sink, NULL);

	/* Specify the RTP stream that we expect */
	caps = gst_caps_new_simple("application/x-rtp",
			"clock-rate", G_TYPE_INT, 90000,
			"payload", G_TYPE_INT, 96,
			"encoding-name", G_TYPE_STRING, "H263-1998",
			NULL);

	/* Link the UDP source with the RTP decoder */
	if(!gst_element_link_filtered(udp_src, rtp_h263_depayloader, caps))
	{
		g_critical("Can't link elements for the iptv pipeline.\n");
		return FALSE;
	}


	if (pref->iptv_hw_acceleration == 1)
	{
		// printf("Starting IPtv Client with hardware acceleration\n");

		if(!gst_element_link_many(rtp_h263_depayloader, h263_decoder, csp_filter, NULL))
		{
			g_critical("Can't link elements for the iptv pipeline.\n");
			return FALSE;
		}
		caps = gst_caps_new_simple("video/x-raw-yuv",
			"format", GST_TYPE_FOURCC, GST_STR_FOURCC("YV12"),
			NULL);

		if(!gst_element_link_filtered(csp_filter, screen_sink, caps))
		{
			g_critical("Can't link elements for the iptv pipeline.\n");
			return FALSE;
		}

		gst_caps_unref(caps);

	}
	else
	{
		// printf("Starting IPtv Client without hardware acceleration.\n");

		if(!gst_element_link_many(rtp_h263_depayloader, h263_decoder, video_scaler, csp_filter, screen_sink, NULL))
		{
			g_critical("Can't link elements for the iptv pipeline.\n");
			return FALSE;
		}
	}


	/* Uncomment the next line if you want the video to stay op top - not recommended */
	// gtk_window_set_keep_above(GTK_WINDOW(videoWin), TRUE);
	gtk_window_resize(GTK_WINDOW(videoWin), 640, 480);
	gtk_widget_show_all(videoWin);

	gst_x_overlay_prepare_xwindow_id(GST_X_OVERLAY(screen_sink));

	GdkWindow *window = gtk_widget_get_window (videoWin);
	gulong window_xid = GDK_WINDOW_XID (window);

	/* Tell the x overlay in which window we want it to appear */
	gst_x_overlay_set_xwindow_id(GST_X_OVERLAY(screen_sink), window_xid);

	gst_element_set_state(pipeline, GST_STATE_PLAYING);

	return TRUE;
}

int destroyRingingPipeline(Call *ca)
{


	if((ca->ringingPipeline) && GST_IS_ELEMENT(ca->ringingPipeline))
	{
		gst_element_set_state(ca->ringingPipeline, GST_STATE_NULL);
		gst_object_unref(GST_OBJECT(ca->ringingPipeline));
		ca->ringingPipeline = NULL;
	}

	return TRUE;

}

int destroyBackgroundVideoPipeline()
{

	if(backgroundVideoPipeline && GST_IS_ELEMENT(backgroundVideoPipeline))
	{
		gst_element_set_state(backgroundVideoPipeline, GST_STATE_NULL);
		gst_object_unref(GST_OBJECT(backgroundVideoPipeline));
		backgroundVideoPipeline = NULL;
	}

	return TRUE;


}

int destroyVideoTxPipeline(Call *ca)
{

	if((ca->videoTxPipeline) && GST_IS_ELEMENT(ca->videoTxPipeline))
	{
		gst_element_set_state(ca->videoTxPipeline, GST_STATE_NULL);
		gst_object_unref(GST_OBJECT(ca->videoTxPipeline));
		ca->videoTxPipeline = NULL;
	}

	return TRUE;

}


int destroyVideoRxPipeline(Call *ca)
{

	if((ca->videoRxPipeline) && GST_IS_ELEMENT(ca->videoRxPipeline))
	{
		gst_element_set_state(ca->videoRxPipeline, GST_STATE_NULL);
		gst_object_unref(GST_OBJECT(ca->videoRxPipeline));
		ca->videoRxPipeline = NULL;
	}

	return TRUE;

}

int destroyAudioTxPipeline(Call *ca)
{

	if((ca->audioTxPipeline) && GST_IS_ELEMENT(ca->audioTxPipeline))
	{
		gst_element_set_state(ca->audioTxPipeline, GST_STATE_NULL);
		gst_object_unref(GST_OBJECT(ca->audioTxPipeline));
		ca->audioTxPipeline = NULL;
	}

	return TRUE;

}


int destroyAudioRxPipeline(Call *ca)
{

	if((ca->audioRxPipeline) && GST_IS_ELEMENT(ca->audioRxPipeline))
	{
		gst_element_set_state(ca->audioRxPipeline, GST_STATE_NULL);
		gst_object_unref(GST_OBJECT(ca->audioRxPipeline));
		ca->audioRxPipeline = NULL;
	}

	return TRUE;

}

int destroyIptvVideoPipeline(Call *ca)
{

	if((ca->iptvVideoPipeline) && GST_IS_ELEMENT(ca->iptvVideoPipeline))
	{
		gst_element_set_state(ca->iptvVideoPipeline, GST_STATE_NULL);
		gst_object_unref(GST_OBJECT(ca->iptvVideoPipeline));
		ca->iptvVideoPipeline = NULL;
	}

	return TRUE;

}
