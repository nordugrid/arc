'''
This regression test tests whether deleting a Swig iterator obtained
from a ARC C++ public member std::list object generates a
segmentation fault. That issue was reported in bug 2473.
'''

import arc

j = arc.Job()

for i in j.Error:
    continue
