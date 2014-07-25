/*
  The UCT IMS Client
  Copyright (C) 2008 - University of Cape Town
  David Waiting <david@crg.ee.uct.ac.za>
  Richard Good <rgood@crg.ee.uct.ac.za>
  Robert Marston <rmarston@crg.ee.uct.ac.za> (This file)

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
#include "iptv_epg.h"
#include <libxml/parser.h>
#include <libxml/tree.h>

#define DESC_MAX_LINE 40

//Structs to hold information about videos and tv programmes
typedef struct {
    char *channel_id;
    char *title;
    char *sub_title;
    char *start_time;
    char *description;
    char *category_main;
    char *category_secondary;
} programme;

typedef struct {
    char *series_id;
    char *type;
    char *season_num;
    char *title;
    char *secondary_title;
    char *description;
    char *rating;
    char *runtime;
    char *category;
} video;

enum {
    COL_CHANNEL_ID = 0,
    COL_CHANNEL,
    COL_START_TIME,
    COL_PROGRAM_NAME,
    COL_PROGRAM_INFO,
    COL_CATEGORY,
    COL_PROGRAMME_INFO,
    NUM_CHANNEL_COLS
};

enum {
    COL_VIDEO_SERIESID = 0,
    COL_VIDEO_TITLE,
    COL_VIDEO_EPS,
    COL_VIDEO_TYPE,
    COL_VIDEO_RATING,
    COL_VIDEO_CATEGORY,
    COL_VIDEO_INFO,
    NUM_VOD_COLS
};


/*static void iptv_epg_subscribe() {
    if (!registered)
        return ;

    osip_message_t *subscribe;

    if(eXosip_subscribe_build_initial_request (&subscribe, pref->impu, pref->impu, imsua_add_lr_to_route(add_sip_scheme(pref->pcscf)), "iptv.guide", 60000)) {
            fprintf(stderr, "UCTIMSCLIENT: Error building presence.winfo subscribe message. Probably an invalid URI.\n");
            eXosip_unlock();
            return ;
    }

    imsua_add_service_routes(&subscribe);

    eXosip_lock();
    int i = eXosip_subscribe_send_initial_request(subscribe);
    eXosip_unlock();
    if(i < 0)
    fprintf(stderr, "UCTIMSCLIENT: Warning could not subscribe to content guide. VoD and TV listings may be out of date.\n");
}*/

//Function: Extracts the channel's name from the tree for a given channel id.
static void extract_channel_or_series_name(xmlNode * a_node, GtkTreeStore *iptv_epg_store, xmlChar *id, int vod) {
    xmlNode *cur_node = NULL;
    GtkTreeIter iter;

    //Look through all the nodes
    for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
        //When an element is found
        if (cur_node->type == XML_ELEMENT_NODE) {
            //Check if its the display-name of the channel
            if(!xmlStrcmp(cur_node->name,(const xmlChar *)"display-name")) {
                    //If so then add it to the store
                    gtk_tree_store_append(iptv_epg_store, &iter, NULL);

                    if(vod) {
                        gtk_tree_store_set(iptv_epg_store, &iter,
                                            COL_VIDEO_SERIESID, id,
                                            COL_VIDEO_TITLE, xmlNodeGetContent(cur_node),
                                            -1);
                    }
                    else {
                        gtk_tree_store_set(iptv_epg_store, &iter,
                                            COL_CHANNEL_ID, id,
                                            COL_CHANNEL, xmlNodeGetContent(cur_node),
                                            -1);
                    }
            }
        }
        //Recurse through the tree looking for elements
        extract_channel_or_series_name(cur_node->children, iptv_epg_store, id, vod);
    }

}

//Function: Extracts information out of the tree about the programme and adds it to the programme struct
static void extract_programme_info(xmlNode * a_node, programme *prog) {
    xmlNode *cur_node = NULL;

    for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
        //Check if current node is an element
        if (cur_node->type == XML_ELEMENT_NODE) {
            //Set up the programme properties according to the elements we find
            if(!xmlStrcmp(cur_node->name,(const xmlChar *)"title"))
                prog->title = xmlNodeGetContent(cur_node);
            else if(!xmlStrcmp(cur_node->name,(const xmlChar *)"sub-title"))
                prog->sub_title = xmlNodeGetContent(cur_node);
            else if(!xmlStrcmp(cur_node->name,(const xmlChar *)"desc"))
                prog->description = xmlNodeGetContent(cur_node);
            else if(!xmlStrcmp(cur_node->name,(const xmlChar *)"category")) {
                if(prog->category_main == NULL)
                    prog->category_main = xmlNodeGetContent(cur_node);
                else
                    prog->category_secondary = xmlNodeGetContent(cur_node);
            }
        }
        //Recurse through subtree looking for elements
        extract_programme_info(cur_node->children, prog);
    }
}

