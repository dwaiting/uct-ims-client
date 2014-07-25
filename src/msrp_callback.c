#include "msrp_callback.h"

/*
	The wrapper to callback the application about endpoints
*/
void local_ep_callback(int event, MsrpEndpoint *endpoint, int content, void *data, int bytes)
{
	if(ep_callback)
		ep_callback(event, endpoint, content, data, bytes);
}

/*
	The wrapper to callback the application about switches
*/
void local_sw_callback(int event, MsrpSwitch *switcher, unsigned short int user, void *data, int bytes)
{
	if(sw_callback)
		sw_callback(event, switcher, user, data, bytes);
}

/*
	The wrapper to compose the event notification strings
*/
void local_events(int event, char *format, ...)
{
	if(!events)	/* If there's no application callback, stop here */
		return;

	va_list ap;
	va_start(ap, format);

	if(!format)
		return;

	char buffer[200];
	memset(buffer, 0, 200);
	vsprintf(buffer, format, ap);

	va_end(ap);

	if(events)
		events(event, buffer);
}
