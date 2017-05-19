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
	sudo rm -rf $OUTPUT >> $LOG
fi

mkdir -p $OUTPUT

cd $SRC >> $LOG
make distclean
./autogen-clean.sh
./autogen.sh

echo "Configuring..."
export CFLAGS="-s -march=armv6 -mfloat-abi=hard -mfpu=vfp"
export CXXFLAGS="${CFLAGS}"

./configure --prefix=/usr/local --enable-shared=yes --enable-static=no >> $LOG

echo "Running make"
make >> $LOG
make DESTDIR=$OUTPUT install >> $LOG

cd $OUTPUT/usr/local

rm -rf include
rm -rf lib/pkgconfig
find lib -name '*\.la' -exec rm {} \;

mkdir etc
mkdir -p share/libts/files
cp -p $OUTPUT/../ts.conf share/libts/files >> $LOG
cp -p $OUTPUT/../pointercal share/libts/files >> $LOG

mkdir tce.installed >> $LOG
cp -p $OUTPUT/../tce.libts tce.installed/libts >> $LOG

sudo chown -Rh root:root $OUTPUT >> $LOG

sudo chown -R tc:staff tce.installed >> $LOG
sudo chmod 755 tce.installed/libts >> $LOG
sudo chown tc:staff share/libts/files/* >> $LOG
sudo chmod 664 share/lirc/files/* >> $LOG

echo "Building tcz"
cd $OUTPUT/.. >> $LOG

if [ -f $TCZ ]; then
	rm $TCZ >> $LOG
fi

mksquashfs $OUTPUT $TCZ >> $LOG
md5sum `basename $TCZ` > ${TCZ}.md5.txt

echo "$TCZ contains"
unsquashfs -ll $TCZ