//Function: Extracts information out of the tree about the video and adds it to the video
static void extract_video_info(xmlNode * a_node, video *vid) {
    xmlNode *cur_node = NULL;

    for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
        //Check if current node is an element
        if (cur_node->type == XML_ELEMENT_NODE) {
            //Set up the programme properties according to the elements we find
            if(!xmlStrcmp(cur_node->name,(const xmlChar *)"title"))
                vid->title = xmlNodeGetContent(cur_node);
            else if(!xmlStrcmp(cur_node->name,(const xmlChar *)"secondary-title"))
                vid->secondary_title = xmlNodeGetContent(cur_node);
            else if(!xmlStrcmp(cur_node->name,(const xmlChar *)"desc"))
                vid->description = xmlNodeGetContent(cur_node);
            else if(!xmlStrcmp(cur_node->name,(const xmlChar *)"category")) 
                vid->category = xmlNodeGetContent(cur_node);
            else if(!xmlStrcmp(cur_node->name,(const xmlChar *)"length"))
                vid->runtime = xmlNodeGetContent(cur_node);
            else if(!xmlStrcmp(cur_node->name,(const xmlChar *)"rating")) {
                xmlNode *cur_node_child = NULL;//need to look for the child element with name value
                for (cur_node_child = cur_node->children; cur_node_child; cur_node_child = cur_node_child->next) {
                    if(!xmlStrcmp(cur_node_child->name,(const xmlChar *)"value"))
                        vid->rating = xmlNodeGetContent(cur_node_child);
                }
            }
            else if(!xmlStrcmp(cur_node->name,(const xmlChar *)"episode-num"))
                vid->season_num = xmlNodeGetContent(cur_node);
        }
        //Recurse through subtree looking for elements
        extract_video_info(cur_node->children, vid);
    }
}

//Function: Finds the parent channel for a programme and then adds the information as a child of the channel
static int add_programme_to_store(programme* prog, GtkTreeStore *iptv_epg_store) {
    GtkTreeIter iter, child;
    gboolean valid;
    char *cur_chanID;

    //Do some checks so our application doesn't crash and burn with invalid pointers
    if(prog->description == NULL)
        prog->description = "No Information found.";

    //Get a reference to the first node in the store to start looking from
    valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(iptv_epg_store), &iter);
    while(valid) {
        //Get the channel id at the current location
        gtk_tree_model_get (GTK_TREE_MODEL(iptv_epg_store), &iter,
                                COL_CHANNEL_ID, &cur_chanID,
                                -1);
        //compare the channel id at the current location to the programmes parent channel
        if(!strcmp(prog->channel_id, cur_chanID)) {
            //Set up the information for the programme so it can be displayed when user selects it in tree view
            gchar *programme_info = g_strjoin("", prog->description, "\n\n", "Category: ", prog->category_main, NULL);

            //found the programmes parent channel so append all the information
            gtk_tree_store_append(iptv_epg_store, &child, &iter);
            gtk_tree_store_set(iptv_epg_store, &child,
                            COL_CHANNEL_ID, prog->channel_id,
                            COL_START_TIME, prog->start_time,
                            COL_PROGRAM_NAME, prog->title,
                            COL_PROGRAM_INFO, programme_info,
                            COL_CATEGORY, prog->category_main,
                            -1);
            //free up memory created for variable
            g_free (cur_chanID);
            g_free (programme_info);
            return 1;
        }
       //Make iter point to the next row in the list store
       valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(iptv_epg_store), &iter);
    }

    //Error did not find a parent channel to add programme to
    return 0;
}

