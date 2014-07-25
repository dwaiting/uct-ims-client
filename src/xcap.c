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
#include "xcap.h"


struct MemoryStruct {
	char *memory;
	size_t size;
};



int xcap_find_user_in_rule(xmlNodePtr cur, char *user)
{
	xmlNode *cur_node = NULL;

	for (cur_node = cur; cur_node; cur_node = cur_node->next)
	{
	        if (cur_node->type == XML_ELEMENT_NODE) 
		{
			if((!xmlStrcmp(cur_node->name, (const xmlChar *)"one")))
			{
				if (!xmlStrcmp(xmlGetProp(cur_node, (const xmlChar *)"id"), user))
					return 1;
			}

	        }

		if(xcap_find_user_in_rule(cur_node->children, user))
			return 1;
	}
	return 0;
}


int xcap_remove_user_from_rule(xmlNodePtr cur, char *user)
{

	xmlNode *cur_node = NULL;

	for (cur_node = cur; cur_node; cur_node = cur_node->next)
	{
	        if (cur_node->type == XML_ELEMENT_NODE) 
		{
			if((!xmlStrcmp(cur_node->name, (const xmlChar *)"one")))
			{
				if (!xmlStrcmp(xmlGetProp(cur_node, (const xmlChar *)"id"), user))
				{
					xmlUnlinkNode(cur_node);
					xmlFreeNode(cur_node);
					return 1;
				}
			}

	        }

		if(xcap_remove_user_from_rule(cur_node->children, user))
			return 1;
	}
	return 0;


}

int xcap_add_user_to_rule(xmlNodePtr cur, char *user)
{
	xmlNode *cur_node = NULL;
	xmlNodePtr oneChild;

	for (cur_node = cur; cur_node; cur_node = cur_node->next)
	{
	        if (cur_node->type == XML_ELEMENT_NODE) 
		{
			if((!xmlStrcmp(cur_node->name, (const xmlChar *)"identity")))
			{
				oneChild = xmlNewChild(cur_node, NULL, "one", NULL);
				xmlSetProp(oneChild, "id", user);
				return 1;
			}

	        }

		if(xcap_add_user_to_rule(cur_node->children, user))
			return 1;
	}
	return 0;
}


int xcap_rule_is_not_empty(xmlNodePtr cur)
{
	xmlNode *cur_node = NULL;
	xmlNodePtr oneChild;

	for (cur_node = cur; cur_node; cur_node = cur_node->next)
	{
	        if (cur_node->type == XML_ELEMENT_NODE) 
		{
			if((!xmlStrcmp(cur_node->name, (const xmlChar *)"one")) || (!xmlStrcmp(cur_node->name, (const xmlChar *)"many")))
			{
				return 1;
			}

	        }

		if(xcap_rule_is_not_empty(cur_node->children))
			return 1;
	}
	return 0;
}


int xcap_rule_is_block(xmlNode *cur)
{
	xmlNode *cur_node = NULL;
	xmlNode *contents = NULL;

	for (cur_node = cur; cur_node; cur_node = cur_node->next)
	{

	        if (cur_node->type == XML_ELEMENT_NODE) 
		{

			if((!xmlStrcmp(cur_node->name, (const xmlChar *)"sub-handling")))
			{
				if (contents = cur_node->children)
					if (!xmlStrcmp(contents->content, (const xmlChar *)"block"))
						return 1;
			}

	        }

		if(xcap_rule_is_block(cur_node->children))
			return 1;
	}

	return 0;
}

int xcap_rule_is_confirm(xmlNodePtr cur)
{
	xmlNode *cur_node = NULL;
	xmlNode *contents = NULL;

	for (cur_node = cur; cur_node; cur_node = cur_node->next)
	{
	        if (cur_node->type == XML_ELEMENT_NODE) 
		{
			if((!xmlStrcmp(cur_node->name, (const xmlChar *)"sub-handling")))
			{
				if (contents = cur_node->children)
					if (!strcmp(contents->content, "confirm"))
						return 1;
			}

	        }

		if(xcap_rule_is_confirm(cur_node->children))
			return 1;
	}
	return 0;
}

int xcap_rule_is_politeblock(xmlNodePtr cur)
{
	xmlNode *cur_node = NULL;
	xmlNode *contents = NULL;

	for (cur_node = cur; cur_node; cur_node = cur_node->next)
	{
	        if (cur_node->type == XML_ELEMENT_NODE) 
		{
			if((!xmlStrcmp(cur_node->name, (const xmlChar *)"sub-handling")))
			{
				if (contents = cur_node->children)
					if (!strcmp(contents->content, "polite-block"))
						return 1;
			}

	        }

		if(xcap_rule_is_politeblock(cur_node->children))
			return 1;
	}
	return 0;
}

