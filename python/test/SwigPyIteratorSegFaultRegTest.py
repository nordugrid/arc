'''
This regression test tests whether looping over ARC C++ public
member std::list objects mapped to a python list raises a
segmentation fault. That issue was reported in bug 2473.
'''

import arc

j = arc.Job()

for i in j.Error:
    continue
