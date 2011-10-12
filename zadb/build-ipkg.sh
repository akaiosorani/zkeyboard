#!/bin/sh 

IPKG_BUILD=../utils/ipkg-build
PKGDIR=./pkg
DESTDIR=${PKGDIR}/opt/QtPalmtop/
BINDIR=${DESTDIR}/bin
BINARY=src/zadb

strip $BINARY

mkdir $BINDIR -p

cp -a $BINARY $BINDIR/

sh $IPKG_BUILD $PKGDIR

