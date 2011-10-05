#!/bin/sh 

IPKG_BUILD=../../utils/ipkg-build
DESTDIR=../pkg/opt/QtPalmtop/

mkdir $DESTDIR -p

mkdir $DESTDIR/lib
cp -a libusb/.libs/libusb-1.0.so.0 $DESTDIR/lib
cp -a libusb/.libs/libusb-1.0.so.0.0.0 $DESTDIR/lib

mkdir $DESTDIR/share/libusb -p
cp COPYING $DESTDIR/share/libusb/

fakeroot sh $IPKG_BUILD ../pkg

