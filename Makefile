#Makefile for UCT IMS Client version 1.0
#
#Compiler: gcc 4.0.3


CC = gcc
# LIBS = -L/usr/local/lib -leXosip2 -lxml2 -lcurl -lgstinterfaces-0.10
LIBS = -L/usr/local/lib -leXosip2 -losip2 -losipparser2 -lgstinterfaces-0.10

INCLUDES = -I/usr/include/libxml2
GTK_LIB_FLAGS = `pkg-config gtk+-3.0 libglade-2.0 --cflags --libs`

GTK_INC_FLAGS = `pkg-config --cflags gtk+-3.0 gstreamer-0.10`

ODIR=obj

#_OBJS = callbacks.o common_exosip_event_handler.o common_interface_event_handler.o DigestAKAv1MD5.o ims_exosip_event_handler.o \
ims_interface_event_handler.o main.o media.o preferences.o presence.o \
sdp_methods.o sound_conv.o support.o useful_methods.o watchers.o xcap.o gstreamer.o uuid.o sysdep.o rtsp.o \
msrp.o msrp_callback.o msrp_message.o msrp_network.o msrp_relay.o msrp_session.o msrp_switch.o msrp_utils.o endpointmsrp.o

_OBJS = main.o preferences.o support.o callbacks.o ims_interface_event_handler.o ims_exosip_event_handler.o useful_methods.o \
DigestAKAv1MD5.o sdp_methods.o common_exosip_event_handler.o media.o gstreamer.o common_interface_event_handler.o uuid.o sysdep.o \
msrp.o msrp_callback.o msrp_message.o msrp_network.o msrp_relay.o msrp_session.o msrp_switch.o msrp_utils.o endpointmsrp.o

OBJS = $(patsubst %,$(ODIR)/%,$(_OBJS))

SRC_DIR=src

#SRCS = main.c callbacks.c common_exosip_event_handler.c common_interface_event_handler.c DigestAKAv1MD5.c ims_exosip_event_handler.c \
ims_interface_event_handler.c media.c preferences.c presence.c \
sdp_methods.c sound_conv.c support.c useful_methods.c watchers.c xcap.c gstreamer.c uuid.c sysdep.c rtsp.c \
msrp.o msrp_callback.c msrp.message.c msrp_network.c msrp_relay.c msrp_session.c msrp_switch.c msrp_utils.c endpointmsrp.c

SRCS = main.c preferences.c support.c callbacks.c ims_interface_event_handler.c ims_exosip_event_handler.c useful_methods.c \
DigestAKAv1MD5.c sdp_methods.c common_exosip_event_handler.c media.c gstreamer.c common_interface_event_handler.c uuid.c sysdep.c \
msrp.o msrp_callback.c msrp.message.c msrp_network.c msrp_relay.c msrp_session.c msrp_switch.c msrp_utils.c endpointmsrp.c


PROG = uctimsclient

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) -export-dynamic -o $@ $(OBJS) $(LIBS) $(GTK_LIB_FLAGS)

$(ODIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(INCLUDES) $(GTK_INC_FLAGS) -o $@ -c $<

depend: $(SRCS)
	makedepend $(SRCS)

clean: 
	rm -f $(PROG) $(ODIR)/*.o *~ $(SRC_DIR)/*~

# DO NOT DELETE THIS LINE --
