/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "zadb.h"
#include "sysdeps.h"

#define  TRACE_TAG   TRACE_ADB

#include <errno.h>

int seed = 0;
int id = -1;
int remote_id = -1;

apacket *received = NULL;
apacket *last = NULL;

ADB_MUTEX_DEFINE(packet_lock);

// adb.c
void fatal(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "error: ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    exit(-1);
}

// adb.c
void fatal_errno(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "error: %s: ", strerror(errno));
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    exit(-1);
}


// adb.c
#if ADB_TRACE_PACKETS
#define DUMPMAX 32
void print_packet(const char *label, apacket *p)
{
    char *tag;
    char *x;
    unsigned count;

    switch(p->msg.command){
    case A_SYNC: tag = "SYNC"; break;
    case A_CNXN: tag = "CNXN" ; break;
    case A_OPEN: tag = "OPEN"; break;
    case A_OKAY: tag = "OKAY"; break;
    case A_CLSE: tag = "CLSE"; break;
    case A_WRTE: tag = "WRTE"; break;
    default: tag = "????"; break;
    }

    fprintf(stderr, "%s: %s %08x %08x %04x \"",
            label, tag, p->msg.arg0, p->msg.arg1, p->msg.data_length);
    count = p->msg.data_length;
    x = (char*) p->data;
    if(count > DUMPMAX) {
        count = DUMPMAX;
        tag = "\n";
    } else {
        tag = "\"\n";
    }
    while(count-- > 0){
        if((*x >= ' ') && (*x < 127)) {
            fputc(*x, stderr);
        } else {
            fputc('.', stderr);
        }
        x++;
    }
    fprintf(stderr, tag);
}
#endif // ADB_TRACE_PACKETS


// adb.c
apacket *get_apacket(void)
{
    apacket *p = malloc(sizeof(apacket));
    if(p == 0) fatal("failed to allocate an apacket");
    memset(p, 0, sizeof(apacket) - MAX_PAYLOAD);
    return p;
}

// adb.c
void put_apacket(apacket *p)
{
    free(p);
}

// adb.c
void handle_packet(apacket *p, atransport *t)
{
    char buf[0x1000 + 8];

    D("handle_packet() %c%c%c%c\n", ((char*) (&(p->msg.command)))[0],
            ((char*) (&(p->msg.command)))[1],
            ((char*) (&(p->msg.command)))[2],
            ((char*) (&(p->msg.command)))[3]);
    print_packet("recv", p);

    switch(p->msg.command){
    case A_SYNC:
        return;

    case A_CNXN: /* CONNECT(version, maxdata, "system-id-string") */
            /* XXX verify version, etc */
/*
        if(t->connection_state != CS_OFFLINE) {
            t->connection_state = CS_OFFLINE;
            handle_offline(t);
        }
        parse_banner((char*) p->data, t);
        handle_online();
*/
        break;

    case A_OPEN: /* OPEN(local-id, 0, "destination") */
//  TODO not support -> drop packet
/*
        if(t->connection_state != CS_OFFLINE) {
            char *name = (char*) p->data;
            name[p->msg.data_length > 0 ? p->msg.data_length - 1 : 0] = 0;
        }
*/
        break;

    case A_OKAY: /* READY(local-id, remote-id, "") */
        if (id == -1 && p->msg.arg1 == seed) {
            /* it's first OKAY reply */
            adb_mutex_lock(&packet_lock);
            id = seed;
            remote_id = p->msg.arg0;
            D("remote_id %d\n", remote_id);
            adb_mutex_unlock(&packet_lock);

            clear_received_packet();
            store_received_packet(p);
            p = NULL;
        } else if(id == p->msg.arg0 && remote_id == p->msg.arg1) {
            // TODO check OKAY

            store_received_packet(p);
            p = NULL;
        }
/*
        if(t->connection_state != CS_OFFLINE) {
        }
*/
        break;

    case A_CLSE: /* CLOSE(local-id, remote-id, "") */
        // TODO handle closing shell
        if (id == p->msg.arg0 && remote_id == p->msg.arg1) {
            adb_mutex_lock(&packet_lock);
            store_received_packet(p);
            p = NULL;
        }
/*
        if(t->connection_state != CS_OFFLINE) {
       }
*/
        break;

    case A_WRTE:
        /*if(t->connection_state != CS_OFFLINE) */{
            if(p->msg.arg0 == remote_id) {
                if(p->msg.data_length > 0) {
/*
                    strncpy(buf, p->data, p->msg.data_length);
                    buf[p->msg.data_length] = 0;
                    fprintf(stdout, buf);
                    fflush(stdout);
*/
                    store_received_packet(p);
                    p = NULL;
                }
                send_ready(t);
            }
        }
/*
*/
        break;

    default:
        printf("handle_packet: what is %08x?!\n", p->msg.command);
    }
    if (p) {
        put_apacket(p);
    }
}


// adb.c
void send_connect(atransport *t)
{
    D("Calling send_connect \n");
    apacket *cp = get_apacket();
    cp->msg.command = A_CNXN;
    cp->msg.arg0 = A_VERSION;
    cp->msg.arg1 = MAX_PAYLOAD;
    snprintf((char*) cp->data, sizeof cp->data, "host::zadb");
    cp->msg.data_length = strlen((char*) cp->data) + 1;
    send_packet(cp, t);
        /* XXX why sleep here? */
    // allow the device some time to respond to the connect message
    adb_sleep_ms(1000);
}

void send_open(atransport *t, const char* msg)
{
    apacket *p = get_apacket();
    p->msg.command = A_OPEN;
    p->msg.arg0 = ++seed;
    p->msg.arg1 = 0;
    snprintf((char*)p->data, sizeof p->data, msg);
//    snprintf((char*)p->data, sizeof p->data, "shell:");
    p->msg.data_length = strlen(p->data);

    send_packet(p, t);
    id = -1;
    remote_id = -1;
}

void send_ready(atransport *t)
{
    apacket *p = get_apacket();
    p->msg.command = A_OKAY;
    p->msg.arg0 = id;
    p->msg.arg1 = remote_id;
    p->msg.data_length = 0;

    send_packet(p, t);
}

void send_close()
{

}

void send_write(atransport *t, const char* msg)
{
    if (remote_id==-1)
    {
        return;
    }
    apacket *p = get_apacket();
    p->msg.command = A_WRTE;
    p->msg.arg0 = 0;
    p->msg.arg1 = remote_id;
    snprintf((char*)p->data, sizeof p->data, msg);
    p->msg.data_length = strlen(p->data);

    send_packet(p, t);
}

void  store_received_packet(apacket *p)
{
    adb_mutex_lock(&packet_lock);
    if (received == NULL) {
        received = p;
        last = p;
    } else {
        last->next = p;
        last = p;
    }
    last->next = NULL;
    adb_mutex_unlock(&packet_lock);
}

apacket* shift_received_packet()
{
    apacket* p = NULL;
    adb_mutex_lock(&packet_lock);
    if (received == NULL) {
        p = NULL;
    } else {
        p = received;
        received = p->next;
        if (!received) {
            last = NULL;
        }
    }
    adb_mutex_unlock(&packet_lock);
    return p;
}

void clear_received_packet()
{
    adb_mutex_lock(&packet_lock);
    while(received != NULL) {
        apacket* packet = received;
        received = received->next;
        put_apacket(packet);
    }
    received = last = NULL;
    adb_mutex_unlock(&packet_lock);
}


