#ifndef GLOBALS_H
#define GLOBALS_H

/* change to /usr/share/uctimsclient/ for the debian package */
char *filepath = "./";
char *preferences_file = "preferences.xml";

int mode = IMS_MODE;

int state;
int current_dialog_id;

char presentity[100] = "Available";

ClientUI *client;

char *ims_service_route[50];
int num_service_routes = 0;

char *ims_associated_uris[20];
int num_associated_uris = 0;

char input[10];
int is_message_deregister = 0;
int im_window_open = 0;
int preferences_open = 0;
int preferences_changed = 0;

int xdms_download_open = 0;
int xdms_upload_open = 0;

int xml_file_dialog_open = 0;

char *sip_str = "sip:";
char *sip_strs = "sips:";

struct timeval start_time;
struct timeval end_time;

char display[500];

GList *buddy_list;
GList *watcher_list;

int winfo_subscription_did = -1;

GstElement *backgroundVideoPipeline;

char *access_networks[6] = {
	"IEEE-802.11a",
	"IEEE-802.11b",
	"3GPP-GERAN",
	"3GPP-UTRAN-FDD",
	"3GPP-UTRAN-TDD",
	"3GPP-CDMA2000"
};

int num_im_tabs = 0;

GtkWidget *imsUA;
GtkWidget *videoWin;

GtkWindow *im_window;
GtkWidget *preferences;

GtkWidget *xml_file_dialog;

GtkWindow *xdms_download_window;
GtkWindow *xdms_upload_window;

int time_stamps = NOT_VISIBLE;
int reg_id = -1;
int registered = NOT_REGISTERED;
int dtmf_tab[16]={'0','1','2','3','4','5','6','7','8','9','*','#','A','B','C','D'};

int full_screen = 0;

// Preferences *pref;

//vod/RTSP global variables
int vod_window_open = 0;
int vod_state = IDLE;
GtkWindow *vod_window;
int vod_full_screen = 0;

//MSRP global var
MsrpEndpoint *local_msrp_endpoint;
int msrp_destroyed = 1;

//IPTV global vars - Robert Marston July 2008
GtkWidget *iptv_epg_window;
GtkWidget *iptv_vod_epg_window;
int iptv_epg_window_open = 0;
int iptv_vod_epg_window_open = 0;
char *iptv_epg_server = 0;
char *iptv_media_server = 0;
GtkTreeStore *iptv_vod_treestore;

#endif
