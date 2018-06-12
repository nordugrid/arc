from ControlCommon import *
import sys
import re
import fnmatch


def complete_rte_name(prefix, parsed_args, **kwargs):
    arcconf = get_parsed_arcconf(parsed_args.config)
    if parsed_args.action == 'enable':
        return RTEControl(arcconf).complete_enable()
    if parsed_args.action == 'default':
        return RTEControl(arcconf).complete_default()
    if parsed_args.action == 'disable':
        return RTEControl(arcconf).complete_disable()
    if parsed_args.action == 'undefault':
        return RTEControl(arcconf).complete_undefault()
    if parsed_args.action == 'cat':
        return RTEControl(arcconf).complete_all()
    if parsed_args.action == 'params-get':
        return RTEControl(arcconf).complete_all()
    if parsed_args.action == 'params-set':
        return RTEControl(arcconf).complete_all()


def complete_rte_params(prefix, parsed_args, **kwargs):
    arcconf = get_parsed_arcconf(parsed_args.config)
    return RTEControl(arcconf).complete_params(parsed_args.rte)


def complete_rte_params_values(prefix, parsed_args, **kwargs):
    arcconf = get_parsed_arcconf(parsed_args.config)
    return RTEControl(arcconf).complete_params_values(parsed_args.rte, parsed_args.parameter)


