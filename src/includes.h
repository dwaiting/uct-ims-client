#ifndef INCLUDES_H
#define INCLUDES_H

#include <signal.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <eXosip2/eXosip.h>
#include <gtk/gtk.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <curl/curl.h>
#include <regex.h>
#include <unistd.h>

#include <unistd.h>

#include <glib.h>
#include <string.h>

#include <gst/gst.h>
#include <gdk/gdkx.h>
#include <gst/video/videooverlay.h>

#include <time.h>

//to extract linux ip address
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <arpa/inet.h>

//to use libvlc
// #include <vlc/vlc.h>
// #include <vlc/mediacontrol.h>
// #include <vlc/mediacontrol_structures.h>

#include "msrp.h"
#include "msrp_session.h"


typedef struct
{
	char name[50];
	char impu[50];
	char impi[50];
	char pcscf[50];
	char realm[50];
	char password[50];
	
	int presence_enabled;
	int video_enabled;
	int session_im_enabled;
	int qos_strength;
	int qos_type;
	int access_network;
	int audio_codec_primary;
	int audio_codec_secondary;
	int video_codec_primary;
	int dtmf;
	int media_interface;
	int video_bw;

	char iptv_server[50];
	int iptv_hw_acceleration;

	char local_audio_ip[128];
	int local_audio_port;
	char local_video_ip[128];
	int local_video_port;
	
	char local_msrp_ip[128];
	int local_msrp_port;

	char xdms_root_url[50];
	char xdms_username[50];
	char xdms_password[50];

} Preferences;



typedef struct
{
        GtkWidget *mainwindow;
	GtkWidget *uri_entry;
	GtkWidget *local_cam;
	GtkWidget *remote_cam;
        GtkWidget *registration_status_label;
	GtkWidget *call_status_label;
        GtkWidget *log_view;

	GtkWidget *im_text_entry;
	GtkWidget *im_text_view;
	GtkWidget *im_notebook;
	GtkWidget *im_tab_label;

	GtkWidget *general_name_entry;
	GtkWidget *general_impu_entry;	
	GtkWidget *general_impi_entry;
	GtkWidget *general_pcscf_entry;
	GtkWidget *general_realm_entry;
	GtkWidget *general_password_entry;

	GtkWidget *media_primaryvoicecodec_combobox;
	GtkWidget *media_secondaryvoicecodec_combobox;
	GtkWidget *media_mediainterface_combobox;
	GtkWidget *media_videocalling_combobox;
	GtkWidget *media_videoquality_combobox;

	char im_buffer[250];
	MsrpEndpoint *msrp_endpoint;

	Preferences *pref;
	
} ClientUI;


typedef struct
{
	char to_uri[50];
	char from_name[50];
	char from_uri[50];
	char remote_audio_ip[50];
	int remote_audio_port;
	char remote_video_ip[50];
	int remote_video_port;
	char local_ip[128];
	int local_audio_port;
	int local_video_port;
	int audio_codec;
	GstElement *audioRxPipeline;
	GstElement *audioTxPipeline;
	GstElement *videoRxPipeline;
	GstElement *videoTxPipeline;
	GstElement *ringingPipeline;
	GstElement *backgroundVideoPipeline;
	GstElement *iptvVideoPipeline;
	char file_name[100];
	char p_access_network[50];
	int cid;
	int tid;
	int did;
	int call_is_active;
	int call_number;
	int qos_local_state;
	int qos_remote_state;
	int qos_remote_preference;
	int qos_confirm;
	int sent_update;
	int caller;
	osip_message_t *most_recent_message;
	osip_message_t *prov_resp;
	int audio_supported;
	int video_supported;
	int im_supported;
	int media_negotiated;
	int content_indirected;
	char content_indirection_uri[200];

	MsrpEndpoint *msrp_endpoint;

        int remote_msrp_port;
	char *remote_msrp_path;
} Call;


typedef struct
{
	char *s;
	int len;
} str;


typedef struct
{
	char buddy[50];
	char basic[20];
	char note[500];
	int subscription_did;
} buddydata;


typedef struct
{
	char watcher[50];
	char status[50];
} watcherdata;


typedef unsigned char u8;

#define DEBUG 0

#define IDLE 0
#define LOCAL_RINGING 1
#define REMOTE_RINGING 2
#define IN_CALL 3
#define MOD_SESSION 4

#define IMS_MODE 0
#define SIP_MODE 1

#define QOS_NONE 0
#define QOS_OPTIONAL 1
#define QOS_MANDATORY 2

#define QOS_SEGMENTED 0
#define QOS_E2E 1

#define NONE 0
#define SEND 1
#define RECV 2
#define SENDRECV 3

#define NOT_REGISTERED 0
#define REGISTERED 1

#define NOT_VISIBLE 0
#define VISIBLE 1

#define PLAY 7
#define PAUSE 8
#define FORWARD 9
#define REWIND 10

#define REG_EXPIRE 600000
#define PRESENCE_EXPIRE 3600

#define PRES_RULES_DOC "presence_rules.xml"

#define GLADE_HOOKUP_OBJECT(component,widget,name) \
  g_object_set_data_full (G_OBJECT (component), name, \
    gtk_widget_ref (widget), (GDestroyNotify) gtk_widget_unref)

#define GLADE_HOOKUP_OBJECT_NO_REF(component,widget,name) \
  g_object_set_data (G_OBJECT (component), name, widget)

extern struct eXosip_t *context_eXosip;

#include "extern_vars.h"
#include "preferences.h"
#include "presence.h"
#include "watchers.h"
#include "xcap.h"
#include "callbacks.h"
#include "interface.h"
// #include "support.h"
#include "media.h"
#include "sysdep.h"
#include "uuid.h"
#include "useful_methods.h"
#include "common_exosip_event_handler.h"
#include "common_interface_event_handler.h"
#include "ims_exosip_event_handler.h"
#include "ims_interface_event_handler.h"
#include "DigestAKAv1MD5.h"
#include "sound_conv.h"
#include "sdp_methods.h"
#include "gstreamer.h"
#include "rtsp.h"
#include "endpointmsrp.h"
#include "iptv.h"
#include "iptv_epg.h"

#endif
