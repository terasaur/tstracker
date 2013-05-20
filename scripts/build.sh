#!/bin/bash

if [ ! -d src ]; then
    if [ -d ../src ]; then
        cd ..
    else
        echo "Could not find tracker source directory"
        exit 1
    fi
fi

BOOST_TYPE=system # system|source
if [ "$BOOST_TYPE" = "system" ]; then
    BOOST_VERSION="148"
    BJAM_CMD="/usr/bin/bjam${BOOST_VERSION}"
    export BOOST_INCLUDE_PATH="/usr/include/boost${BOOST_VERSION}"
    export BOOST_LIBRARY_PATH="/usr/lib64/boost${BOOST_VERSION}"
    export BOOST_BUILD_PATH="/usr/share/boost-build"
fi

if [ ! -f ${BJAM_CMD} ]; then
    echo "Unable to find bjam: ${BJAM_CMD}"
    exit 1
fi

# build single target
if [ ! -z $1 ]; then
    if [ "$1" = "clean" ]; then
        CLEAN=1
    else
        if [ "$1" = "debug" ]; then
            BUILD_DEBUG=1
        fi
    fi
else
    CLEAN=0
fi

if [ "$CLEAN" = "1" ]; then
    echo "Running bjam clean"
    ${BJAM_CMD} --clean
    rm -rf bin Debug
    rm -f bindings/python/mpi.c
    exit
fi

BJAM_OPTS="-d2"
BJAM_OPTS="$BJAM_OPTS toolset=gcc-4.4"
#BJAM_OPTS="$BJAM_OPTS gcc"
BJAM_OPTS="$BJAM_OPTS boost=${BOOST_TYPE}"
BJAM_OPTS="$BJAM_OPTS boost-link=shared"
#BJAM_OPTS="$BJAM_OPTS mongodb-link=shared"

if [ $BUILD_DEBUG ]; then
    BJAM_OPTS="$BJAM_OPTS variant=debug"
else
    BJAM_OPTS="$BJAM_OPTS variant=release"
fi

CPPFLAGS=""
LINKFLAGS="linkflags=-L/lib64 linkflags=-L/usr/lib64"
if [ "$BOOST_TYPE" = "system" ]; then
    echo -n ""
    #CPPFLAGS="${CPPFLAGS} cppflags=-I${BOOST_INCLUDE_PATH}"
    LINKFLAGS="${LINKFLAGS} linkflags=-L${BOOST_LIBRARY_PATH}"
else
    echo -n ""
fi

#BJAM_OPTS="$BJAM_OPTS ${CPPFLAGS}"
#BJAM_OPTS="$BJAM_OPTS ${LINKFLAGS}"

#echo ""
#echo "#----------------------------------------------------------------------#"
#echo "Building terasaur tracker..."
#echo ""
${BJAM_CMD} ${BJAM_OPTS}
ERROR=$?
if [ $ERROR -gt 0 ]; then
    exit $ERROR
fi

if [ -f src/tstracker ]; then
    echo "Copying tstracker to root directory"
    cp src/tstracker .
fi

