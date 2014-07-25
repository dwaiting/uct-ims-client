#ifndef _MSRP_CALLBACK_H
#define _MSRP_CALLBACK_H

#include "msrp.h"
#include "msrp_utils.h"


/*
	Several callbacks for MSRP applications to use,
	for endpoints, relays, switches and events
*/
void(*ep_callback)(int event, MsrpEndpoint *endpoint, int content, void *data, int bytes);
void(*sw_callback)(int event, MsrpSwitch *switcher, unsigned short int user, void *data, int bytes);
void(*rl_callback)(MsrpRelay *relay);
void(*events)(int event, void *info);
void local_ep_callback(int event, MsrpEndpoint *endpoint, int content, void *data, int bytes);
void local_rl_callback(MsrpRelay *relay);
void local_sw_callback(int event, MsrpSwitch *switcher, unsigned short int user, void *data, int bytes);
void local_events(int event, char *format, ...);


#endif