int xcap_rule_is_allow(xmlNodePtr cur)
{
	xmlNode *cur_node = NULL;
	xmlNode *contents = NULL;

	for (cur_node = cur; cur_node; cur_node = cur_node->next)
	{
	        if (cur_node->type == XML_ELEMENT_NODE) 
		{
			if((!xmlStrcmp(cur_node->name, (const xmlChar *)"sub-handling")))
			{
				if (contents = cur_node->children)
					if (!strcmp(contents->content, "allow"))
						return 1;
			}

	        }

		if(xcap_rule_is_allow(cur_node->children))
			return 1;
	}
	return 0;
}

xmlNsPtr xcap_get_namespace(xmlDocPtr doc, xmlNodePtr cur, char *comparehref)
{
	xmlNs *namespace;
	
	namespace = *xmlGetNsList(doc, cur);

	while (namespace != NULL)
	{
		if (!strcmp(namespace->href, comparehref))
			return namespace;
	
		namespace = namespace->next;
	}

	return NULL;
}


int xcap_delete_empty_rules(xmlDocPtr doc)
{
	xmlNodePtr cur;

	cur = xmlDocGetRootElement(doc);
	cur = cur->xmlChildrenNode;

	while (cur != NULL) 
	{
		if ((!xmlStrcmp(cur->name, (const xmlChar *)"rule")))
		{
			if (!xcap_rule_is_not_empty(cur->children))
			{
				xmlUnlinkNode(cur);
				xmlFreeNode(cur);
			}
		}
		cur = cur->next;
	}

}

xmlDocPtr xcap_add_new_rule(xmlDocPtr doc, char *ruleName, char *action, char *user)
{

	xmlNodePtr cur;
	xmlNodePtr ruleChild, conditionsChild, identityChild, oneChild, actionsChild, subhandlingChild, transformationsChild;
	xmlNsPtr crNamespace, prNamespace;

	/* now let's add a rule to the document */
	cur = xmlDocGetRootElement(doc);

	/* Get the common-policy and pres-rules namespaces */
	crNamespace = xcap_get_namespace(doc, cur, "urn:ietf:params:xml:ns:common-policy");
	prNamespace = xcap_get_namespace(doc, cur, "urn:ietf:params:xml:ns:pres-rules");

	/* set the user's uri in the id field */
	ruleChild = xmlNewChild(cur, crNamespace, "rule", NULL);
	xmlSetProp(ruleChild, "id", ruleName);
	conditionsChild = xmlNewChild(ruleChild, crNamespace, "conditions", NULL);
	identityChild = xmlNewChild(conditionsChild, crNamespace, "identity", NULL);
	oneChild = xmlNewChild(identityChild, crNamespace, "one", NULL);
	xmlSetProp(oneChild, "id", user);
	
	/* set the subhandling node to "allow" */
	actionsChild = xmlNewChild(ruleChild, crNamespace, "actions", NULL);
	subhandlingChild = xmlNewChild(actionsChild, prNamespace, "sub-handling", NULL);
	xmlNodeAddContent(subhandlingChild, action);

	/* add an empty transformations element to conform to RFC */
	transformationsChild = xmlNewChild(ruleChild, crNamespace, "transformations", NULL);

	xmlChar *xmlbuff;
	int buffersize;

	xmlDocDumpFormatMemory(doc, &xmlbuff, &buffersize, 1);

}



