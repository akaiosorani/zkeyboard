
CC=gcc
ZADBDEF="-DADB_HOST=1 -DADB_TRACE_PACKETS=1 -DHAVE_TERMIO_H=1"

CFLAGS="-O2 -g -Wall -Wno-unused-parameter"
LIBUSBCFLAGS="`pkg-config --cflags  --libs libusb-1.0`"
#LIBUSBCFLAGS="-I. -L."
SOURCES="usb_libusb.c usb_vendors.c transport_usb.c send.c transport_z.c zadb.c commandline.c keycode.c"
LIBS="-lpthread "
OUTFILE="zadb-x86"

rm $OUTFILE

$CC $CFLAGS $ZADBDEF $LIBUSBCFLAGS $SOURCES $LIBS -o $OUTFILE

