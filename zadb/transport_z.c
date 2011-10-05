

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sysdeps.h"
ADB_MUTEX_DEFINE( transport_lock );

#define   TRACE_TAG  TRACE_TRANSPORT

#include "zadb.h"

atransport *transport;

// transport.c
void send_packet(apacket *p, atransport *t)
{
    unsigned char *x;
    unsigned sum;
    unsigned count;

    p->msg.magic = p->msg.command ^ 0xffffffff;

    count = p->msg.data_length;
    x = (unsigned char *) p->data;
    sum = 0;
    while(count-- > 0){
        sum += *x++;
    }
    p->msg.data_check = sum;

    print_packet("send", p);

    if (t == NULL) {
        fatal_errno("Transport is null");
        D("Transport is null \n");
    }

    // TODO
    t->write_to_remote(p, t);
    put_apacket(p);
}

void
kick_transport(atransport*  t)
{
    if (t && !t->kicked)
    {
        int  kicked;

        adb_mutex_lock(&transport_lock);
        kicked = t->kicked;
        if (!kicked)
            t->kicked = 1;
        adb_mutex_unlock(&transport_lock);

        if (!kicked)
            t->kick(t);
    }
}

static void *output_thread(void *_t)
{
    atransport *t = _t;
    apacket *p;

    D("%s: data pump started\n", t->serial);
    for(;;) {
        p = get_apacket();

        if(t->read_from_remote(p, t) == 0){
            D("%s: received remote packet, sending to transport\n",
              t->serial);
            handle_packet(p, t);
        } else {
            D("%s: remote read failed for transport\n", t->serial);
            put_apacket(p);
            break;
        }
    }

oops:
    D("%s: transport output thread is exiting\n", t->serial);
    kick_transport(t);
//    transport_unref(t);
    return 0;
}


static void *input_thread(void *_t)
{
    atransport *t = _t;
    apacket *p;
    int active = 0;

    D("%s: starting transport input thread\n",
       t->serial);

    for(;;){
        // TODO get P
        if(active) {
            D("%s: transport got packet, sending to remote\n", t->serial);
             t->write_to_remote(p, t);
        } else {
            D("%s: transport ignoring packet while offline\n", t->serial);
        }

        put_apacket(p);
    }

    D("%s: transport input thread is exiting\n", t->serial);
    kick_transport(t);
//    transport_unref(t);
    return 0;
}

static void create_output_thread(atransport *t)
{
   adb_thread_t output_thread_ptr;
    if(adb_thread_create(&output_thread_ptr, output_thread, t)){
        fatal_errno("cannot create output thread");
    }

}

// transport.c
void register_usb_transport(usb_handle *usb, const char *serial, unsigned writeable)
{
    D("register");
//    adb_mutex_lock(&transport_lock);

    atransport *t = calloc(1, sizeof(atransport));
    D("transport: %p init'ing for usb_handle %p (sn='%s')\n", t, usb,
      serial ? serial : "");
    init_usb_transport(t, usb, (writeable ? CS_OFFLINE : CS_NOPERM));
    if(serial) {
        t->serial = strdup(serial);
    }
    transport = t;

    create_output_thread(t);

}

// transport.c
/* this should only be used for transports with connection_state == CS_NOPERM */
void unregister_usb_transport(usb_handle *usb)
{
    D("unregister");
    adb_mutex_lock(&transport_lock);
//    if(transport && transport->usb == usb && transport->connection_state == CS_NOPERM)
    if(transport && transport->usb == usb)
    {
        transport = NULL;
    }
    adb_mutex_unlock(&transport_lock);
}

// transport.c
int check_header(apacket *p)
{
    if(p->msg.magic != (p->msg.command ^ 0xffffffff)) {
        D("check_header(): invalid magic\n");
        return -1;
    }

    if(p->msg.data_length > MAX_PAYLOAD) {
        D("check_header(): %d > MAX_PAYLOAD\n", p->msg.data_length);
        return -1;
    }

    return 0;
}

// transport.c
int check_data(apacket *p)
{
    unsigned count, sum;
    unsigned char *x;

    count = p->msg.data_length;
    x = p->data;
    sum = 0;
    while(count-- > 0) {
        sum += *x++;
    }

    if(sum != p->msg.data_check) {
        return -1;
    } else {
        return 0;
    }
}