//Function: Adds the information about the video to the store.
static int add_video_to_store(video *vid, GtkTreeStore *iptv_epg_store) {
    GtkTreeIter iter, child;
    gboolean valid;
    gchar *sec_title, *info_string;
    int info_string_len;

    //Do some checks here so our program doesn't crash and burn with invalid pointers
    if(vid->secondary_title == NULL)
        //If no secondary title then just append an empty string in its place
        sec_title = "";
    else {
        //If secondary title then append a new line to it for formating purposes
        sec_title = g_strjoin("", vid->secondary_title, "\n\n", NULL);
    }

    if(vid->category == NULL)
        vid->category = "N\\A";

    if(vid->description == NULL)
        vid->description = "No Information found.";

    if(vid->rating == NULL)
        vid->rating = "WARNING: NOT RATED";

     if(vid->runtime == NULL)
        vid->runtime = "N\\A";

    //Copy all the information into one string so we can store it and recall it when user selects video in vew
    info_string = g_strjoin("", sec_title, vid->description, "\n\n", "Category: ", vid->category, "\n", "Rating: ", vid->rating, "\n", "Runtime: ", vid->runtime, " mins", NULL);

    //Get a reference to the first node in the store to start looking from
    valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(iptv_epg_store), &iter);

    //Check if this video is part of a series
    if(vid->series_id != NULL) {
        gchar *season_eps, **delim_str;
        char *cur_series_id;
        //Need information regarding if video belongs to series
        if(vid->season_num != NULL) {
            delim_str = g_strsplit_set(vid->season_num,"./",-1);
            if((unsigned int)g_strv_length(delim_str) == 4) {
                //Check if video is part of a season
                if(g_ascii_strcasecmp(delim_str[0], "0")) {
                    season_eps = g_strconcat("S",delim_str[0],"E",delim_str[1],NULL);
                }
                else {
                    season_eps = delim_str[1];
                }
                //Check if video is part x of an episode split into parts
                if(g_ascii_strcasecmp(delim_str[2], "0")) {
                    gchar *temp = g_strjoin("",season_eps,NULL);
                    season_eps = g_strjoin(temp,"_",delim_str[2],NULL);
                    g_free(temp);
                }
                g_free(delim_str);
            }
            else {
                printf("Error: The episode-num for %s is not in the correct format. Should be season.epiosde_number.part/number_of_seasons\n", vid->title);
                season_eps = g_strconcat("?",NULL);
            }
        }
        else {
            printf("Warning: The video found is part of a series but does not contain an episode number.\n");
            season_eps = g_strconcat("?",NULL);
        }
        while(valid) {
            //Get the channel id at the current location
            gtk_tree_model_get (GTK_TREE_MODEL(iptv_epg_store), &iter,
                                    COL_VIDEO_SERIESID, &cur_series_id,
                                    -1);
            //compare the channel id at the current location to the programmes parent channel
            if(!strcmp(vid->series_id, cur_series_id)) {
                char *parent_type;
                //found the programmes parent channel so append all the information
                gtk_tree_store_append(iptv_epg_store, &child, &iter);
                gtk_tree_store_set(iptv_epg_store, &child,
                                        COL_VIDEO_SERIESID, vid->series_id,
                                        COL_VIDEO_EPS, season_eps,
                                        COL_VIDEO_TYPE, vid->type,
                                        COL_VIDEO_TITLE, vid->title,
                                        COL_VIDEO_RATING, vid->rating,
                                        COL_VIDEO_CATEGORY, vid->category,
                                        COL_VIDEO_INFO, info_string,
                                        -1);

                //When we filter results we may need to remove a parent node which is the name of the series
                gtk_tree_model_get (GTK_TREE_MODEL(iptv_epg_store), &iter,
                                    COL_VIDEO_TYPE, &parent_type,
                                    -1);

                //If NULL then hasn't been set before so just set it to the child nodes type
                if(parent_type == NULL) {
                    gtk_tree_store_set(iptv_epg_store, &iter, COL_VIDEO_TYPE, vid->type, -1);
                }
                else {
                    //If not NULL then has been set so check if the current type differs from the childs type
                    if(strcmp(parent_type, vid->type)) {
                        //If they are not the same type we presume this is not a mistake the series
                        //contains both movies and tv shows
                        gtk_tree_store_set(iptv_epg_store, &iter, COL_VIDEO_TYPE, "MIXED", -1);
                    }
                    //Else do nothing since the parents type is the same as the type of the video we are adding
                }

                //free up memory
                g_free (info_string);
                g_free (season_eps);
                return 1;
            }
            //Make iter point to the next row in the list store
            valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(iptv_epg_store), &iter);
        }

        printf("Error: Could not find a parent series id to append show to\n");
        return 0;
    }
    else {//just a tv show or movie that doesnt belong to a series
        gtk_tree_store_append(iptv_epg_store, &iter, NULL);
        gtk_tree_store_set(iptv_epg_store, &iter,
                                    COL_VIDEO_SERIESID, "",
                                    COL_VIDEO_TYPE, vid->type,
                                    COL_VIDEO_TITLE, vid->title,
                                    COL_VIDEO_RATING, vid->rating,
                                    COL_VIDEO_CATEGORY, vid->category,
                                    COL_VIDEO_INFO, info_string,
                                    -1);
        g_free (info_string);
        return 1;
    }

    //shouldnt have got here
    return 0;
}

