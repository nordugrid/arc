#!/bin/sh
export LDLIBRARY_PATH=/home/szferi/arc1/src/hed/libs/common/.libs/:/home/szferi/arc1/src/hed/libs/data/.libs/:/home/szferi1/arc1/src/hed/libs/loader/.libs/:/home/szferi/arc1/src/hed/libs/message/.libs/
export PYTHONPATH=/home/szferi/arc1/python:/home/szferi/arc1/python/.libs
#gdb /usr/bin/python
 python test.py