void xcap_allow_user(char *user)
{

	/* Update the presence_rules.xml file */

	FILE *fp;
	char *filename = PRES_RULES_DOC;
	xmlDocPtr doc;
	xmlNodePtr cur, child;
	int user_already_added = 0;


	/* parse the pres-rules file */
	doc = xmlParseFile(imsua_addpath(filename));
	
	if (doc == NULL ) {
		fprintf(stderr,"UCTIMSCLIENT ERROR: Pres-rules file not parsed successfully\n");
		return ;
	}

	cur = xmlDocGetRootElement(doc);
	if (cur == NULL) {
		fprintf(stderr,"UCTIMSCLIENT ERROR: Pres-rules file is empty\n");
		xmlFreeDoc(doc);
		return ;
	}
	
	if (xmlStrcmp(cur->name, (const xmlChar *) "ruleset")) {
		fprintf(stderr,"UCTIMSCLIENT ERROR: document of the wrong type, root node != ruleset\n");
		xmlFreeDoc(doc);
		return ;
	}	

	/* go through and look if the user is any allow rules and delete any blocking rules */
	cur = cur->xmlChildrenNode;
	while (cur != NULL) {
		if ((!xmlStrcmp(cur->name, (const xmlChar *)"rule")))
		{
			/* if the user is in a rule and they are already allowed then exit */
			if(xcap_find_user_in_rule(cur->children, user) && xcap_rule_is_allow(cur->children))
			{
				imsua_log("User already in allow rule");

				user_already_added = 1;
			}
			else if(xcap_find_user_in_rule(cur->children, user) && !xcap_rule_is_allow(cur->children))
			{
				/* if the rule is not allow then remove the user from the rule */
				imsua_log("Deleting user from rule");

				xcap_remove_user_from_rule(cur->children, user);
	
			}
			else if (!xcap_find_user_in_rule(cur->children, user) && xcap_rule_is_allow(cur->children))
			{
				imsua_log("Adding user to an existng xcap rule");

				xcap_add_user_to_rule(cur->children, user);
				user_already_added = 1;
			}


		}
		cur = cur->next;
	}

	/* delete any empty rules */
	xcap_delete_empty_rules(doc);


	/* add a new rule if no allow rules exist */
	if (!user_already_added)
	{
		imsua_log("Adding new allow rule");

		xcap_add_new_rule(doc, "pres_whitelist", "allow", user);	
 		
	}

	/* now write this document back to our presence_rules.xml file */
	if((fp = fopen(imsua_addpath(filename),"wb")) == NULL) 
	{ 
		printf("UCTIMSCLIENT WARNING: Error opening pres-rules file for writing: %s\n", imsua_addpath(filename));
 		return ;
	} 

	xmlDocDump(fp, doc);
	
	fclose(fp);

	/* now display our new document */
	char pres_display[200];
	sprintf(pres_display, "Watcher %s approved\n\nNow upload presence-rules.xml\nfrom the XDMS manager", user);
	set_display(pres_display);

	cur = xmlDocGetRootElement(doc);
	imsua_clear_presrules_display();
	xcap_pretty_print_xml(cur);

}


void xcap_block_user(char *user)
{

	/* Update the presence_rules.xml file */

	FILE *fp;
	char *filename = PRES_RULES_DOC;
	xmlDocPtr doc;
	xmlNodePtr cur, child;
	int user_already_added = 0;

	/* parse the pres-rules file */
	doc = xmlParseFile(imsua_addpath(filename));
	
	if (doc == NULL ) {
		fprintf(stderr,"UCTIMSCLIENT ERROR: Pres-rules file not parsed successfully\n");
		return ;
	}

	cur = xmlDocGetRootElement(doc);
	if (cur == NULL) {
		fprintf(stderr,"UCTIMSCLIENT ERROR: Pres-rules file is empty\n");
		xmlFreeDoc(doc);
		return ;
	}
	
	if (xmlStrcmp(cur->name, (const xmlChar *) "ruleset")) {
		fprintf(stderr,"UCTIMSCLIENT ERROR: document of the wrong type, root node != ruleset\n");
		xmlFreeDoc(doc);
		return ;
	}	

	/* go through and look if the user is any block rules and delete any allowing rules */
	cur = cur->xmlChildrenNode;
	while (cur != NULL) {
		if ((!xmlStrcmp(cur->name, (const xmlChar *)"rule")))
		{
			/* if the user is in a rule and they are already allowed then exit */
			if(xcap_find_user_in_rule(cur->children, user) && xcap_rule_is_block(cur->children))
			{
				imsua_log("User is already blocked");		

				user_already_added = 1;
			}
			else if(xcap_find_user_in_rule(cur->children, user) && !xcap_rule_is_block(cur->children))
			{
				imsua_log("Removing user from non-block rule");
				
				xcap_remove_user_from_rule(cur->children, user);
	
			}
			else if (!xcap_find_user_in_rule(cur->children, user) && xcap_rule_is_block(cur->children))
			{
				imsua_log("Adding user to existing block rule");

				xcap_add_user_to_rule(cur->children, user);
				user_already_added = 1;
			}

		}
		cur = cur->next;
	}

	/* delete any empty rules */
	xcap_delete_empty_rules(doc);

	/* if no block rules exist then add one */
	if (!user_already_added)
	{
		imsua_log("Adding new block rule");

 		xcap_add_new_rule(doc, "pres_blacklist", "block", user);

	}

	/* now write this document back to our presence_rules.xml file */
	if((fp = fopen(imsua_addpath(filename),"wb")) == NULL) 
	{ 
		printf("UCTIMSCLIENT WARNING: Error opening pres-rules file for writing: %s\n", imsua_addpath(filename));
 		return ;
	} 

	xmlDocDump(fp, doc);
	
	fclose(fp);

	/* now display our new pres-rules document */
	char pres_display[200];
	sprintf(pres_display, "Watcher %s blocked\n\nNow upload presence-rules.xml\nfrom the XDMS manager", user);
	set_display(pres_display);

	cur = xmlDocGetRootElement(doc);
	imsua_clear_presrules_display();
	xcap_pretty_print_xml(cur);

}


