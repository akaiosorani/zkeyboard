

#ifndef __ADB_H
#define __ADB_H

#include <limits.h>

#define MAX_PAYLOAD 4096

#define A_SYNC 0x434e5953
#define A_CNXN 0x4e584e43
#define A_OPEN 0x4e45504f
#define A_OKAY 0x59414b4f
#define A_CLSE 0x45534c43
#define A_WRTE 0x45545257

#define A_VERSION 0x01000000        // ADB protocol version

#define ADB_VERSION_MAJOR 1         // Used for help/version information
#define ADB_VERSION_MINOR 0         // Used for help/version information

#define ADB_SERVER_VERSION    26    // Increment this when we want to force users to start a new adb server

typedef struct amessage amessage;
typedef struct apacket apacket;
typedef struct asocket asocket;
typedef struct alistener alistener;
typedef struct aservice aservice;
typedef struct atransport atransport;
typedef struct adisconnect  adisconnect;
typedef struct usb_handle usb_handle;

struct amessage {
    unsigned command;       /* command identifier constant      */
    unsigned arg0;          /* first argument                   */
    unsigned arg1;          /* second argument                  */
    unsigned data_length;   /* length of payload (0 is allowed) */
    unsigned data_check;    /* checksum of data payload         */
    unsigned magic;         /* command ^ 0xffffffff             */
};

struct apacket
{
    apacket *next;

    unsigned len;
    unsigned char *ptr;

    amessage msg;
    unsigned char data[MAX_PAYLOAD];
};

#define ADB_CLASS              0xff
#define ADB_SUBCLASS           0x42
#define ADB_PROTOCOL           0x1

/* usb host/client interface */
void usb_init();
void usb_cleanup();
int usb_write(usb_handle *h, const void *data, int len);
int usb_read(usb_handle *h, void *data, int len);
int usb_close(usb_handle *h);
void usb_kick(usb_handle *h);

/* used for USB device detection */
int is_adb_interface(int vid, int pid, int usb_class, int usb_subclass, int usb_protocol);

void register_usb_transport(usb_handle *h, const char *serial, unsigned writeable);

/* this should only be used for transports with connection_state == CS_NOPERM */
void unregister_usb_transport(usb_handle *usb);

void init_usb_transport(atransport *t, usb_handle *usb, int state);

struct atransport
{
    atransport *next;
    atransport *prev;

    int (*read_from_remote)(apacket *p, atransport *t);
    int (*write_to_remote)(apacket *p, atransport *t);
    void (*close)(atransport *t);
    void (*kick)(atransport *t);

    int connection_state;

    usb_handle *usb;
    char *serial;

    int          kicked;
};

void fatal(const char *fmt, ...);
void fatal_errno(const char *fmt, ...);

void print_packet(const char *label, apacket *p);

void handle_packet(apacket *p, atransport *t);
void send_packet(apacket *p, atransport *t);

/* packet allocator */
apacket *get_apacket(void);
void put_apacket(apacket *p);

int check_header(apacket *p);
int check_data(apacket *p);

void store_received_packet(apacket* packet);
apacket* shift_received_packet();

#define  ADB_TRACE    1

/* IMPORTANT: if you change the following list, don't
 * forget to update the corresponding 'tags' table in
 * the adb_trace_init() function implemented in adb.c
 */
typedef enum {
    TRACE_ADB = 0,
    TRACE_PACKETS,
    TRACE_TRANSPORT,
    TRACE_USB,
    TRACE_KEY, 
} AdbTrace;

#if ADB_TRACE

int     adb_trace_mask;
void    adb_trace_init(void);

#  define ADB_TRACING  ((adb_trace_mask & (1 << TRACE_TAG)) != 0)

  /* you must define TRACE_TAG before using this macro */
#define  D(...)                                      \
        do {                                           \
            if (ADB_TRACING)                           \
                fprintf(stderr, __VA_ARGS__ );         \
        } while (0)
#else

#  define  D(...)          ((void)0)
#  define  ADB_TRACING     0

#endif // ADB_TRACE

#if !ADB_TRACE_PACKETS
#define print_packet(tag,p) do {} while (0)
#endif // ADB_TRACE_PACKETS

#define CS_ANY       -1
#define CS_OFFLINE    0
#define CS_BOOTLOADER 1
#define CS_DEVICE     2
#define CS_HOST       3
#define CS_RECOVERY   4
#define CS_NOPERM     5 /* Insufficient permissions to communicate with the device */

extern atransport *transport;

#endif

