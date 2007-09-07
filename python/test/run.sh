#!/bin/sh
ROOT=/home/szferi/Projects/knowarc/arc1
export LDLIBRARY_PATH=$ROOT/src/libs/common/.libs/:$ROOT/src/libs/data/.libs/:$ROOT/src/hed/libs/loader/.libs/:$ROOT/src/hed/libs/message/.libs/
export PYTHONPATH=$ROOT/python:$ROOT/python/.libs
# gdb /usr/bin/python
python test.py