void xcap_reset_presrules_doc()
{

	FILE *fp;
	char fullpath[100];
	char *filename = PRES_RULES_DOC;

	sprintf(fullpath, "%s%s", filepath, filename);

	if((fp = fopen(imsua_addpath(fullpath),"wb")) == NULL) 
	{ 
		printf("UCTIMSCLIENT WARNING: Error opening pres-rules file for writing: %s\n", imsua_addpath(filename));
 		return ;
	} 

	fprintf(fp, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
	"<cr:ruleset xmlns=\"urn:ietf:params:xml:ns:pres-rules\" xmlns:pr=\"urn:ietf:params:xml:ns:pres-rules\" xmlns:cr=\"urn:ietf:params:xml:ns:common-policy\">"
	"</cr:ruleset>");
	
	fclose(fp);

	set_display("Local Pres-Rules file reset\n\nNow upload to the XCAP server");

	imsua_clear_presrules_display();

}


void xcap_pretty_print_xml(xmlNodePtr cur)
{

	xmlNode *cur_node = NULL;
	xmlNode *contents = NULL;
	
	for (cur_node = cur; cur_node; cur_node = cur_node->next)
	{
	        if (cur_node->type == XML_ELEMENT_NODE) 
		{
			if(!xmlStrcmp(cur_node->name, (const xmlChar *)"rule"))
			{
				imsua_set_presrules_display("Rule ID:  ");
				imsua_set_presrules_display(xmlGetProp(cur_node, (const xmlChar *)"id"));
				imsua_set_presrules_display("\n");
								
			}
			else if(!xmlStrcmp(cur_node->name, (const xmlChar *)"one"))
			{
				imsua_set_presrules_display("User ID:  ");
				imsua_set_presrules_display(xmlGetProp(cur_node, (const xmlChar *)"id"));
				imsua_set_presrules_display("\n");
								
			}
			else if(!xmlStrcmp(cur_node->name, (const xmlChar *)"many"))
			{
				imsua_set_presrules_display("Domain:  ");
				imsua_set_presrules_display(xmlGetProp(cur_node, (const xmlChar *)"domain"));
				imsua_set_presrules_display("\n");
								
			}
			else if (!xmlStrcmp(cur_node->name, (const xmlChar *)"sub-handling"))
			{
				imsua_set_presrules_display("Action: ");
				imsua_set_presrules_display((cur_node->children)->content);
				imsua_set_presrules_display("\n\n");

			}
			
	        }

		xcap_pretty_print_xml(cur_node->children);
		
	}
}


char *xcap_get_username_from_uri(char *uri)
{

	char *tok = NULL;
	char *return_string;
	char *uri_tok;

	uri_tok = strdup(uri);

	if (imsua_regex_match(uri, "sips?:.*[@].*") == 1)
	{
		tok = strtok(uri_tok, ":");
		tok = strtok(NULL, ":");
		tok = strtok(tok, "@");
		return_string = strdup(tok);
		return return_string;
	}
	else if (imsua_regex_match(uri, ".*[@].*") == 1)
	{
		tok = strtok(uri_tok, "@");
		return_string = strdup(tok);
		return return_string;
	}
	else
		return uri;

}


size_t WriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data)
{
   size_t realsize = size * nmemb;
   struct MemoryStruct *mem = (struct MemoryStruct *)data;
 
   mem->memory = (char *)realloc(mem->memory, mem->size + realsize + 1);
   if (mem->memory) {
     memcpy(&(mem->memory[mem->size]), ptr, realsize);
     mem->size += realsize;
     mem->memory[mem->size] = 0;
   }
   return realsize;
}


