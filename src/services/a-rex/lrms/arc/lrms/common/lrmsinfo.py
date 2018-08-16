import re

class LRMSInfo(object):

    _split = re.compile(r' (?=[^ =]+=)')
    opts_schema = {
        'remote_host' : '*',
        'remote_user' : '*',
        'private_key' : '*',
        'ssh_timeout' : '*',
        }

    def __init__(self, options):
        self.lrms_info = {'queues':{},
                          'nodes' :{},
                          'jobs'  :{}}

        self._ssh = False
        if 'remote_host' in options:
            from arc.lrms.common.config import Config
            from arc.lrms.common.ssh import ssh_connect

            tcp_window = (2 << 30) - 1
            ssh_connect(options['remote_host'], options['remote_user'], options['private_key'], tcp_window)
            self._ssh = True
            self._remote_user = options['remote_user']
            Config.ssh_timeout = int(options['ssh_timeout']) if 'ssh_timeout' in options else 10

    '''
    Convert human readable number to integer.
    '''
    @staticmethod
    def parse_number(value):
        if value[-1].isdigit():
            return int(value)
        try:
            units = ['K', 'M', 'G', 'T']
            return int(value[:-1]) << 10*(1 + units.index(value[-1]))
        except:
            return 0

    @staticmethod
    def split(string):
        return LRMSInfo._split.split(string)

    @staticmethod
    def get_lrms_options_schema(**opts):
        opts.update(LRMSInfo.opts_schema)
        return opts
