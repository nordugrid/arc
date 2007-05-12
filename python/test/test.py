#!/usr/bin/env python
import sys
sys.path=sys.path+['../', '../.libs/' ]
import arc
c = arc.Config('../../src/tests/echo/service.xml')
c._print()