void xcap_get_presrules_from_server()
{
	/* mostly taken from the libcurl website example "getinmemory.c" */

	FILE *fp;
	int hd ;
	char *filename = PRES_RULES_DOC;

	xmlDocPtr doc;
	xmlNodePtr cur;

	CURL *handle = NULL;
	struct MemoryStruct chunk;
	struct stat file_info;
	char full_request_url[150];
	char xcap_auth[100];

	chunk.memory=NULL;
	chunk.size = 0;

	/*  check if the xcap url looks like http://xcap.com or http://xcap.com/ otherwise exit */
	if (imsua_regex_match(pref->xdms_root_url, "https?://.*/$") == 1)
	{
		sprintf(full_request_url, "%s"
		"pres-rules/users/"
		"%s/"
		"%s", pref->xdms_root_url, pref->xdms_username, PRES_RULES_DOC); 
	}		
	else if (imsua_regex_match(pref->xdms_root_url, "https?://.*") == 1)
	{
		sprintf(full_request_url, "%s/"
		"pres-rules/users/"
		"%s/"
		"%s", pref->xdms_root_url, pref->xdms_username, PRES_RULES_DOC); 
	}
	else
	{
		set_display("Error in preferences:\nIncorrect format for XCAP Server URL\n\nExample: http://xcap.com/xcap-root/");
		return ;
	}
		
	sprintf(xcap_auth, "%s:%s", xcap_get_username_from_uri(pref->xdms_username), pref->xdms_password);

	/* get the file size of the local file */
	hd = open(imsua_addpath(filename), O_RDONLY) ;
	fstat(hd, &file_info);

	/* open the file stream */
	//fp = fdopen(hd, "wb");

	handle = curl_easy_init();

	curl_easy_setopt(handle, CURLOPT_URL, full_request_url);

	/* do not store data into a file - store them in memory */
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
  
	/* we pass our 'chunk' struct to the callback function */
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, (void *)&chunk);
	// curl_easy_setopt(handle, CURLOPT_WRITEDATA, fp);

	/* auth */
	curl_easy_setopt(handle, CURLOPT_HTTPAUTH, CURLAUTH_ANY);
	curl_easy_setopt(handle, CURLOPT_NETRC, CURL_NETRC_IGNORED);
	curl_easy_setopt(handle, CURLOPT_USERPWD, xcap_auth);

	/* some servers don't like requests that are made without a user-agent field, so we provide one */
	curl_easy_setopt(handle, CURLOPT_USERAGENT, "UCT IMS Client");

	if(curl_easy_perform(handle) != 0)
	{
		fprintf(stderr, "UCTIMSCLIENT: Error sending XCAP request\n");
		return ;
	}

	curl_easy_cleanup(handle);

	if(!(doc = xmlParseDoc(chunk.memory)))
	{
		fprintf(stderr, "UCTIMSCLIENT: Presence NOTIFY XML not parsed successfully.\n" );
		xmlFreeDoc( doc );
		return ;
	}

	if(!(cur = xmlDocGetRootElement(doc)))
	{
		fprintf( stderr, "UCTIMSCLIENT: XML document has no root element.\n" );
		xmlFreeDoc( doc );
		return ;
	}

	if( xmlStrcmp( cur->name, (const xmlChar *) "ruleset" ) ){
		fprintf( stderr, "UCTIMSCLIENT: XML document of the wrong type, root node != ruleset\n" );
		xmlFreeDoc( doc );
		return ;
	}

	imsua_clear_presrules_display();
	xcap_pretty_print_xml(cur);

	/* now write this document back to our presence_rules.xml file */
	if((fp = fopen(imsua_addpath(filename),"wb")) == NULL) 
	{ 
		printf("UCTIMSCLIENT WARNING: Error opening pres-rules file for writing: %s\n", imsua_addpath(filename));
 		return ;
	} 

	xmlDocDump(fp, doc);
	
	fclose(fp);


	if(chunk.memory)
		free(chunk.memory);

}


size_t read_callback(void *ptr, size_t size, size_t nmemb, void *stream)
{
   size_t retcode;
 
   int fd = (int)stream;
 
   retcode = read(fd, ptr, size * nmemb);
 
   fprintf(stderr, "*** We read %d bytes from file\n", retcode);
 
   return retcode;
}

