#!/bin/bash

if test ! -f "${GM_KICK_TEST_FILE}"; then
  GM_KICK_TEST_FILE=gm_kick_test_file
fi

printf "%s\n" "$@" >> ${GM_KICK_TEST_FILE}
