#!/bin/sh

set -e

MINOR_VERSION=18
INSTALLDIR=/usr/local
export PKG_CONFIG_PATH=$INSTALLDIR/lib/pkgconfig

USAGE=true
BOOTSTRAP=false
UPDATE=false
MAKE_TARGET="optdepend opt"

while [ -n "$1" ]; do
    case "$1" in
    	"--debug" )
    		MAKE_TARGET="all"
    	;;

    	"bootstrap" )
    		USAGE=false
    		BOOTSTRAP=true
    		UPDATE=true
    	;;

    	"update" )
    		USAGE=false
    		UPDATE=true
    	;;
    esac
    shift
done

if $USAGE; then
    echo "usage: $0 [ --debug ] { bootstrap | update }"
    exit 1
fi

cd `dirname $0`
if pwd | grep samples/server; then
    cd ../../..
fi

if $BOOTSTRAP; then
    if which apt > /dev/null 2> /dev/null; then
    	sudo apt install \
    		g++ git make autoconf libpcap-dev libexpat-dev libssl1.0-dev \
    		libsasl2-dev libldap-dev unixodbc-dev liblua5.3-dev libv8-dev \
    		libncurses-dev libsdl2-dev libavformat-dev libswscale-dev \
    		libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev libgsm1-dev \
    		libspeex-dev libopus-dev libavcodec-dev libx264-dev libvpx-dev \
    		libtheora-dev libspandsp-dev capiutils dahdi
    elif which yum > /dev/null 2> /dev/null; then
        sudo yum-config-manager --enable epel
    	sudo yum install \
    		gcc-c++ git make autoconf libpcap-devel expat-devel openssl-devel \
    		cyrus-sasl-devel openldap-devel unixODBC-devel lua-devel v8-devel \
    		ncurses-devel SDL2-devel libavformat-devel libswscale-devel \
    		gstreamer1.0-devel gstreamer-plugins-base1.0-devel gsm-devel \
    		speex-devel opus-devel avcodec-devel x264-devel libvpx-devel \
    		theora-devel libspandsp-devel capiutils dahdi
    else
    	echo "What OS is this? No apt or yum!"
    	exit 1
    fi

    if [ -n $SOURCEFORGE_USERNAME ]; then
        GIT_PATH=ssh://$SOURCEFORGE_USERNAME@git.code.sf.net/p/opalvoip
    else
        GIT_PATH=git://git.code.sf.net/p/opalvoip
    fi

    echo "========================================================================"
    git clone -b v2_$MINOR_VERSION $GIT_PATH/ptlib
    echo "========================================================================"
    echo "Configuring PTLib to install to $INSTALLDIR"
    cd ptlib
    make "CONFIG_PARMS=--prefix=$INSTALLDIR" config
    git checkout configure
    cd ..

    git clone -b v3_$MINOR_VERSION $GIT_PATH/opal
    echo "========================================================================"
    echo "Configuring OPAL to install to $INSTALLDIR"
    cd opal
    make "CONFIG_PARMS=--prefix=$INSTALLDIR" config
    git checkout configure plugins/configure
    cd ..
fi

if $UPDATE; then
    cd ptlib
    git pull --rebase
    make $MAKE_TARGET
    echo "----------------------------------------"
    sudo -E make install
    echo "========================================================================"
    cd ../opal
    git pull --rebase
    make $MAKE_TARGET
    echo "----------------------------------------"
    sudo -E make install
    echo "========================================================================"
    cd samples/server
    make $MAKE_TARGET
    echo "----------------------------------------"
    sudo -E make install
    echo "========================================================================"
    echo "Done"
fi
