
CC=gcc
ZADBDEF="-DADB_HOST=1 -DADB_TRACE_PACKETS=1"

CFLAGS="-O2 -g -Wall -Wno-unused-parameter -DHAVE_TERMIO_H"
LIBUSBCFLAGS="`pkg-config --cflags libusb-1.0`"
LIBUSBLIBS="`pkg-config --libs libusb-1.0`"
# FIX ME pkg-config not working correctly
LIBUSBLIBS="-L/usr/lib/i386-linux-gnu/ -lusb-1.0"
SOURCES="usb_libusb.c usb_vendors.c transport_usb.c send.c transport_z.c zadb.c commandline.c keycode.c"
LIBS="-lpthread "
OUTFILE="zadb-x86"

rm $OUTFILE

$CC $CFLAGS $ZADBDEF $LIBUSBCFLAGS $SOURCES $LIBUSBLIBS $LIBS -o $OUTFILE
echo "$CC $CFLAGS $ZADBDEF $LIBUSBCFLAGS $SOURCES $LIBUSBLIBS $LIBS -o $OUTFILE"

