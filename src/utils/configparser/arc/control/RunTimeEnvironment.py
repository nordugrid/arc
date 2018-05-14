from ControlCommon import *
import sys
import re
from arc.utils import config


def complete_rte_name(prefix, parsed_args, **kwargs):
    try:
        config.parse_arc_conf(parsed_args.config)
        arcconf = config
    except IOError:
        arcconf = None
    if parsed_args.action == 'enable':
        return RTEControl(arcconf).complete_enable()
    if parsed_args.action == 'default':
        return RTEControl(arcconf).complete_default()
    if parsed_args.action == 'disable':
        return RTEControl(arcconf).complete_disable()
    if parsed_args.action == 'undefault':
        return RTEControl(arcconf).complete_undefault()


class RTEControl(ComponentControl):
    def __init__(self, arcconfig):
        self.logger = logging.getLogger('ARCCTL.RunTimeEnvironment')
        if arcconfig is None:
            self.logger.critical('Controlling RunTime Environments is not possible without arc.conf defined controldir')
            sys.exit(1)
        self.control_rte_dir = arcconfig.get_value('controldir', 'arex').rstrip('/') + '/rte'
        self.system_rte_dir = ARC_DATA_DIR.rstrip('/') + '/rte'
        self.user_rte_dirs = arcconfig.get_value('runtimedir', 'arex', force_list=True)
        if self.user_rte_dirs is None:
            self.user_rte_dirs = []

    @staticmethod
    def __get_dir_rtes(rtedir):
        rtes = {}
        for path, dirs, files in os.walk(rtedir):
            if not dirs:
                rtebase = path.lstrip(rtedir + '/')
                for f in files:
                    rtename = rtebase + '/' + f
                    rtepath = path + '/' + f
                    if os.path.islink(rtepath):
                        rtepath = os.readlink(rtepath)
                    rtes[rtename] = rtepath
        return rtes

    @staticmethod
    def __list_rte(rte_dict, long_list, prefix=''):
        for rte, link in rte_dict.iteritems():
            if long_list:
                print '{}{:32} -> {}'.format(prefix, rte, link)
            else:
                print rte

    @staticmethod
    def __get_rte_description(rte_path):
        with open(rte_path) as rte_f:
            max_lines = 10
            description = 'RTE Description is Not Available'
            for line in rte_f:
                descr_re = re.match(r'^#+\s*description:\s*(.*)\s*$', line, flags=re.IGNORECASE)
                if descr_re:
                    description = descr_re.group(1)
                max_lines -= 1
                if not max_lines:
                    break
            return description

    def __get_rtes(self):
        # available pre-installed RTEs
        self.logger.debug('Indexing ARC defined RTEs from %s', self.system_rte_dir)
        self.system_rtes = self.__get_dir_rtes(self.system_rte_dir)
        if not self.system_rtes:
            self.logger.info('There are no RTEs found in ARC defined location %s', self.system_rte_dir)

        # RTEs in user-defined locations
        self.user_rtes = {}
        for urte in self.user_rte_dirs:
            self.logger.debug('Indexing user-defined RTEs from %s', urte)
            rtes = self.__get_dir_rtes(urte)
            if not rtes:
                self.logger.info('There are no RTEs found in user-defined location %s', urte)
            self.user_rtes.update(rtes)

        # all available RTEs
        self.all_rtes = {}
        self.all_rtes.update(self.system_rtes)
        self.all_rtes.update(self.user_rtes)

        # enabled RTEs (linked to controldir)
        self.logger.debug('Indexing enabled RTEs in %s', self.control_rte_dir + '/enabled')
        self.enabled_rtes = self.__get_dir_rtes(self.control_rte_dir + '/enabled')

        # default RTEs (linked to default)
        self.logger.debug('Indexing default RTEs in %s', self.control_rte_dir + '/default')
        self.default_rtes = self.__get_dir_rtes(self.control_rte_dir + '/default')

    def __list_brief(self):
        for rte_type, rte_dict in [('system', self.system_rtes), ('user', self.user_rtes)]:
            for rte, link in rte_dict.iteritems():
                kind = [rte_type]
                if rte_type == 'system':
                    if rte in self.user_rtes:
                        kind.append('masked')
                if rte in self.enabled_rtes:
                    if link == self.enabled_rtes[rte]:
                        kind.append('enabled')
                if rte in self.default_rtes:
                    if link == self.default_rtes[rte]:
                        kind.append('default')
                print '{:32} ({})'.format(rte, ', '.join(kind))

    def __list_long(self):
        # system
        if not self.system_rtes:
            print 'There are no system pre-defined RTEs in {}'.format(self.system_rte_dir)
        else:
            print 'System pre-defined RTEs in {}:'.format(self.system_rte_dir)
            for rte, rte_file in self.system_rtes.iteritems():
                print '\t{:32} # {}'.format(rte, self.__get_rte_description(rte_file))
        # user-defined
        if not self.user_rte_dirs:
            print 'User-defined RTEs are not configured in arc.conf'
        elif not self.user_rtes:
            print 'There are no user-defined RTEs in {}'.format(', '.join(self.user_rte_dirs))
        else:
            print 'User-defined RTEs in {}:'.format(', '.join(self.user_rte_dirs))
            for rte, rte_file in self.user_rtes.iteritems():
                print '\t{:32} # {}'.format(rte, self.__get_rte_description(rte_file))
        # enabled
        if not self.enabled_rtes:
            print 'There are no enabled RTEs'
        else:
            print 'Enabled RTEs:'
            self.__list_rte(self.enabled_rtes, True, prefix='\t')
        # default
        if not self.default_rtes:
            print 'There are no default RTEs'
        else:
            print 'Default RTEs:'
            self.__list_rte(self.default_rtes, True, prefix='\t')

    def list(self, args):
        self.__get_rtes()
        if args.enabled:
            self.__list_rte(self.enabled_rtes, args.long)
        elif args.default:
            self.__list_rte(self.default_rtes, args.long)
        elif args.system:
            self.__list_rte(self.system_rtes, args.long)
        elif args.user:
            self.__list_rte(self.user_rtes, args.long)
        elif args.available:
            self.system_rtes.update(self.user_rtes)
            self.__list_rte(self.system_rtes, args.long)
        elif args.long:
            self.__list_long()
        else:
            self.__list_brief()

    def enable(self, rte, force=False, rtetype='enabled'):
        self.__get_rtes()

        rte_enable_path = self.control_rte_dir + '/' + rtetype + '/'
        if not os.path.exists(rte_enable_path):
            self.logger.debug('Making control directory %s for %s RunTimeEnvironments', rte_enable_path, rtetype)
            os.makedirs(rte_enable_path, mode=0755)

        if rte not in self.all_rtes:
            self.logger.error('Failed to enable RunTimeEnvironment. No such RTE file \'%s\' available.', rte)
            sys.exit(1)

        rte_dir_path = rte_enable_path + '/'.join(rte.split('/')[:-1])
        if not os.path.exists(rte_dir_path):
            self.logger.debug('Making RunTimeEnvironment directory structure inside controldir %s', rte_dir_path)
            os.makedirs(rte_dir_path, mode=0755)

        rte_path = rte_enable_path + rte
        if os.path.exists(rte_path):
            if os.path.islink(rte_path):
                linked_to = os.readlink(rte_path)
                if linked_to != self.all_rtes[rte]:
                    if not force:
                        self.logger.error(
                            'RunTimeEnvironment %s is already enabled but linked to different location (%s). '
                            'Use \'--force\' to relink', rte, linked_to)
                        sys.exit(1)
                    else:
                        self.logger.debug(
                            'RunTimeEnvironment %s is already enabled but linked to different location (%s). '
                            'Removing previous link.', rte, linked_to)
                        os.unlink(rte_path)
                else:
                    self.logger.info('RunTimeEnvironment %s is already %s. Nothing to do.', rte, rtetype)
                    sys.exit(0)
            else:
                self.logger.error('RunTimeEnvironment file %s is already exists but it is not symlink as expected. '
                                  'Have you manually perform modifications of controldir content?', rte_path)
                sys.exit(1)

        self.logger.debug('Linking RunTimeEnvironment file %s to %s', self.all_rtes[rte], rte_enable_path)
        os.symlink(self.all_rtes[rte], rte_enable_path + rte)

    def disable(self, rte, rtetype='enabled'):
        self.__get_rtes()

        enabled_rtes = {}
        if rtetype == 'enabled':
            enabled_rtes = self.enabled_rtes
        elif rtetype == 'default':
            enabled_rtes = self.default_rtes

        if rte not in enabled_rtes:
            self.logger.error('RunTimeEnvironment \'%s\' is not %s.', rte, rtetype)
            sys.exit(0)

        rte_enable_path = self.control_rte_dir + '/' + rtetype + '/'
        self.logger.debug('Removing RunTimeEnvironment link %s', rte_enable_path + rte)
        os.unlink(rte_enable_path + rte)

        rte_split = rte.split('/')[:-1]
        while rte_split:
            rte_dir = rte_enable_path + '/'.join(rte_split)
            if not os.listdir(rte_dir):
                self.logger.debug('Removing empty RunTimeEnvironment directory %s', rte_dir)
                os.rmdir(rte_dir)
            del rte_split[-1]

    def control(self, args):
        if args.action == 'list':
            self.list(args)
        elif args.action == 'enable':
            self.enable(args.rte, args.force)
        elif args.action == 'default':
            self.enable(args.rte, args.force, 'default')
        elif args.action == 'disable':
            self.disable(args.rte)
        elif args.action == 'undefault':
            self.disable(args.rte, 'default')
        else:
            self.logger.critical('Unsupported RunTimeEnvironment control action %s', args.action)
            sys.exit(1)

    def complete_enable(self):
        self.__get_rtes()
        return list(set(self.system_rtes.keys() + self.user_rtes.keys()) - set(self.enabled_rtes.keys()))

    def complete_default(self):
        self.__get_rtes()
        return list(set(self.system_rtes.keys() + self.user_rtes.keys()) - set(self.default_rtes.keys()))

    def complete_disable(self):
        self.__get_rtes()
        return self.enabled_rtes.keys()

    def complete_undefault(self):
        self.__get_rtes()
        return self.default_rtes.keys()

    @staticmethod
    def register_parser(root_parser):
        rte_ctl = root_parser.add_parser('rte', help='RunTime Environments')
        rte_ctl.set_defaults(handler_class=RTEControl)

        rte_actions = rte_ctl.add_subparsers(title='RunTime Environments Actions', dest='action',
                                             metavar='ACTION', help='DESCRIPTION')
        rte_enable = rte_actions.add_parser('enable', help='Enable RTE to be used by A-REX')
        rte_enable.add_argument('rte', help='RTE name').completer = complete_rte_name
        rte_enable.add_argument('-f', '--force', help='Force RTE enabling', action='store_true')

        rte_disable = rte_actions.add_parser('disable', help='Disable RTE to be used by A-REX')
        rte_disable.add_argument('rte', help='RTE name').completer = complete_rte_name

        rte_list = rte_actions.add_parser('list', help='List RunTime Environments')
        rte_list.add_argument('-l', '--long', help='Detailed listing of RTEs', action='store_true')
        rte_list_types = rte_list.add_mutually_exclusive_group()
        rte_list_types.add_argument('-e', '--enabled', help='List enabled RTEs', action='store_true')
        rte_list_types.add_argument('-d', '--default', help='List default RTEs', action='store_true')
        rte_list_types.add_argument('-a', '--available', help='List available RTEs', action='store_true')
        rte_list_types.add_argument('-s', '--system', help='List available system RTEs', action='store_true')
        rte_list_types.add_argument('-u', '--user', help='List available user-defined RTEs', action='store_true')

        rte_default = rte_actions.add_parser('default', help='Transparently use RTE for every A-REX job')
        rte_default.add_argument('rte', help='RTE name').completer = complete_rte_name
        rte_default.add_argument('-f', '--force', help='Force RTE enabling', action='store_true')

        rte_undefault = rte_actions.add_parser('undefault', help='Remove RTE from transparent A-REX usage')
        rte_undefault.add_argument('rte', help='RTE name').completer = complete_rte_name
