
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>

#include "zadb.h"
#include "usb_vendors.h"

#include "sysdeps.h"

#define  TRACE_TAG   TRACE_ADB

#define MESSAGE_DEVICE_CONNECTED "device connected\n"
#define MESSAGE_DEVICE_DISCONNECTED "device disconnected\n"
#define COMMAND_TEXT "shell:input text \0"
#define COMMAND_KEYCODE "shell:input keyevent \0"

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
        { "sockets", TRACE_SOCKETS },
        { "packets", TRACE_PACKETS },
        { "rwx", TRACE_RWX },
        { "usb", TRACE_USB },
        { "sync", TRACE_SYNC },
        { "sysdeps", TRACE_SYSDEPS },
        { "transport", TRACE_TRANSPORT },
        { "jdwp", TRACE_JDWP },
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

void send_text(const char* text)
{
    char p[350];
    strcpy(p, COMMAND_TEXT);
    char *p1 = p + strlen(p);
    int count = 100;
    char* s = text;
    while(count-- && *s) {
        if(*s == ' ') {
            *(p1++) = '%';
            *(p1++) = '%';
            *(p1++) = 's';
            s++;
        } else if (isalnum(*s)) {
            *(p1++) = *s++;
        } else {
            s++;
        }
    }
    *(p1++) = ' ';
    *p1 = '\0';
    D(p);
    send_open(transport, p);
}

void send_keycode(int keycode)
{
    char p[100];
    strcpy(p, COMMAND_KEYCODE);
    char *p1 = p + strlen(p);
    snprintf(p1, sizeof(p) - (p1 - p), " %d ", keycode);
    D(p);
    send_open(transport, p);
}

void parse_command()
{
    char buf[4096];
    int code;

    for(;;)
    {
        if (fgets(buf, sizeof(buf), stdin) == NULL)
            continue;

        if (strchr(buf, '\n')) {
            buf[strlen(buf) -1] = '\0';
        } else {
            while(getchar() != '\n') {};
        }

        if (!device_found)
            continue;

        if (buf[1] != ':')
            continue;

        switch(buf[0]) {
            case 't':
                send_text(&buf[2]);
                usleep(50000);
                break;
            case 'k':
                code = atoi(&buf[2]);
                if (code <= 0 || code > 255)
                    continue;
                send_keycode(code);
                usleep(50000);
                break;
            case 'q':
                return;
                break;
            default:
                break;
        }
    }
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

}

void create_device_watch_thread()
{
   adb_thread_t device_watch_thread_ptr;
    if(adb_thread_create(&device_watch_thread_ptr, device_watch_thread, NULL)){
        fatal_errno("cannot create device watch thread");
    }
}

void message(const char* text)
{
    fprintf(stdout, text);
    fflush(stdout);
}

int main (int argc, char **argv)
{
    adb_trace_init();
    usb_vendors_init();
    usb_init();

    device_found = 0;
    create_device_watch_thread();

    parse_command();

    usb_cleanup();
    return 0;
}