void xcap_put_presrules_to_server()
{

	CURL *curl;
	CURLcode res;
	int hd ;
	struct stat file_info;

	FILE *fp;
	char *filename = PRES_RULES_DOC;

	char full_request_url[150];
	char xcap_auth[100];
		
	sprintf(xcap_auth, "%s:%s", xcap_get_username_from_uri(pref->xdms_username), pref->xdms_password);
 
	/*  check if the xcap url looks like http://xcap.com or http://xcap.com/ otherwise exit */
	if (imsua_regex_match(pref->xdms_root_url, "https?://.*/$") == 1)
	{
		sprintf(full_request_url, "%s"
		"pres-rules/users/"
		"%s/"
		"%s", pref->xdms_root_url, pref->xdms_username, PRES_RULES_DOC); 
	}		
	else if (imsua_regex_match(pref->xdms_root_url, "https?://.*") == 1)
	{
		sprintf(full_request_url, "%s/"
		"pres-rules/users/"
		"%s/"
		"%s", pref->xdms_root_url, pref->xdms_username, PRES_RULES_DOC); 
	}
	else
	{
		set_display("Error in preferences:\nIncorrect format for XCAP Server URL\n\nExample: http://xcap.com/xcap-root/");
		return ;
	}

	/* get the file size of the local file */
	hd = open(imsua_addpath(filename), O_RDONLY) ;
	fstat(hd, &file_info);

	/* open the file stream */
	fp = fdopen(hd, "rb");
 
	/* get a curl handle */
	curl = curl_easy_init();
	if(curl) {

		/* we want to use our own read function */
		// curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);

		/* enable uploading */
		curl_easy_setopt(curl, CURLOPT_UPLOAD, TRUE);

		/* specify target URL, and note that this URL should include a file
        	name, not only a directory */
		curl_easy_setopt(curl,CURLOPT_URL, full_request_url);
 	
		/* now specify which file to upload */
		curl_easy_setopt(curl, CURLOPT_READDATA, fp);
 
		/* provide the size of the upload, we specicially typecast the value
		to curl_off_t since we must be sure to use the correct data size */
		curl_easy_setopt(curl, CURLOPT_INFILESIZE, (long)file_info.st_size);

		/* auth */
		curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_ANY);
		curl_easy_setopt(curl, CURLOPT_NETRC, CURL_NETRC_IGNORED);
		curl_easy_setopt(curl, CURLOPT_USERPWD, xcap_auth);

		/* some servers don't like requests that are made without a user-agent field, so we provide one */
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "UCT IMS Client");
 
		/* Now run off and do what you've been told! */
		res = curl_easy_perform(curl);

		/* always cleanup */
		curl_easy_cleanup(curl);

	}
	
	fclose(fp); /* close the local file */
 
	curl_global_cleanup();
	

}

void xcap_open_xdms_download_window()
{
	/*check if XDMS download window is open and if not open it*/
 	if(xdms_download_open == 0)
 	{
 		xdms_download_window = GTK_WINDOW(create_xdms_download_window());
 		gtk_widget_show (GTK_WIDGET(xdms_download_window));
 		xdms_download_open = 1;
 	}
 	else /* If already open bring focus to it*/
 	{
 		//bring focus to XDMS download window
 		gtk_window_present(xdms_download_window);
 	}	


}

void xcap_open_xdms_upload_window()
{
	/*check if XDMS upload window is open and if not open it*/
 	if(xdms_upload_open == 0)
 	{
 		xdms_upload_window = GTK_WINDOW(create_xdms_upload_window());
 		gtk_widget_show (GTK_WIDGET(xdms_upload_window));
 		xdms_upload_open = 1;
 	}
 	else /* If already open bring focus to it*/
 	{
 		//bring focus to XDMS download window
 		gtk_window_present(xdms_upload_window);
 	}	
}

