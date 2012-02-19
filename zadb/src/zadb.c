/*
 * Copyright (C) 2007 The Android Open Source Project
 * Copyright (C) 2011 Akaiosorani 
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
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>

#include "zadb.h"
#include "usb_vendors.h"

#include "sysdeps.h"

#define  TRACE_TAG   TRACE_ADB

#define MESSAGE_DEVICE_INITIALIZING "finding and initializing device...\n"
#define MESSAGE_DEVICE_NOT_FOUND "device not found\n"
#define MESSAGE_DEVICE_CONNECTED "device connected\n"
#define MESSAGE_DEVICE_DISCONNECTED "device disconnected\n"

int device_found;

// adb.c
/* read a comma/space/colum/semi-column separated list of tags
 * from the ADB_TRACE environment variable and build the trace
 * mask from it. note that '1' and 'all' are special cases to
 * enable all tracing
 */
void  adb_trace_init(void)
{
    const char*  p = getenv("ADB_TRACE");
    const char*  q;

    static const struct {
        const char*  tag;
        int           flag;
    } tags[] = {
        { "1", 0 },
        { "all", 0 },
        { "adb", TRACE_ADB },
        { "packets", TRACE_PACKETS },
        { "usb", TRACE_USB },
        { "transport", TRACE_TRANSPORT },
        { "key", TRACE_KEY },
        { NULL, 0 }
    };

    if (p == NULL)
            return;

    /* use a comma/column/semi-colum/space separated list */
    while (*p) {
        int  len, tagn;

        q = strpbrk(p, " ,:;");
        if (q == NULL) {
            q = p + strlen(p);
        }
        len = q - p;

        for (tagn = 0; tags[tagn].tag != NULL; tagn++)
        {
            int  taglen = strlen(tags[tagn].tag);

            if (len == taglen && !memcmp(tags[tagn].tag, p, len) )
            {
                int  flag = tags[tagn].flag;
                if (flag == 0) {
                    adb_trace_mask = ~0;
                    return;
                }
                adb_trace_mask |= (1 << flag);
                break;
            }
        }
        p = q;
        if (*p)
            p++;
    }
}

void message(const char* text)
{
    fprintf(stdout, "%s", text);
    fflush(stdout);
}

static void *device_watch_thread(void *_t)
{
    int current_state;
    current_state = 0;
    for(;;) {
        if (transport && !current_state) {
            D("connected\n");
            send_connect(transport);
            sleep(2);
            device_found = 1;
            current_state = device_found;
            message(MESSAGE_DEVICE_CONNECTED);
        } else if (!transport && current_state) {
            D("disconnected\n");
            device_found = 0;
            current_state = device_found;
            message(MESSAGE_DEVICE_DISCONNECTED);
        }
        sleep(1);

        D("device %d %d %x \n", device_found, current_state, transport);
    }
    return NULL;
}

void create_device_watch_thread()
{
   adb_thread_t device_watch_thread_ptr;
    if(adb_thread_create(&device_watch_thread_ptr, device_watch_thread, NULL)){
        fatal_errno("cannot create device watch thread");
    }
}

int init_and_wait_device(int wait_seconds, int first_time)
{
    message(MESSAGE_DEVICE_INITIALIZING);
    if(first_time) {
        usb_init();
    }

    device_found = 0;
    create_device_watch_thread();

    if (wait_seconds <= 0) {
        wait_seconds = -1;
    }

    while((wait_seconds == -1 ) || (wait_seconds--)) {
        adb_sleep_ms(1000);
        if (device_found) {
            return device_found;
        }
    }

    message(MESSAGE_DEVICE_NOT_FOUND);
    return device_found;
}

int main (int argc, char **argv)
{
    /* init trace setting */
    adb_trace_init();

    /* init usb */
    usb_vendors_init();

    adb_commandline(argc - 1 , argv + 1 );

    return 0;
}