class RTEControl(ComponentControl):
    def __init__(self, arcconfig):
        self.logger = logging.getLogger('ARCCTL.RunTimeEnvironment')
        if arcconfig is None:
            self.logger.critical('Controlling RunTime Environments is not possible without arc.conf defined controldir')
            sys.exit(1)
        # define directories
        self.control_rte_dir = arcconfig.get_value('controldir', 'arex').rstrip('/') + '/rte'
        self.system_rte_dir = ARC_DATA_DIR.rstrip('/') + '/rte'
        self.user_rte_dirs = arcconfig.get_value('runtimedir', 'arex', force_list=True)
        if self.user_rte_dirs is None:
            self.user_rte_dirs = []
        # define internal structures to hold RTEs
        self.all_rtes = {}
        self.system_rtes = {}
        self.user_rtes = {}


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

    def __fetch_rtes(self):
        """Look for RTEs on the filesystem and fill the object structures"""
        # run once per tool invocation
        if self.all_rtes:
            return
        # available pre-installed RTEs
        self.logger.debug('Indexing ARC defined RTEs from %s', self.system_rte_dir)
        self.system_rtes = self.__get_dir_rtes(self.system_rte_dir)
        if not self.system_rtes:
            self.logger.info('There are no RTEs found in ARC defined location %s', self.system_rte_dir)

        # RTEs in user-defined locations
        for urte in self.user_rte_dirs:
            self.logger.debug('Indexing user-defined RTEs from %s', urte)
            rtes = self.__get_dir_rtes(urte)
            if not rtes:
                self.logger.info('There are no RTEs found in user-defined location %s', urte)
            self.user_rtes.update(rtes)

        # all available RTEs
        self.all_rtes.update(self.system_rtes)
        self.all_rtes.update(self.user_rtes)

        # enabled RTEs (linked to controldir)
        self.logger.debug('Indexing enabled RTEs in %s', self.control_rte_dir + '/enabled')
        self.enabled_rtes = self.__get_dir_rtes(self.control_rte_dir + '/enabled')

        # default RTEs (linked to default)
        self.logger.debug('Indexing default RTEs in %s', self.control_rte_dir + '/default')
        self.default_rtes = self.__get_dir_rtes(self.control_rte_dir + '/default')

    def __get_rte_file(self, rte):
        self.__fetch_rtes()
        if rte not in self.all_rtes:
            self.logger.error('There is no %s RunTimeEnvironment found', rte)
            sys.exit(1)
        rte_file = self.all_rtes[rte]
        return rte_file

    def __get_rte_params_file(self, rte):
        rte_params_path = self.control_rte_dir + '/params/'
        rte_params_file = rte_params_path + rte
        if os.path.exists(rte_params_file):
            return rte_params_file
        return None

    def __get_rte_list(self, rtes):
        rte_list = []
        for r in rtes:
            if r in self.all_rtes:
                # filename match goes directly to list
                rte_list.append(r)
            else:
                # check for glob wildcards in rte name
                for irte in self.all_rtes.keys():
                    if fnmatch.fnmatch(irte, r):
                        self.logger.debug('Glob wildcard match for %s RTE, adding to the list.', irte)
                        rte_list.append(irte)
            if not rte_list:
                self.logger.error('Failed to find requested RunTimeEnvironment(s). '
                                  'No RTEs that match \'%s\' are available.', r)
                sys.exit(1)
        return rte_list

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
        self.__fetch_rtes()
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

    def __params_parse(self, rte):
        rte_file = self.__get_rte_file(rte)
        param_str = re.compile(r'#\s*param:([^:]+):([^:]+):([^:]+):(.*)$')
        params = {}
        with open(rte_file) as rte_f:
            max_lines = 20
            for line in rte_f:
                param_re = param_str.match(line)
                if param_re:
                    pname = param_re.group(1)
                    params[pname] = {
                        'name': pname,
                        'allowed_string': param_re.group(2),
                        'allowed_values': param_re.group(2).split(','),
                        'value': param_re.group(3),
                        'description': param_re.group(4)
                    }
                    params_defined = self.__params_read(rte)
                    if pname in params_defined:
                        params[pname]['value'] = params_defined[pname]
                max_lines -= 1
                if not max_lines:
                    break
        return params

    def __params_read(self, rte):
        self.__fetch_rtes()
        rte_params_file = self.__get_rte_params_file(rte)
        params = {}
        if rte_params_file:
            kv_re = re.compile(r'^([^ =]+)="(.*)"\s*$')
            with open(rte_params_file) as rte_parm_f:
                for line in rte_parm_f:
                    kv = kv_re.match(line)
                    if kv:
                        params[kv.group(1)] = kv.group(2)
        return params

    def __params_write(self, rte, params):
        rte_params_path = self.control_rte_dir + '/params/'
        if not os.path.exists(rte_params_path):
            self.logger.debug('Making control directory %s for RunTimeEnvironments parameters', rte_params_path)
            os.makedirs(rte_params_path, mode=0755)

        rte_dir_path = rte_params_path + '/'.join(rte.split('/')[:-1])
        if not os.path.exists(rte_dir_path):
            self.logger.debug('Making RunTimeEnvironment directory structure inside controldir %s', rte_dir_path)
            os.makedirs(rte_dir_path, mode=0755)

        rte_params_file = rte_params_path + rte
        with open(rte_params_file, 'w') as rte_parm_f:
            for p in params.values():
                rte_parm_f.write('{name}="{value}"\n'.format(**p))

    def params_get(self, rte, long=False):
        params = self.__params_parse(rte)
        for pdescr in params.values():
            if long:
                print '{name:>16} = {value:8} {description} (allowed values are: {allowed_string})'.format(**pdescr)
            else:
                print '{name}={value}'.format(**pdescr)

    def params_set(self, rte, parameter, value):
        params = self.__params_parse(rte)
        if parameter not in params:
            self.logger.error('There is no such parameter %s for RunTimeEnvironment %s', parameter, rte)
            sys.exit(1)
        if params[parameter]['allowed_string'] == 'string':
            pass
        elif params[parameter]['allowed_string'] == 'int':
            if not re.match(r'[-0-9]+'):
                self.logger.error('Parameter %s for RunTimeEnvironment %s should be integer', parameter, rte)
                sys.exit(1)
        elif value not in params[parameter]['allowed_values']:
            self.logger.error('Parameter %s for RunTimeEnvironment %s should be one of %s',
                              parameter, rte, params[parameter]['allowed_string'])
            sys.exit(1)
        params[parameter]['value'] = value
        self.__params_write(rte, params)

    def cat_rte(self, rte):
        rte_file = self.__get_rte_file(rte)
        self.logger.info('Printing the content of %s RunTimeEnvironment from %s', rte, rte_file)
        rte_params_file = self.__get_rte_params_file(rte)
        if rte_params_file:
            self.logger.info('Including the content of RunTimeEnvironment parameters file from %s', rte_params_file)
            with open(rte_params_file) as rte_parm_f:
                for line in rte_parm_f:
                    sys.stdout.write(line)
        with open(rte_file, 'r') as rte_fd:
            for line in rte_fd:
                sys.stdout.write(line)
        sys.stdout.flush()

    def enable(self, rtes_def, force=False, rtetype='enabled'):
        """
        Entry point for enable operation.
        RTE definition can be glob wildcarded RTE name.
        """
        self.__fetch_rtes()
        for rte in self.__get_rte_list(rtes_def):
            self.enable_rte(rte, force, rtetype)

    def enable_rte(self, rte, force=False, rtetype='enabled'):
        """
        Enables single RTE by name
        """
        self.__fetch_rtes()

        rte_enable_path = self.control_rte_dir + '/' + rtetype + '/'
        if not os.path.exists(rte_enable_path):
            self.logger.debug('Making control directory %s for %s RunTimeEnvironments', rte_enable_path, rtetype)
            os.makedirs(rte_enable_path, mode=0755)

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

    def disable(self, rte_def, rtetype='enabled'):
        """
        Entry point for disable operation.
        RTE definition can be glob wildcarded RTE name.
        """
        self.__fetch_rtes()
        for rte in self.__get_rte_list(rte_def):
            self.disable_rte(rte, rtetype)

    def disable_rte(self, rte, rtetype='enabled'):
        """
        Disables single RTE by name
        """
        self.__fetch_rtes()

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
            self.enable(args.rtes, args.force)
        elif args.action == 'default':
            self.enable(args.rtes, args.force, 'default')
        elif args.action == 'disable':
            self.disable(args.rtes)
        elif args.action == 'undefault':
            self.disable(args.rtes, 'default')
        elif args.action == 'cat':
            self.cat_rte(args.rte)
        elif args.action == 'params-get':
            self.params_get(args.rte, args.long)
        elif args.action == 'params-set':
            self.params_set(args.rte, args.parameter, args.value)
        else:
            self.logger.critical('Unsupported RunTimeEnvironment control action %s', args.action)
            sys.exit(1)

    def complete_enable(self):
        self.__fetch_rtes()
        return list(set(self.system_rtes.keys() + self.user_rtes.keys()) - set(self.enabled_rtes.keys()))

    def complete_default(self):
        self.__fetch_rtes()
        return list(set(self.system_rtes.keys() + self.user_rtes.keys()) - set(self.default_rtes.keys()))

    def complete_disable(self):
        self.__fetch_rtes()
        return self.enabled_rtes.keys()

    def complete_undefault(self):
        self.__fetch_rtes()
        return self.default_rtes.keys()

    def complete_all(self):
        self.__fetch_rtes()
        return self.all_rtes.keys()

    def complete_params(self, rte):
        self.__fetch_rtes()
        return self.__params_parse(rte).keys()

    def complete_params_values(self, rte, param):
        self.__fetch_rtes()
        param_options = self.__params_parse(rte)[param]
        if param_options['allowed_string'] == 'string' or param_options['allowed_string'] == 'int':
            return []
        return param_options['allowed_values']

    @staticmethod
    def register_parser(root_parser):
        rte_ctl = root_parser.add_parser('rte', help='RunTime Environments')
        rte_ctl.set_defaults(handler_class=RTEControl)

        rte_actions = rte_ctl.add_subparsers(title='RunTime Environments Actions', dest='action',
                                             metavar='ACTION', help='DESCRIPTION')
        rte_enable = rte_actions.add_parser('enable', help='Enable RTE to be used by A-REX')
        rte_enable.add_argument('rtes', nargs='*', help='RTE name').completer = complete_rte_name
        rte_enable.add_argument('-f', '--force', help='Force RTE enabling', action='store_true')

        rte_disable = rte_actions.add_parser('disable', help='Disable RTE to be used by A-REX')
        rte_disable.add_argument('rtes', nargs='*', help='RTE name').completer = complete_rte_name

        rte_list = rte_actions.add_parser('list', help='List RunTime Environments')
        rte_list.add_argument('-l', '--long', help='Detailed listing of RTEs', action='store_true')
        rte_list_types = rte_list.add_mutually_exclusive_group()
        rte_list_types.add_argument('-e', '--enabled', help='List enabled RTEs', action='store_true')
        rte_list_types.add_argument('-d', '--default', help='List default RTEs', action='store_true')
        rte_list_types.add_argument('-a', '--available', help='List available RTEs', action='store_true')
        rte_list_types.add_argument('-s', '--system', help='List available system RTEs', action='store_true')
        rte_list_types.add_argument('-u', '--user', help='List available user-defined RTEs', action='store_true')

        rte_default = rte_actions.add_parser('default', help='Transparently use RTE for every A-REX job')
        rte_default.add_argument('rtes', nargs='*', help='RTE name').completer = complete_rte_name
        rte_default.add_argument('-f', '--force', help='Force RTE enabling', action='store_true')

        rte_undefault = rte_actions.add_parser('undefault', help='Remove RTE from transparent A-REX usage')
        rte_undefault.add_argument('rtes', nargs='*', help='RTE name').completer = complete_rte_name

        rte_cat = rte_actions.add_parser('cat', help='Print the content of RTE file')
        rte_cat.add_argument('rte', help='RTE name').completer = complete_rte_name

        rte_params_get = rte_actions.add_parser('params-get', help='List configurable RTE parameters')
        rte_params_get.add_argument('rte', help='RTE name').completer = complete_rte_name
        rte_params_get.add_argument('-l', '--long', help='Detailed listing of RTEs', action='store_true')

        rte_params_set = rte_actions.add_parser('params-set', help='List configurable RTE parameters')
        rte_params_set.add_argument('rte', help='RTE name').completer = complete_rte_name
        rte_params_set.add_argument('parameter', help='RTE parameter to configure').completer = complete_rte_params
        rte_params_set.add_argument('value', help='RTE parameter value to set').completer = complete_rte_params_values