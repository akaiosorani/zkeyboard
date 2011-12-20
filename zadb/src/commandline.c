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
#include <errno.h>

#include "sysdeps.h"

#ifdef HAVE_TERMIO_H
#include <termios.h>
#endif

#define  TRACE_TAG  TRACE_ADB
#include "zadb.h"

typedef void (*WRITE_FUNC) (char*, int);

void version(FILE * out) {
    fprintf(out, "Android Debug Bridge for Zaurus version %d.%d.%d\n",
         ADB_VERSION_MAJOR, ADB_VERSION_MINOR, ADB_SERVER_VERSION);
}

void help()
{
    version(stderr);

    fprintf(stderr,
        "\n"
        "device commands:\n"
        "  zadb shell                    - run remote shell interactively\n"
        "  zadb shell <command>          - run remote shell command\n"
        "\n"
        "  zadb keyboard                          - Do as a devices keyboard\n"
        "  zadb logcat [ <filter-spec> ] - View device log\n"
        "\n"
        "  zadb help                     - show this help message\n"
        "  zadb version                  - show version num\n"
        "\n"
        );
}

int usage()
{
    help();
    return 1;
}

#ifdef HAVE_TERMIO_H
static struct termios tio_save;

static void stdin_raw_init(int fd)
{
    struct termios tio;

    if(tcgetattr(fd, &tio)) return;
    if(tcgetattr(fd, &tio_save)) return;

    tio.c_lflag = 0; /* disable CANON, ECHO*, etc */

        /* no timeout but request at least one character per read */
    tio.c_cc[VTIME] = 0;
    tio.c_cc[VMIN] = 1;

    tcsetattr(fd, TCSANOW, &tio);
    tcflush(fd, TCIFLUSH);
}

static void stdin_raw_restore(int fd)
{
    tcsetattr(fd, TCSANOW, &tio_save);
    tcflush(fd, TCIFLUSH);
}
#endif

static void read_and_dump()
{
    char buf[4096];
    int len;

    FILE * file = stdout;
    apacket *p;
    for(;;) {
        usleep(50000);
        p = shift_received_packet();
        if (!p) {
            continue;
        }
        switch(p->msg.command){
            case A_OKAY:
                // OKAYなら表示して続く
                break;
            case A_CLSE:
                // CLOSEなら終了
                return;
            case A_WRTE:
                if (file) {
                    strncpy(buf, p->data, p->msg.data_length);
                    buf[p->msg.data_length] = 0;
                    fprintf(file, buf);
                    fflush(file);
                }
                break;
            default:
                break;
        }

/*
        len = adb_read(fd, buf, 4096);
        if(len == 0) {
            break;
        }

        if(len < 0) {
            if(errno == EINTR) continue;
            break;
        }
        fwrite(buf, 1, len, stdout);
        fflush(stdout);
*/
        
    }
}

static void *stdin_read_thread(void *x)
{
    int fdi;
    unsigned char buf[1024];
    int r, n;
    int state = 0;

    WRITE_FUNC fn;

    fn = ((WRITE_FUNC*)x)[0];
    fdi = ((int*)x)[1];
    free(x);

    for(;;) {
        /* fdi is really the client's stdin, so use read, not adb_read here */
        r = read(fdi, buf, 1000);
        if(r == 0) break;
        if(r < 0) {
            if(errno == EINTR) continue;
            break;
        }
        buf[r] = '\0';
        for(n = 0; n < r; n++){
            switch(buf[n]) {
            case '\n':
                state = 1;
                break;
            case '\r':
                state = 1;
                break;
            case '~':
                if(state == 1) state++;
                break;
            case '.':
                if(state == 2) {
                    fprintf(stderr,"\n* disconnect *\n");
#ifdef HAVE_TERMIO_H
                    stdin_raw_restore(fdi);
#endif
                    exit(0);
                }
            default:
                state = 0;
            }
        }
/*
fprintf(stderr, "read:%d %s\n", r, buf);
int i;
for(i=0;i<r;i++){
  fprintf(stderr, "%x %c %d ", buf[i], buf[i], isprint(buf[i]));
}
*/
        fn(buf, r);
    }
    return 0;
}

static void write_to_device(char* buf, int length)
{
    if (length > 0) {
        send_write(transport, buf);
    }
}

int interactive_shell(void)
{
    adb_thread_t thr;
    int fdi;
    int *fds;

    send_open(transport, "shell: ");
    fdi = 0; //dup(0);

    fds = malloc(sizeof(void) * 2);
    fds[0] = &(write_to_device);
    fds[1] = fdi;

#ifdef HAVE_TERMIO_H
    stdin_raw_init(fdi);
#endif
    adb_thread_create(&thr, stdin_read_thread, fds);
    read_and_dump();
#ifdef HAVE_TERMIO_H
    stdin_raw_restore(fdi);
#endif
    return 0;
}

