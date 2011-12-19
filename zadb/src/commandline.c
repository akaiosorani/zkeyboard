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

/*
dummy
*/

int adb_connect(char* buf) { return 0; }
int adb_read(int  fd, void* buf, int len) { return 0; }
int adb_close(int fd) { return 0;}
int adb_write(int  fd, const void*  buf, int  len) { return 0;}
const char *adb_error(void) {return NULL;}

enum {
    IGNORE_DATA,
    WIPE_DATA,
    FLASH_DATA
};

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

static void read_and_dump(int fd)
{
    char buf[4096];
    int len;

    FILE * file = stdout;
    apacket *p;
    while(fd >= 0) {
        usleep(50000);
        p = shift_received_packet();
        if (!p) {
            break;
        }
        switch(p->msg.command){
            case A_OKAY:
                // OKAYなら表示して続く
                break;
            case A_CLSE:
                // CLOSEなら終了
                break;
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
    int fd, fdi;
    unsigned char buf[1024];
    int r, n;
    int state = 0;

    int *fds = (int*) x;
    fd = fds[0];
    fdi = fds[1];
    free(fds);

    for(;;) {
        /* fdi is really the client's stdin, so use read, not adb_read here */
        r = read(fdi, buf, 1024);
        if(r == 0) break;
        if(r < 0) {
            if(errno == EINTR) continue;
            break;
        }
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
fprintf(stderr, "read:%d ", r, buf);
int i;
for(i=0;i<r;i++){
  fprintf(stderr, "%x %c %d ", buf[i], buf[i], isprint(buf[i]));
}
fprintf(stderr, "\n");
    add_keylist(r, buf);
    dump_keylist();
*/
/*
        r = adb_write(fd, buf, r);
        if(r <= 0) {
            break;
        }
*/
    }
    return 0;
}

int interactive_shell(void)
{
    adb_thread_t thr;
    int fdi, fd;
    int *fds;

    fd = adb_connect("shell:");
    if(fd < 0) {
        fprintf(stderr,"error: %s\n", adb_error());
        return 1;
    }
    fdi = 0; //dup(0);

    fds = malloc(sizeof(int) * 2);
    fds[0] = fd;
    fds[1] = fdi;

#ifdef HAVE_TERMIO_H
    stdin_raw_init(fdi);
#endif
    adb_thread_create(&thr, stdin_read_thread, fds);
    read_and_dump(fd);
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
        fd = adb_connect(buf);
        if(fd >= 0)
            break;
        fprintf(stderr,"- waiting for device -\n");
        adb_sleep_ms(1000);
    }

    read_and_dump(fd);
    ret = adb_close(fd);
    if (ret)
        perror("close");

    return ret;
}

static int logcat(int argc, char **argv)
{
    char buf[4096];

    char *log_tags;
    char *quoted_log_tags;

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

int adb_commandline(int argc, char **argv)
{
    char buf[4096];
    int no_daemon = 0;
    int is_daemon = 0;
    int is_server = 0;
    int persist = 0;
    int r;
    int quote;

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
        int r;
        int fd;

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
            fd = adb_connect(buf);
            if(fd >= 0) {
                read_and_dump(fd);
                adb_close(fd);
                r = 0;
            } else {
                fprintf(stderr,"error: %s\n", adb_error());
                r = -1;
            }
        }
    }

    if(!strcmp(argv[0],"logcat") || !strcmp(argv[0],"lolcat")) {
        return logcat(argc, argv);
    }

    usage();
    return 1;
}


