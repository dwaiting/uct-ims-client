#ifndef EXTERN_VARS_H
#define EXTERN_VARS_H

extern char *filepath;
extern char *preferences_file;

extern int mode;
extern int state;
extern int current_dialog_id;

extern char presentity[100];

extern ClientUI *client;

extern char *ims_service_route[50];
extern int num_service_routes;

extern char *ims_associated_uris[20];
extern int num_associated_uris;

extern char input[10];
extern int is_message_deregister;
extern int im_window_open;
extern int preferences_open;
extern int preferences_changed;

extern int xdms_download_open;
extern int xdms_upload_open;
extern int xml_file_dialog_open;

extern char *sip_str;
extern char *sip_strs;

extern struct timeval start_time;
extern struct timeval end_time;

extern char display[500];

extern GList *buddy_list;
extern GList *watcher_list;

extern int winfo_subscription_did;

extern GstElement *backgroundVideoPipeline;

extern char *access_networks[6];

extern int num_im_tabs;

extern GtkWidget *imsUA;
extern GtkWidget *videoWin; 

extern GtkWindow *im_window;
extern GtkWidget *preferences;

extern GtkWindow *xdms_download_window;
extern GtkWindow *xdms_upload_window;

extern GtkWidget *xml_file_dialog;

extern int time_stamps;
extern int reg_id;
extern int registered;
extern int dtmf_tab[16];

extern int full_screen;

//vod variables
extern GtkWindow *vod_window;
// extern mediacontrol_Exception *media_excp;
// extern mediacontrol_Instance *media_inst;
// extern int vod_window_open;
// extern int vod_state;
// extern int vod_full_screen;

//msrp var
extern MsrpEndpoint *local_msrp_endpoint;
extern int msrp_destroyed;

// Preferences *pref;

//IPTV external variables - Robert Marston July 2008
// extern GtkWidget *iptv_epg_window;
// extern GtkWidget *iptv_vod_epg_window;
// extern int iptv_epg_window_open;
// extern int iptv_vod_epg_window_open;
// extern GtkTreeStore *iptv_vod_treestore;

#endif