//Function: Searches though the xml DOM tree for known elements
static void search_tree(xmlNode * a_node, GtkTreeStore *iptv_epg_store, int vod) {
    xmlNode *cur_node = NULL;
    GtkTreeIter child;
    xmlChar *id;

    for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
        if (cur_node->type == XML_ELEMENT_NODE) {

            //Look for channel or series elements
            if(!xmlStrcmp(cur_node->name,(const xmlChar *)"channel") || !xmlStrcmp(cur_node->name,(const xmlChar *)"series")) {
                //Get the channel's id from the id property
                id = xmlGetProp(cur_node, (const xmlChar *)"id");
                //Find out the display name of the channel and add all info to the treestore
                extract_channel_or_series_name(cur_node->children, iptv_epg_store, id, vod);
            }

            //look for programme elements
            else if(!xmlStrcmp(cur_node->name,(const xmlChar *)"programme")) {
                //Create the struct used to store info about the programme
                programme *prog = (programme*)malloc(sizeof(programme));
                //Initialise these to null since we use them for checks
                //for the programme FIXME: From the code we limit it to to perhaps with a list we can get more?
                prog->description = NULL;
                prog->category_main = NULL;
                prog->category_secondary = NULL;
                //The channel id of the channel the programme belongs to and its start time are properties
                //in the programme element
                prog->channel_id = (char*)xmlGetProp(cur_node, (const xmlChar *)"channel");
                prog->start_time = (char*)xmlGetProp(cur_node, (const xmlChar *)"start");
                //Reads out selected information from the xml content guide to a program struct
                extract_programme_info(cur_node->children, prog);
                //Add the programme to the store as a child of the channel it belongs to
                if(!add_programme_to_store(prog,iptv_epg_store))
                    printf("Error: Could not add %s to the store since could not find any channel it belongs to\n", prog->title);
                free(prog);
            }

            //look for video elements
            else if(!xmlStrcmp(cur_node->name,(const xmlChar *)"video")) {
                //Create the struct used to store info about the video
                video *vid = (video*)malloc(sizeof(video));
                //initialise to null since we check these later
                vid->description = NULL;
                vid->rating = NULL;
                vid->secondary_title = NULL;
                vid->runtime = NULL;
                vid->category = NULL;
                vid->season_num = NULL;

                //check what type of video we are dealing with
                vid->type = xmlGetProp(cur_node, (const xmlChar *)"type");

                //Might have a series attribute associated with it
                vid->series_id = xmlGetProp(cur_node, (const xmlChar *)"series");

                //Get all the information about the video
                extract_video_info(cur_node->children, vid);

                //Add the programme to the store as a child of the channel it belongs to
                if(!add_video_to_store(vid,iptv_epg_store))
                    printf("Error: There was a problem adding the video to the store.\n", vid->title);
                free(vid);
            }
        }
        search_tree(cur_node->children, iptv_epg_store, vod);
    }
}

