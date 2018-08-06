#!/usr/bin/python2

import sys, traceback

try:
    import arc
    from arc.lrms.common.log import ArcError, error
except:
    sys.stderr.write('Failed to import arc or arc.lrms.common module\n')
    sys.exit(2)

def get_lrms_module(lrmsname):
    try:
        return  __import__('arc.lrms.' + lrmsname, fromlist = ['lrms'])
    except:
        error('Failed to import arc.lrms.%s module' % lrmsname, 'pyCancel')
        sys.exit(2)


def main(lrms, jobid, conf = "/etc/arc.conf"):
    lrms = get_lrms_module(lrms)
    try:
        if lrms.Cancel(conf, jobid):
            return 0
    except Exception:
        error('Unexpected exception:\n%s' % traceback.format_exc(), 'pyCancel')
    except ArcError:
        pass
    return 1


if __name__ == '__main__':

    if len(sys.argv) != 4:
        error('Usage: %s <arc.conf> <lrms> <jobid>' % (sys.argv[0]), 'pyCancel')
        sys.exit(1)

    sys.exit(main(sys.argv[2], sys.argv[3], sys.argv[1]))
