import sys, traceback

try:
    import arc
    from arc.lrms.common.log import ArcError, error
    from arc.lrms.common.parse import JobDescriptionParserGRAMi
except:
    sys.stderr.write('Failed to import arc or arc.lrms.common module\n')
    sys.exit(2)

def get_lrms_module(lrmsname):
    try:
        return  __import__('arc.lrms.' + lrmsname, fromlist = ['lrms'])
    except:
        error('Failed to import arc.lrms.%s module' % lrmsname, 'pySubmit')
        sys.exit(2)


def main(lrms, grami, conf = "/etc/arc.conf"):
    lrms = get_lrms_module(lrms)
    gridid = grami.split('.')[-2]
    is_parsed = False
    try:
        jds = arc.JobDescriptionList()
        with open(grami, 'r+') as jobdesc:
            content = jobdesc.read()
            is_parsed = JobDescriptionParserGRAMi.Parse(content, jds)
            jd = jds[0]
            localid = lrms.Submit(conf, jd)
            assert(type(localid) == str)
            jobdesc.write('joboption_jobid=%s\n' % localid)
        return 0
    except (ArcError, AssertionError):
        pass
    except IOError:
        error('%s: Failed to access GRAMi file' % gridid, 'pySubmit')
    except Exception:
        error('Unexpected exception:\n%s' % traceback.format_exc(), 'pySubmit')
    return 1


if __name__ == '__main__':

    if len(sys.argv) != 4:
        error('Usage: %s <arc.conf> <lrms> <grami>' % (sys.argv[0]), 'pySubmit')
        sys.exit(1)

    sys.exit(main(sys.argv[2], sys.argv[3], sys.argv[1]))