//Function: Extracts out information about channels and programmes from an xml document
static GtkTreeModel* iptv_epg_extract_guide(int vod) {
    xmlDoc *doc = NULL;
    xmlNode *root_element;
    xmlParserCtxtPtr ctxt;
    GtkTreeStore *epg_store;
    char *filename, *root_element_name;

    //Change the following lines if you need to use another file
    if(vod) {
        filename = imsua_addpath("vod_epguide.xml");
        root_element_name = "vod";
    }
    else {
        filename = imsua_addpath("tv_epguide.xml");
        root_element_name = "tv";
    }

    //Set up the columns for our tree store
    if(vod) {
        //We need to use the global variable for the store here if we want to do filter functions on it (curses!)
        iptv_vod_treestore = gtk_tree_store_new(7, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
        epg_store = iptv_vod_treestore;
    }
    else
        epg_store = gtk_tree_store_new(6, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

    ctxt = xmlNewParserCtxt();
    if (ctxt == NULL) {
        fprintf(stderr, "Failed to allocate parser context\n");
        return GTK_TREE_MODEL(epg_store);;
    }

    //Check if epguide.xml exists and open it if it does
    if(NULL == (doc = xmlCtxtReadFile(ctxt, filename, NULL, XML_PARSE_DTDVALID))) {
        printf("error: Failed to parse file %s\n", filename);
    }
    else {
            if(ctxt->valid == 0) {
	        fprintf(stderr, "Failed to validate %s\nRead above too see why the xml file is not valid.\n", filename);
            }
            else {
                //Get the root element node
                root_element = xmlDocGetRootElement(doc);

                //Call the function to look through the tree and extract out information to the store
                search_tree(root_element, epg_store, vod);
                //FIXME: Still plenty that can be done here
                //eg. sorting channels by start time and changing the way the time is diplayed
            }
    }

    //free the document
    xmlFreeDoc(doc);

    //Free the global variables that may
    //have been allocated by the parser.
    xmlCleanupParser();
    //Free up parser
    xmlFreeParserCtxt(ctxt);

    return GTK_TREE_MODEL(epg_store);
}

//Function used to show the guide in the ims client window
void iptv_epg_show_guide() {
    //Check if vod window is already open if not then construct view and show it
    if(iptv_epg_window_open == 0) {
    GtkWidget           *iptv_epg_view;
    GtkTreeViewColumn   *col;
    GtkCellRenderer     *renderer;
    GtkTreeModel        *model;

    /* Set up the EPG Window */
    iptv_epg_window_open = 1;
    iptv_epg_window = create_iptv_epg_window();

    iptv_epg_view = lookup_widget(GTK_WIDGET(iptv_epg_window), "iptv_epg_treeview");

    //First we call the iptv_epg_extract_guide function to read out the xml file and get all the
    //channel and tv programme information and store it in our model.
    model = iptv_epg_extract_guide(0);

    //Then we set up our view so it can show some selected information from the model
    /* --- Column #1 --- */
    col = gtk_tree_view_column_new();

    gtk_tree_view_column_set_title(col, "Channel");

    /* pack tree view column into tree view */
    gtk_tree_view_append_column(GTK_TREE_VIEW(iptv_epg_view), col);

    renderer = gtk_cell_renderer_text_new();

    /* pack cell renderer into tree view column */
    gtk_tree_view_column_pack_start(col, renderer, TRUE);

    /* connect 'text' property of the cell renderer to
    *  model column that contains the first name */
    gtk_tree_view_column_add_attribute(col, renderer, "text", COL_CHANNEL);

    /* --- Column #2 --- */
    col = gtk_tree_view_column_new();

    gtk_tree_view_column_set_title(col, "Title");

    /* pack tree view column into tree view */
    gtk_tree_view_append_column(GTK_TREE_VIEW(iptv_epg_view), col);

    renderer = gtk_cell_renderer_text_new();

    /* pack cell renderer into tree view column */
    gtk_tree_view_column_pack_start(col, renderer, TRUE);

    /* connect 'text' property of the cell renderer to
    *  model column that contains the first name */
    gtk_tree_view_column_add_attribute(col, renderer, "text", COL_PROGRAM_NAME);

    /* --- Column #3 --- */
    col = gtk_tree_view_column_new();

    gtk_tree_view_column_set_title(col, "Start Time");

    /* pack tree view column into tree view */
    gtk_tree_view_append_column(GTK_TREE_VIEW(iptv_epg_view), col);

    renderer = gtk_cell_renderer_text_new();

    /* pack cell renderer into tree view column */
    gtk_tree_view_column_pack_start(col, renderer, TRUE);

    /* connect 'text' property of the cell renderer to
    *  model column that contains the first name */
    gtk_tree_view_column_add_attribute(col, renderer, "text", COL_START_TIME);

    //set the view to show the model we are using
    gtk_tree_view_set_model(GTK_TREE_VIEW(iptv_epg_view), model);

    //We don't need the model anymore so can destroy it
    g_object_unref(model);

    //Lastly we show the window with the view
    gtk_widget_show(GTK_WIDGET(iptv_epg_window));
    }
    else //bring focus to preferences window if already open
    {
        gtk_window_present(GTK_WINDOW(iptv_epg_window));
    }
}

void iptv_vod_epg_show_guide() {
    if(iptv_vod_epg_window_open == 0) {

    GtkTreeViewColumn   *col;
    GtkCellRenderer     *renderer;
    GtkWidget           *iptv_vod_epg_view;
    GtkTreeModel        *model;
    GtkTreeSelection    *selection;

    // Set up the VoD EPG Window
    iptv_vod_epg_window_open = 1;
    iptv_vod_epg_window = create_iptv_vod_epg_window();

    iptv_vod_epg_view = lookup_widget(GTK_WIDGET(iptv_vod_epg_window), "iptv_vod_epg_treeview");

    /* --- Column #1 --- */
    col = gtk_tree_view_column_new();

    gtk_tree_view_column_set_title(col, "Video Title");

    /* pack tree view column into tree view */
    gtk_tree_view_append_column(GTK_TREE_VIEW(iptv_vod_epg_view), col);

    renderer = gtk_cell_renderer_text_new();

    /* pack cell renderer into tree view column */
    gtk_tree_view_column_pack_start(col, renderer, TRUE);

    /* connect 'text' property of the cell renderer to
    *  model column that contains the first name */
    gtk_tree_view_column_add_attribute(col, renderer, "text", COL_VIDEO_TITLE);

    /* --- Column #2 --- */
    col = gtk_tree_view_column_new();

    gtk_tree_view_column_set_title(col, "Number");

    /* pack tree view column into tree view */
    gtk_tree_view_append_column(GTK_TREE_VIEW(iptv_vod_epg_view), col);

    renderer = gtk_cell_renderer_text_new();

    /* pack cell renderer into tree view column */
    gtk_tree_view_column_pack_start(col, renderer, TRUE);

    /* connect 'text' property of the cell renderer to
    *  model column that contains the first name */
    gtk_tree_view_column_add_attribute(col, renderer, "text", COL_VIDEO_EPS);

    model = iptv_epg_extract_guide(1);

    gtk_tree_view_set_model(GTK_TREE_VIEW(iptv_vod_epg_view), model);

    //Ensure that only one row can be selected at a time
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(iptv_vod_epg_view));
    gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);

    gtk_widget_show(GTK_WIDGET(iptv_vod_epg_window));
    }
    else //bring focus to preferences window
    {
        gtk_window_present(GTK_WINDOW(iptv_vod_epg_window));
    }
}