xmlDocPtr xcap_get_xml_from_xdms_address(char *full_request_url)
{
	xmlDocPtr doc;
	xmlNodePtr cur;

	CURL *handle = NULL;
	struct MemoryStruct chunk;
	struct stat file_info;
	char xcap_auth[100];

	chunk.memory=NULL;
	chunk.size = 0;
	sprintf(xcap_auth, "%s:%s", xcap_get_username_from_uri(pref->xdms_username), pref->xdms_password);
	handle = curl_easy_init();

	curl_easy_setopt(handle, CURLOPT_URL, full_request_url);

	/* do not store data into a file - store them in memory */
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
  
	/* we pass our 'chunk' struct to the callback function */
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, (void *)&chunk);
	// curl_easy_setopt(handle, CURLOPT_WRITEDATA, fp);

	/* auth */
	curl_easy_setopt(handle, CURLOPT_HTTPAUTH, CURLAUTH_ANY);
	curl_easy_setopt(handle, CURLOPT_NETRC, CURL_NETRC_IGNORED);
	curl_easy_setopt(handle, CURLOPT_USERPWD, xcap_auth);

	/* some servers don't like requests that are made without a user-agent field, so we provide one */
	curl_easy_setopt(handle, CURLOPT_USERAGENT, "UCT IMS Client");

	
	if(curl_easy_perform(handle) != 0)
	{
		set_xdms_download_display("Error sending XCAP request\n");
		return NULL;
	}

	curl_easy_cleanup(handle);
	
	if(!(doc = xmlParseDoc(chunk.memory)))
	{
		set_xdms_download_display("XML document not parsed successfully.\nXDMS AUID probably incorrect" );
		xmlFreeDoc( doc );
		return NULL;
	}
	if(!(cur = xmlDocGetRootElement(doc)))
	{
		set_xdms_download_display("XML document has no root element.\nXDMS AUID probably incorrect" );
		xmlFreeDoc( doc );
		return NULL;
	}
	if(chunk.memory)
		free(chunk.memory);
	return doc;

	
}

int xcap_put_xml_to_xdms_address(char *full_request_url, gchar *filename)
{
	int status = 0;
	CURL *curl;
	CURLcode res;
	int hd ;
	struct stat file_info;

	FILE *fp;
	
	char xcap_auth[100];
		
	sprintf(xcap_auth, "%s:%s", xcap_get_username_from_uri(pref->xdms_username), pref->xdms_password);
 
	/* get the file size of the local file */
	hd = open(filename, O_RDONLY) ;
	fstat(hd, &file_info);

	/* open the file stream */
	fp = fdopen(hd, "rb");
 
	/* get a curl handle */
	curl = curl_easy_init();
	if(curl) {

		/* we want to use our own read function */
		// curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);

		/* enable uploading */
		curl_easy_setopt(curl, CURLOPT_UPLOAD, TRUE);

		/* specify target URL, and note that this URL should include a file
        	name, not only a directory */
		curl_easy_setopt(curl,CURLOPT_URL, full_request_url);
 	
		/* now specify which file to upload */
		curl_easy_setopt(curl, CURLOPT_READDATA, fp);
 
		/* provide the size of the upload, we specicially typecast the value
		to curl_off_t since we must be sure to use the correct data size */
		curl_easy_setopt(curl, CURLOPT_INFILESIZE, (long)file_info.st_size);

		/* auth */
		curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_ANY);
		curl_easy_setopt(curl, CURLOPT_NETRC, CURL_NETRC_IGNORED);
		curl_easy_setopt(curl, CURLOPT_USERPWD, xcap_auth);

		/* some servers don't like requests that are made without a user-agent field, so we provide one */
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "UCT IMS Client");
 
		/* Now run off and do what you've been told! */
		res = curl_easy_perform(curl);
		if(res == 0)
		{
			status = 1;
		}
		/* always cleanup */
		curl_easy_cleanup(curl);

	}
	
	fclose(fp); /* close the local file */
	curl_global_cleanup();
	return status;
}

void xcap_get_xml_from_server()
{
	/*get XDMS AUID*/
	const gchar *auid_entry;
	GtkWidget *auid_input = lookup_widget(GTK_WIDGET(xdms_download_window), "auid_input");
	auid_entry = gtk_entry_get_text(GTK_ENTRY(auid_input));
	

	char full_request_url[150];
	xmlDocPtr doc;
	
	/*Create XDMS Full request UR*/
	/*  check if the xcap url looks like http://xcap.com or http://xcap.com/ otherwise exit */
	if (imsua_regex_match(pref->xdms_root_url, "https?://.*/$") == 1)
	{
		sprintf(full_request_url, "%s"
		"%s"
		"/users/"
		"%s/"
		"%s"
		".xml", pref->xdms_root_url, auid_entry, pref->xdms_username, auid_entry); 
	}		
	else if (imsua_regex_match(pref->xdms_root_url, "https?://.*") == 1)
	{
		sprintf(full_request_url, "%s/"
		"%s"
		"/users/"
		"%s/"
		"%s"
		".xml", pref->xdms_root_url, auid_entry, pref->xdms_username, auid_entry); 
	}
	else
	{
		set_xdms_download_display("Error in preferences:\nIncorrect format for XCAP Server URL\n\nExample: http://xcap.com/xcap-root/");
		return ;
	}
	doc = xcap_get_xml_from_xdms_address(full_request_url);
	
	if(doc!=NULL)
	{
		xmlChar *xmlbuff;
		int buffersize;
		xmlDocDumpFormatMemory(doc, &xmlbuff, &buffersize, 1);
		set_xdms_download_display(xmlbuff);
		//Free associated memory.
 		xmlFree(xmlbuff);
 		xmlFreeDoc(doc);
	}

}

