#include <stdio.h>
#include <ctype.h>

#include "sysdeps.h"
#include "zadb.h"

ADB_MUTEX_DEFINE(keyevent_lock);

typedef struct keydata keydata;
struct keydata {
    int type;
    int count;
    int value;
    char text[100];
    keydata* next;
};

static int KEY_EVENT = 0;
static int KEY_TEXT = 1;

static keydata* keylist = NULL;
static keydata* last = NULL;

static void add(keydata *k)
{
    if (!keylist) 
    {
        keylist = k;
        last = k;
    } else {
        last->next = k;
        last = k;
    }
}

static keydata* get()
{
    keydata* k = malloc(sizeof(keydata));
    bzero(k, sizeof(keydata));
    add(k);
    return k;
}

static keydata* set_key_value(int value)
{
    keydata* k = get();
    k->type = KEY_EVENT;
    k->value = value;
    k->count++;
    return k;
}

static keydata* check_1byte_char(char *buf)
{
    int value;
    switch(buf[0])
    {
        case 0x09:
            value = 61;
            break;
        case 0x0a:
            value = 66;
            break;
        case 0x1b:
            value = 111;
            break;
        case 0x7f:
            value = 67;
            break;
        default:
            return NULL;
    }
    return set_key_value(value);
}

static keydata* check_3bytes_char(char *buf)
{
    if (buf[0] != 0x1b || buf[1] != 0x5b)
    {
        return NULL;
    }
    int value;
    switch(buf[2])
    {
        case 0x41:
            value = 19;
            break;
        case 0x42:
            value = 20;
            break;
        case 0x43:
            value = 22;
            break;
        case 0x44:
            value = 21;
            break;
        default:
            return NULL;
    }
    return set_key_value(value);
}

static keydata* check_4bytes_char(char *buf)
{
    if(buf[0] == 0x1b && buf[1] == 0x5b &&
       buf[2] == 0x33 && buf[3] == 0x7e )
    {
        // DEL 
        return set_key_value(112);
    }
    return NULL;
}

void add_keylist(int length, char* buf)
{
    keydata *k = NULL;
    /* printable char */
    adb_mutex_lock(&keyevent_lock);
    if (length == 1 && isprint(buf[0])) {
        if (last && last->type == KEY_TEXT && last->count < sizeof(last->text) && last->text[last->count-1] != '%') 
        {
            k = last;
        }
        if (!k)
        {
            k = get();
        }
        k->type = KEY_TEXT;
        k->text[k->count] = buf[0];
        k->count++;
    } else {
        switch(length)
        {
            case 1: /* enter,tab,bs,esc */
                k = check_1byte_char(buf);
                break;
            case 3: /* up,down,right,left */
                k = check_3bytes_char(buf);
                break;
            case 4: /* del */
                k = check_4bytes_char(buf);
                break;
            default:
                return;
        }
    }
    adb_mutex_unlock(&keyevent_lock);
}

static void set_keyevent(keydata* k)
{
    char buf[40];
    sprintf(buf, "input keyevent %d \0", k->value);

    /* send packet */
    send_write(transport, buf);
    fprintf(stderr, "%s\n", buf);
}

static void set_text(keydata* k)
{
    int i;
    char* p;
    char buf[240];
    sprintf(buf, "input text \"");
    p = &buf[12];
    for(i=0;i<k->count;i++)
    {
        if(k->text[i] == '"')
        {
            *(p++) = '\\';
        }
        *(p++) = k->text[i];
    }
    *p = '"';
    *(p+1) = ' '; 
    *(p+2) = '\0'; 

    /* send packet */
    send_write(transport, buf);
    fprintf(stderr, "%s\n", buf);
}

void send_command()
{
    adb_mutex_lock(&keyevent_lock);
    keydata* k = keylist;
    keylist = keylist->next;
    if(!keylist)
    {
        last = NULL;
    }
    adb_mutex_unlock(&keyevent_lock);

    if(!k) 
    {
        return;
    }
    if (k->type == KEY_EVENT)
    {
        set_keyevent(k);
    } else {
        set_text(k);
    }
}

void dump_keylist()
{
    adb_mutex_lock(&keyevent_lock);
    keydata* k = keylist;
    if(!k) return;

    while(k) {
        if (k->type == KEY_EVENT)
        {
            set_keyevent(k);
        } else {
            set_text(k);
        }
        k = k->next;
    }
    adb_mutex_unlock(&keyevent_lock);
}


