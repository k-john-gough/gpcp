#!/bin/sh

if [ "$CROOT" != "" ]
then
  echo CROOT already defined: run this once in a new shell
else
  export CROOT=`pwd`
  echo CROOT=$CROOT
  export PATH=$CROOT/bin:$PATH
  echo PATH=$PATH
  export CPSYM=.:$CROOT/symfiles:$CROOT/symfiles/NetSystem
  echo CPSYM=$CPSYM
fi