void xcap_put_xml_to_server()
{
	/*Get filename entry*/
	gchar file_input[200];
	GtkWidget *xml_file_input = lookup_widget(GTK_WIDGET(xdms_upload_window), "xml_file_input");
	strcpy(file_input, gtk_entry_get_text(GTK_ENTRY(xml_file_input)));
		
	/*get XDMS AUID*/
	const gchar *auid_entry;
	GtkWidget *auid_input = lookup_widget(GTK_WIDGET(xdms_upload_window), "auid_input");
	auid_entry = gtk_entry_get_text(GTK_ENTRY(auid_input));
	

	char full_request_url[150];
	xmlDocPtr doc;
	
	/*Check that document to be uploaded has .xml extensions*/
	char *file_name = file_input;
	if (imsua_regex_match(file_name, ".*(.xml$)") == 0)
	{
		set_xdms_upload_display("Error in file format:\n File to upload is not a valid XML document");
		return ;
	}
	
	/*Create XDMS Full request UR*/
	/*  check if the xcap url looks like http://xcap.com or http://xcap.com/ otherwise exit */
	if (imsua_regex_match(pref->xdms_root_url, "https?://.*/$") == 1)
	{
		sprintf(full_request_url, "%s"
		"%s"
		"/users/"
		"%s/"
		"%s"
		".xml", pref->xdms_root_url, auid_entry, pref->xdms_username, auid_entry); 
	}		
	else if (imsua_regex_match(pref->xdms_root_url, "https?://.*") == 1)
	{
		sprintf(full_request_url, "%s/"
		"%s"
		"/users/"
		"%s/"
		"%s"
		".xml", pref->xdms_root_url, auid_entry, pref->xdms_username, auid_entry); 
	}
	else
	{
		set_xdms_upload_display("Error in preferences: \nIncorrect format for XCAP Server URL\n\nExample: http://xcap.com/xcap-root/");
		return ;
	}
	
	int status = 0;
	status = xcap_put_xml_to_xdms_address(full_request_url, file_input);
	
	if(status == 1)
	{
		char output[100];
		sprintf(output,"%s successfully uploaded to XDMS Server\n",file_input);
		set_xdms_upload_display(output);
	}
	else
	{
		
		char output[100];
		sprintf(output,"Error uploading %s to XDMS Server\n",file_input);
		set_xdms_upload_display(output);
	}
	
}

void xdms_xml_file_dialog_open()
{
	/*check if XML file dialog window is open and if not open it*/
 	if(xml_file_dialog_open == 0)
 	{
 		xml_file_dialog = GTK_WIDGET(create_xml_file_dialog());
 		gtk_widget_show (GTK_WIDGET(xml_file_dialog));
 		xml_file_dialog_open = 1;
 	}
 	else /* If already open bring focus to it*/
 	{
 		//bring focus to XDMS download window
 		gtk_window_present(GTK_WINDOW(xml_file_dialog));
 	}	
}

void xcap_get_xml_filename()
{

	//get filename and close file dialog
	char *filename;
    	filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (xml_file_dialog));
	gtk_widget_destroy (xml_file_dialog);
	xml_file_dialog_open = 0;
	
 	/*check if XDMS upload window is open and if not open it*/
 	if(xdms_upload_open == 0)
 	{
 		xdms_upload_window = GTK_WINDOW(create_xdms_upload_window());
 		gtk_widget_show (GTK_WIDGET(xdms_upload_window));
 		xdms_upload_open = 1;
 	}
 	else /* If already open bring focus to it*/
 	{
 		//bring focus to XDMS download window
 		gtk_window_present(xdms_upload_window);
 	}	
 	
 	//copy file name into xml test box in XDMS upload window
 	GtkWidget *xml_file_input = lookup_widget(GTK_WIDGET(xdms_upload_window), "xml_file_input");
 	gtk_entry_set_text(GTK_ENTRY(xml_file_input),filename);
}