/** duplicate string and quote all \ " ( ) chars + space character. */
static char *
dupAndQuote(const char *s)
{
    const char *ts;
    size_t alloc_len;
    char *ret;
    char *dest;

    ts = s;

    alloc_len = 0;

    for( ;*ts != '\0'; ts++) {
        alloc_len++;
        if (*ts == ' ' || *ts == '"' || *ts == '\\' || *ts == '(' || *ts == ')') {
            alloc_len++;
        }
    }

    ret = (char *)malloc(alloc_len + 1);

    ts = s;
    dest = ret;

    for ( ;*ts != '\0'; ts++) {
        if (*ts == ' ' || *ts == '"' || *ts == '\\' || *ts == '(' || *ts == ')') {
            *dest++ = '\\';
        }

        *dest++ = *ts;
    }

    *dest++ = '\0';

    return ret;
}

static int send_shellcommand(char* buf)
{
    int fd, ret;

    for(;;) {
        send_open(transport, buf);
/*
        if(fd >= 0)
            break;
        fprintf(stderr,"- waiting for device -\n");
        adb_sleep_ms(1000);
*/
    }

    read_and_dump();
    send_close();
    return ret;
}

static int logcat(int argc, char **argv)
{
    char buf[4096];

    char *log_tags;
    char *quoted_log_tags;

    init_and_wait_device(10, 1);

    log_tags = getenv("ANDROID_LOG_TAGS");
    quoted_log_tags = dupAndQuote(log_tags == NULL ? "" : log_tags);

    snprintf(buf, sizeof(buf),
        "shell:export ANDROID_LOG_TAGS=\"\%s\" ; exec logcat",
        quoted_log_tags);

    free(quoted_log_tags);

    argc -= 1;
    argv += 1;
    while(argc-- > 0) {
        char *quoted;

        quoted = dupAndQuote (*argv++);

        strncat(buf, " ", sizeof(buf)-1);
        strncat(buf, quoted, sizeof(buf)-1);
        free(quoted);
    }

    send_shellcommand(buf);
    return 0;
}

static int send_keyboard_queue(char* buf, int length)
{
    add_keylist(length, buf);
//    dump_keylist();
}

static int keyboard(int argc, char **argv)
{
    if(!init_and_wait_device(10, 1)) {
        return 1;
    }

    adb_thread_t thr;
    int fdi;
    int *fds;

    send_open(transport, "shell: ");
    fdi = 0; //dup(0);

    fds = malloc(sizeof(void) * 2);
    fds[0] = &(send_keyboard_queue);
    fds[1] = fdi;

#ifdef HAVE_TERMIO_H
    stdin_raw_init(fdi);
#endif
    adb_thread_create(&thr, stdin_read_thread, fds);
    for(;;) {
        adb_sleep_ms(200);
        send_command();
    }
#ifdef HAVE_TERMIO_H
    stdin_raw_restore(fdi);
#endif
    return 0;

}
int adb_commandline(int argc, char **argv)
{
    char buf[4096];
    int quote;
    int ret;

    if(argc == 0) {
        help();
        return 0;
    }

    /* "adb /?" is a common idiom under Windows */
    if(!strcmp(argv[0], "help") || !strcmp(argv[0], "/?")) {
        help();
        return 0;
    }

    if(!strcmp(argv[0], "version")) {
        version(stdout);
        return 0;
    }

    if(!strcmp(argv[0], "shell") || !strcmp(argv[0], "hell")) {
        int found;
        int r;
        int fd;

        found = init_and_wait_device(10, 1);
        if (!found) {
            usb_cleanup();
            return 1;
        }

        char h = (argv[0][0] == 'h');

        if (h) {
            printf("\x1b[41;33m");
            fflush(stdout);
        }

        if(argc < 2) {
            r = interactive_shell();
            if (h) {
                printf("\x1b[0m");
                fflush(stdout);
            }
            usb_cleanup();
            return r;
        }

        snprintf(buf, sizeof buf, "shell:%s", argv[1]);
        argc -= 2;
        argv += 2;
        while(argc-- > 0) {
            strcat(buf, " ");

            /* quote empty strings and strings with spaces */
            quote = (**argv == 0 || strchr(*argv, ' '));
            if (quote)
                strcat(buf, "\"");
                strcat(buf, *argv++);
            if (quote)
                strcat(buf, "\"");
        }

        for(;;) {
            send_open(transport, buf);
            read_and_dump();
            send_close();
            r = 0;

            if (h) {
                printf("\x1b[0m");
                fflush(stdout);
            }
            usb_cleanup();
            return r;
        }
    }

    if(!strcmp(argv[0],"keyboard")) {
        ret = keyboard(argc, argv);
        usb_cleanup();
        return ret;
    }

    if(!strcmp(argv[0],"logcat") || !strcmp(argv[0],"lolcat")) {
        ret = logcat(argc, argv);
        usb_cleanup();
        return ret;
    }

    usage();
    return 1;
}