//Function: Determines which row was selected in the epguide view and displays info about it
void iptv_epg_row_selected(GtkTreeView *treeview, int vod) {

    GtkTreeModel *model;
    GtkTreeIter   iter;
    GtkWidget           *iptv_title_label;
    GtkWidget           *iptv_text_view;
    GtkTreeSelection    *selection;

    //Get the selection from the view
    selection = gtk_tree_view_get_selection(treeview);

    //We need to figure out from the model which row was selected
    model = gtk_tree_view_get_model(treeview);

    //Get the iterator corresponding to the selected row in the view
    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        gchar *title, *info;

        if(vod == 1) {
            gtk_tree_model_get(model, &iter, COL_VIDEO_INFO, &info, COL_VIDEO_TITLE, &title, -1);
            iptv_title_label = lookup_widget(GTK_WIDGET(iptv_vod_epg_window), "iptv_vod_video_title_label");
            iptv_text_view = lookup_widget(GTK_WIDGET(iptv_vod_epg_window), "iptv_vod_text_view");
        }
        else {
            gtk_tree_model_get(model, &iter, COL_PROGRAM_NAME, &title, COL_PROGRAM_INFO, &info, -1);
            iptv_title_label = lookup_widget(GTK_WIDGET(iptv_epg_window), "iptv_programme_title_label");
            iptv_text_view = lookup_widget(GTK_WIDGET(iptv_epg_window), "iptv_text_view");
        }

        //Here we set the label to be the name of the show we are viewing info on
        if(title != NULL) {
            gchar *temp;
            temp =  g_strjoin(" ",title,"Information",NULL);
            gtk_label_set_text(GTK_LABEL(iptv_title_label), temp);
            g_free(temp);
        }

        //Here we put all the info into the text view in the epg window
        if(info != NULL) {
            GtkTextBuffer *text_view_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(iptv_text_view));
            gtk_text_buffer_set_text( text_view_buffer, info, strlen(info) );
        }

        //Free up memory
        g_free(info);
        g_free(title);
    }
}

