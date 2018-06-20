#!/bin/bash

if test ! -f "${GM_KICK_TEST_FILE}"; then
  GM_KICK_TEST_FILE=gm_kick_test_file
fi

isid='0'
for arg in $@; do
  if [ "$isid" = '0' ] ; then
    if [ "$arg" = "-j" ] ; then
      isid='1'
    fi
  else
    echo "$arg" >> ${GM_KICK_TEST_FILE}
    isid='0'
  fi
done
