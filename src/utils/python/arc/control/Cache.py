from __future__ import print_function
from __future__ import absolute_import

from .ControlCommon import *
import sys
import os
import subprocess
import hashlib


class CacheControl(ComponentControl):
    def __init__(self, arcconfig):
        self.logger = logging.getLogger('ARCCTL.Cache')

        # config is mandatory
        if arcconfig is None:
            self.logger.error('Failed to get parsed arc.conf. Cache control is not possible.')
            sys.exit(1)
        # cache should be enabled
        if not arcconfig.check_blocks(['arex/cache']):
            self.logger.critical('A-REX cache is not enabled in arc.conf.')
            sys.exit(1)
        # find the cachedirs
        self.cache_dirs = arcconfig.get_value('cachedir', 'arex/cache', force_list=True)
        if not self.cache_dirs:
            self.logger.error('Failed to get cache directories from arc.conf.')
            sys.exit(1)
        self.logger.debug('Following cache locations found: %s', ','.join(self.cache_dirs))

    def stats(self):
        stats_cmd = ARC_LIBEXEC_DIR + '/cache-clean -s ' + ' '.join(self.cache_dirs)
        subprocess.call(stats_cmd.split(' '))
        pass

    @staticmethod
    def __get_hash_rpath(url):
        """generate relative to cachedir path to cached content"""
        urlhash = hashlib.sha1(url.encode('utf-8')).hexdigest()
        return '/data/' + urlhash[0:2] + '/' + urlhash[2:]

    @staticmethod
    def __get_human_size(sizeinbytes):
        """generate human-readable size representation like du command"""
        sizeinbytes = abs(sizeinbytes)
        output_fmt = '{0}'
        for unit in ['bytes', 'K', 'M', 'G', 'T', 'P']:
            if sizeinbytes < 1024.0:
                return output_fmt.format(sizeinbytes, unit)
            output_fmt = '{0:.1f}{1}'
            sizeinbytes /= 1024.0
        return '{0:.1f}E'.format(sizeinbytes)

    def list(self, args):
        all_cached = {}
        for cachedir in self.cache_dirs:
            for subdir, dirs, files in os.walk(cachedir):
                for f in files:
                    # .meta files is a starting point
                    if f[-5:] != '.meta':
                        continue
                    meta_file = os.path.join(subdir, f)
                    data_file = meta_file[:-5]
                    if not os.path.exists(data_file):
                        self.logger.warning('Corrupted cache data found. '
                                            'There is no data file %s but metadata exists.', data_file)
                        continue
                    with open(meta_file, 'r') as m_f:
                        url = m_f.readline().strip()
                    all_cached[url] = {
                        'file': data_file,
                        'size': os.path.getsize(data_file)
                        # TODO: parse owner, tmestamp and lock if needed at some point
                        # 'locked': os.path.exists(data_file + '.lock'),
                    }
        if args.long:
            max_url_len = max(len(url) for url in all_cached.keys())
            max_path_len = max(len(all_cached[url]['file']) for url in all_cached.keys())
            for url in sorted(all_cached.keys()):
                print('{0}\t{1}\t{2}'.format(url.ljust(max_url_len),
                                             all_cached[url]['file'].ljust(max_path_len),
                                             self.__get_human_size(all_cached[url]['size'])))
        else:
            for url in sorted(all_cached.keys()):
                print(url)

    def is_cached(self, args):
        hash_rpath = self.__get_hash_rpath(args.url)
        for cachedir in self.cache_dirs:
            fpath = cachedir.rstrip('/') + hash_rpath
            self.logger.debug('Checking for cached path: %s', fpath)
            if os.path.exists(fpath):
                if not args.quiet:
                    print(fpath)
                sys.exit(0)
        sys.exit(1)

    def control(self, args):
        if args.action == 'stats':
            self.stats()
        elif args.action == 'list':
            self.list(args)
        elif args.action == 'is-cached':
            self.is_cached(args)
        else:
            self.logger.critical('Unsupported ARC A-REX Cache action %s', args.action)
            sys.exit(1)

    @staticmethod
    def register_parser(root_parser):
        cache_ctl = root_parser.add_parser('cache', help='ARC A-REX Cache control')
        cache_ctl.set_defaults(handler_class=CacheControl)

        cache_actions = cache_ctl.add_subparsers(title='A-REX Cache Actions', dest='action',
                                                 metavar='ACTION', help='DESCRIPTION')

        cache_stats = cache_actions.add_parser('stats', help='Show cache usage statistics')

        cache_list = cache_actions.add_parser('list', help='List cached URLs')
        cache_list.add_argument('-l', '--long', action='store_true', help='Output paths to cached files')

        cache_get = cache_actions.add_parser('is-cached', help='Checks is the URL already in A-REX cache')
        cache_get.add_argument('url', help='URL to check')
        cache_get.add_argument('-q', '--quiet', action='store_true', help='Do not output path to cached file')