//Function: Determines which row was double-clicked by the user and builds a sip_uri in order to obtain the content
void iptv_epg_row_activated(GtkTreeView *treeview, GtkTreePath *path, int vod) {
    GtkTreeModel *model;
    GtkTreeIter   iter;
    GtkWidget           *iptv_title_label;
    GtkWidget           *iptv_text_view;

    //We need to figure out from the model which row was selected
    model = gtk_tree_view_get_model(treeview);

    //Get the iterator corresponding to the selected row in the view
    if (gtk_tree_model_get_iter(model, &iter, path)) {
    gchar *sip_uri;

    //The only nodes that have children are nodes that show the series name
    //and we dont want to allow user to select them so just return.
    if(gtk_tree_model_iter_has_child(model,&iter))
        return;

     //Since we distinguish between video on demand and broadcast tv
    if(vod == 1) {
        //NOTE: we presume here that the sip uri is given by showname_season&eps number
        gchar *title, *eps;
        gtk_tree_model_get(model, &iter, COL_VIDEO_TITLE, &title, COL_VIDEO_EPS, &eps, -1);
        //we added the eps number to be a blank string if it wasn't part of a series
        if(eps != NULL) {
            gchar  *temp_str;
            //append the episode number details to the title
            temp_str = g_strconcat(title, "_", eps, NULL);
            //convert all ascci characters to lower case
            sip_uri = g_ascii_strdown(temp_str,strlen(temp_str));
            g_free(temp_str);
        }
        else
            sip_uri = g_ascii_strdown(title,strlen(title));
        //convert all ' ' to '_'
        sip_uri = g_strdelimit(sip_uri," ",'_');
        //Free up memory we used
        g_free(title);
        g_free(eps);
    }
    else //TV show on a channel so we used the channel id as the sip_uri
        gtk_tree_model_get(model, &iter, COL_CHANNEL_ID, &sip_uri, -1);

    //once we have got the uri we send an invite to the iptv application server
    iptv_invite_iptvAS((char*)sip_uri);

    g_free(sip_uri);
    }
}
//Function: Used to determine if a row must be shown or not when the model is filterd
static gboolean filter_visible_func(GtkTreeModel *model, GtkTreeIter *iter, gpointer data) {
    gchar *type, *comp_string, *filter_string;
    int filter;

    // Get value from column
    gtk_tree_model_get( GTK_TREE_MODEL(model), iter, COL_VIDEO_TYPE, &type, -1 );

    //Check that this is not a parent series node with both movies and tvshows
    if(!g_ascii_strcasecmp(type, "mixed"))
        return TRUE;//return true if it is since we want to keep it

    //Cast the gpointer back to integer
    filter = (int)data;

    //Check what we should be filtering
    if(filter == 1)
        filter_string = "movie";
    else
        filter_string = "tvshow";

    //Do the comparison
    if(g_ascii_strcasecmp(type, filter_string)) {
        return FALSE;
    }
    else {
        return TRUE;
    }
}

//Function: Used to filter the treeview so it only displays movies or tvshows or both
void iptv_vod_filter(int filter) {
    GtkTreeModel *model, *filter_model;
    GtkTreeView  *vod_treeview;
    gpointer data;

    vod_treeview = GTK_TREE_VIEW(lookup_widget(GTK_WIDGET(iptv_vod_epg_window), "iptv_vod_epg_treeview"));
    model = GTK_TREE_MODEL(iptv_vod_treestore);

    if(filter == 0) {//Display both movies and videos
        gtk_tree_view_set_model(vod_treeview, model);
    }
    else {
        //We need to pass a gpointer so cast the integer
        data = (gpointer)filter;

        //Create new model for the filtered tree
        filter_model = gtk_tree_model_filter_new( model, NULL );

        //Set the function of the filter to be the function we created to decied if a row should be removed or kept
        gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER ( filter_model ),(GtkTreeModelFilterVisibleFunc) filter_visible_func, data , NULL);

        //Set the treeview to use the new filtered model
        gtk_tree_view_set_model(vod_treeview,filter_model);

        //Unreference the model when the view if finished with it
        g_object_unref( filter_model );
        return;
    }
}
