#ifndef _MSRP_UTILS_H
#define _MSRP_UTILS_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "msrp_session.h"
#include "msrp_message.h"
#include "msrp_relay.h"
#include "msrp_switch.h"
#include "msrp_callback.h"
#include "msrp_network.h"


int msrp_exists;


/*
	Helpers
*/
const char *stristr(const char *haystack, const char *needle);
char *random_string(char *buf, size_t size);


/*
	Local identifier counter
*/
unsigned long int counter;
pthread_mutex_t counter_lock;
unsigned long int msrp_new_identifier(void);


/*
	Linkeds list management
*/
struct msrp_session *sessions;		/* The sessions linked list */
pthread_mutex_t sessions_lock;		/* A lock to access the list */

struct msrp_context *contexts;		/* The contexts linked list */
pthread_mutex_t contexts_lock;		/* A lock to access the list */

struct msrp_relay *relays;		/* The relays linked list */
pthread_mutex_t relays_lock;		/* A lock to access the list */

struct msrp_conference *switches;	/* The switches (conference rooms) linked list */
pthread_mutex_t switches_lock;		/* A lock to access the list */


/*
	Macros for linked lists management
*/
#define MSRP_LIST_SETUP(list, lock)					\
	(list) = NULL;							\
	pthread_mutex_init(&(lock), NULL);


#define MSRP_LIST_ADD(list, lock, element, failure)			\
	if(!(element))							\
		return (failure);					\
	if(!(list))	/* First add */					\
		(list) = (element);					\
	else {								\
		/* First of all look if such element already exists */	\
		pthread_mutex_lock(&(lock));				\
		typeof(element) temp = NULL, previous = NULL;		\
		temp = (list);						\
		if(temp) {						\
			while(temp) {					\
				if(temp == (element))			\
					break;				\
				previous = temp;			\
				temp = temp->next;			\
			}						\
		}							\
		if(temp) {	/* element already is in the list */	\
			pthread_mutex_unlock(&(lock));			\
			return (failure);				\
		}							\
		/* Append the element at the end of the list */		\
		previous->next = (element);				\
		pthread_mutex_unlock(&(lock));				\
	}


#define MSRP_LIST_REMOVE(list, lock, element)				\
	if(!(element) || !(list))					\
		return -1;						\
	pthread_mutex_lock(&(lock));					\
	/* Look for the element in the list */				\
	typeof(element) temp = NULL, previous = NULL;			\
	temp = (list);							\
	while(temp) {							\
		if(temp == (element)) {					\
			if(!previous)	/* First in the list */		\
				(list) = temp->next;			\
			else		/* Not first in the list */	\
				previous->next = temp->next;		\
			break;						\
		}							\
		previous = temp;					\
		temp = temp->next;					\
	}								\
	pthread_mutex_unlock(&(lock));


#define MSRP_LIST_FREE(list, lock)					\
	if(!(list))	/* No elements to remove */			\
		return 0;						\
	else {								\
		typeof((list)) temp = list, next = NULL;		\
		while(temp) {						\
			next = temp->next;				\
			MSRP_LIST_REMOVE((list), (lock), temp);		\
			temp = next;					\
		}							\
	}


#define MSRP_LIST_CHECK(list, failure)					\
	if(!(list))	/* List doesn't exist */			\
		return (failure);


#define MSRP_LIST_CROSS(list, lock, element)				\
	pthread_mutex_lock(&(lock));					\
	/* Look for the element in the list */				\
	(element) = (list);						\
	while((element)) {						\
		if(!(element))						\
			break;


#define MSRP_LIST_STEP(list, lock, element)				\
		(element) = (element)->next;				\
	}								\
	pthread_mutex_unlock(&(lock));


#define MSRP_LIST_LOCK(list, lock)					\
	pthread_mutex_lock(&(lock));


#define MSRP_LIST_UNLOCK(list, lock)					\
	pthread_mutex_unlock(&(lock));


#endif
