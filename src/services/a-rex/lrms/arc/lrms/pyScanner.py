import sys, time, traceback

try:
    time.sleep(10)
    import arc
    from arc.lrms.common.log import ArcError, error
except:
    sys.stderr.write('Failed to import arc or arc.lrms.common module\n')
    sys.exit(2)

def get_lrms_module(lrmsname):
    try:
        return  __import__('arc.lrms.' + lrmsname, fromlist = ['lrms'])
    except:
        error('Failed to import arc.lrms.%s module' % lrmsname, 'pyScanner')
        sys.exit(2)


def main(lrms, control_dirs, conf = "/etc/arc.conf"):
    lrms = get_lrms_module(lrms)
    try:
        lrms.Scan(conf, control_dirs)
        return 0
    except (ArcError, AssertionError):
        pass
    except Exception:
        error('Unexpected exception:\n%s' % traceback.format_exc(), 'pyScanner')
    return 1


if __name__ == '__main__':

    if len(sys.argv) != 4:
        error('Usage: %s <arc.conf> <lrms> <control directories ... >' % (sys.argv[0]), 'pyScanner')
        sys.exit(1)

    sys.exit(main(sys.argv[2], sys.argv[3:], sys.argv[1]))
