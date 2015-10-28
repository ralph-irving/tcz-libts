#!/bin/sh

TS=libts
TSVERSION=1.0
SRC=${TS}-$TSVERSION
LOG=$PWD/config.log
OUTPUT=$PWD/${TS}-build
TCZ=${TS}.tcz

# Build requires these extra packages in addition to the raspbian 7.6 build tools
# sudo apt-get install squashfs-tools bsdtar

## Start
echo "Most log mesages sent to $LOG... only 'errors' displayed here"
date > $LOG

## Build
echo "Cleaning up..."

if [ -d $OUTPUT ]; then
	rm -rf $OUTPUT >> $LOG
fi

mkdir -p $OUTPUT

cd $SRC >> $LOG
make distclean

echo "Configuring..."
export CFLAGS="-s -O3 -march=armv6 -mfloat-abi=hard -mfpu=vfp"
export CXXFLAGS="${CFLAGS}"

./configure --prefix=/usr/local --enable-shared=yes --enable-static=no >> $LOG

echo "Running make"
make >> $LOG
make DESTDIR=$OUTPUT install >> $LOG

cd $OUTPUT/usr/local
rm -rf include
rm -rf lib/pkgconfig
find lib -name '*\.la' -exec rm {} \;
cp -p $OUTPUT/../ts.conf etc

echo "Building tcz"
cd $OUTPUT/.. >> $LOG

if [ -f $TCZ ]; then
	rm $TCZ >> $LOG
fi

mksquashfs $OUTPUT $TCZ -all-root >> $LOG
md5sum `basename $TCZ` > ${TCZ}.md5.txt

echo "$TCZ contains"
unsquashfs -ll $TCZ
