#include "msrp_utils.h"

/*
	Case-insensitive implementation of strstr
*/
const char *stristr(const char *haystack, const char *needle)
{
	if(!*needle)
		return haystack;

	for(; *haystack; ++haystack) {
		if(toupper(*haystack) == toupper(*needle)) {
			/* Matched starting char -- loop through remaining chars */
			const char *h, *n;
			for (h = haystack, n = needle; *h && *n; ++h, ++n) {
				if(toupper(*h) != toupper(*n))
					break;
			}
			if (!*n)	/* matched all of 'needle' to null termination */
				return haystack;	/* return the start of the match */
		}
	}
	return 0;
}

/*
	Generate a random string (for MSRP IDs)
*/
char *random_string(char *buf, size_t size)
{
	long val[4];
	int x;

	for (x = 0; x < 4; x++)
		val[x] = random();
	snprintf(buf, size, "%08lx%08lx%08lx%08lx", val[0], val[1], val[2], val[3]);

	return buf;
}


/*
	Increase the local counter and return it (for unique identifiers)
*/
unsigned long int msrp_new_identifier()
{
	pthread_mutex_lock(&counter_lock);
	counter++;
	pthread_mutex_unlock(&counter_lock);
	return counter;
}
