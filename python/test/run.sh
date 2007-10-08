#!/bin/sh
export LDLIBRARY_PATH=../../src/hed/libs/common/.libs/:../../src/hed/libs/data/.libs/:../../src/hed/libs/loader/.libs/:../../src/hed/libs/message/.libs/
export PYTHONPATH=..:../.libs
python test.py
