from __future__ import print_function
from __future__ import absolute_import

from .ControlCommon import *
import sys
import re
import fnmatch
from itertools import chain


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
        self.enabled_rtes = {}
        self.default_rtes = {}
        self.dummy_rtes = {}
        self.broken_rtes = {}

    @staticmethod
    def __get_dir_rtes(rtedir):
        rtes = {}
        for path, _, files in os.walk(rtedir):
            rtebase = path.lstrip(rtedir + '/')
            for f in files:
                rtename = rtebase + '/' + f
                rtepath = path + '/' + f
                if os.path.islink(rtepath):
                    rtepath = os.readlink(rtepath)
                rtes[rtename] = rtepath
        return rtes

    @staticmethod
    def __list_rte(rte_dict, long_list, prefix='', suffix='', broken_list=None):
        if broken_list is None:
            broken_list = []
        for rte in sorted(rte_dict):
            if rte in broken_list:
                suffix += ' (broken)'
            if long_list:
                print('{0}{1:32} -> {2}{3}'.format(prefix, rte, rte_dict[rte], suffix))
            else:
                print('{0}{1}'.format(rte, suffix))

    @staticmethod
    def __get_rte_description(rte_path):
        if rte_path == '/dev/null':
            return 'Dummy RTE for information publishing'
        with open(rte_path) as rte_f:
            max_lines = 10
            description = 'RTE description is Not Available'
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

        for rte, rtepath in self.enabled_rtes.items():
            # handle dummy enabled RTEs
            if rtepath == '/dev/null':
                self.dummy_rtes[rte] = '/dev/null'
            # detect broken RTEs
            if not os.path.exists(rtepath):
                self.broken_rtes[rte] = rtepath
                self.logger.warning('RunTimeEnvironment %s is enabled but the link to %s is broken.', rte, rtepath)

        # default RTEs (linked to default)
        self.logger.debug('Indexing default RTEs in %s', self.control_rte_dir + '/default')
        self.default_rtes = self.__get_dir_rtes(self.control_rte_dir + '/default')
        for rte, rtepath in self.default_rtes.items():
            # detect broken RTEs
            if not os.path.exists(rtepath):
                self.broken_rtes[rte] = rtepath
                self.logger.warning('RunTimeEnvironment %s is set default but the link to %s is broken.', rte, rtepath)

    def __get_rte_file(self, rte):
        self.__fetch_rtes()
        if rte not in self.all_rtes:
            if rte not in self.dummy_rtes:
                self.logger.error('There is no %s RunTimeEnvironment found', rte)
                sys.exit(1)
            else:
                return '/dev/null'
        return self.all_rtes[rte]

    def __get_rte_params_file(self, rte):
        rte_params_path = self.control_rte_dir + '/params/'
        rte_params_file = rte_params_path + rte
        if os.path.exists(rte_params_file):
            return rte_params_file
        return None

    def __get_rte_list(self, rtes, check_dict=None):
        rte_list = []
        if check_dict is None:
            check_dict = self.all_rtes
        for r in rtes:
            if r.startswith('/'):
                # path instead of name (comes from filesystem paths in user and system RTE dirs)
                rte_found = False
                for rname, rpath in chain(iter(self.user_rtes.items()), iter(self.system_rtes.items())):
                    if rpath == r:
                        self.logger.debug('RTE path %s match %s RTE name, adding to the list.', rpath, rname)
                        rte_list.append({'name': rname, 'path': rpath})
                        rte_found = True
                        break
                if not rte_found:
                    self.logger.error('There is no RTE defined by %s path.', r)
            elif r in check_dict:
                # filename match goes directly to list
                rte_list.append({'name': r, 'path': check_dict[r]})
            else:
                # check for glob wildcards in rte name
                rte_found = False
                for irte in check_dict:
                    if fnmatch.fnmatch(irte, r):
                        self.logger.debug('Glob wildcard match for %s RTE, adding to the list.', irte)
                        rte_list.append({'name': irte, 'path': check_dict[irte]})
                        rte_found = True
                if not rte_found:
                    self.logger.error('There are no RTEs matched to %s found', r)
        if not rte_list:
            self.logger.error('Failed to find requested RunTimeEnvironment(s). '
                              'No RTEs that matches \'%s\' are available.', ' '.join(rtes))
            sys.exit(1)
        return rte_list

    def __list_brief(self):
        for rte_type, rte_dict in [('system', self.system_rtes),
                                   ('user', self.user_rtes),
                                   ('dummy', self.dummy_rtes),
                                   ('broken', self.broken_rtes)]:
            for rte in sorted(rte_dict):
                link = rte_dict[rte]
                kind = [rte_type]
                show_disabled = True
                if rte_type == 'system':
                    if rte in self.user_rtes or rte in self.dummy_rtes:
                        kind.append('masked')
                if rte in self.enabled_rtes:
                    if link == self.enabled_rtes[rte]:
                        kind.append('enabled')
                        show_disabled = False
                if rte in self.default_rtes:
                    if link == self.default_rtes[rte]:
                        kind.append('default')
                        show_disabled = False
                if rte in self.broken_rtes:
                    show_disabled = False
                if show_disabled:
                    kind.append('disabled')
                print('{0:32} ({1})'.format(rte, ', '.join(kind)))

    def __list_long(self):
        # system
        if not self.system_rtes:
            print('There are no system pre-defined RTEs in {0}'.format(self.system_rte_dir))
        else:
            print('System pre-defined RTEs in {0}:'.format(self.system_rte_dir))
            for rte in sorted(self.system_rtes):
                print('\t{0:32} # {1}'.format(rte, self.__get_rte_description(self.system_rtes[rte])))
        # user-defined
        if not self.user_rte_dirs:
            print('User-defined RTEs are not configured in arc.conf')
        elif not self.user_rtes:
            print('There are no user-defined RTEs in {0}'.format(', '.join(self.user_rte_dirs)))
        else:
            print('User-defined RTEs in {0}:'.format(', '.join(self.user_rte_dirs)))
            for rte in sorted(self.user_rtes):
                print('\t{0:32} # {1}'.format(rte, self.__get_rte_description(self.user_rtes[rte])))
        # enabled
        if not self.enabled_rtes:
            print('There are no enabled RTEs')
        else:
            print('Enabled RTEs:')
            self.__list_rte(self.enabled_rtes, True, prefix='\t', broken_list=self.broken_rtes.keys())
        # default
        if not self.default_rtes:
            print('There are no default RTEs')
        else:
            print('Default RTEs:')
            rte_dict = {}
            for rte, rtepath in self.default_rtes.items():
                if rte in self.broken_rtes:
                    rte += ' (broken)'
                    rte_dict[rte] = rtepath
            self.__list_rte(self.default_rtes, True, prefix='\t', broken_list=self.broken_rtes.keys())

    def list(self, args):
        self.__fetch_rtes()
        if args.enabled:
            self.__list_rte(self.enabled_rtes, args.long, broken_list=self.broken_rtes.keys())
        elif args.default:
            self.__list_rte(self.default_rtes, args.long, broken_list=self.broken_rtes.keys())
        elif args.system:
            self.__list_rte(self.system_rtes, args.long)
        elif args.user:
            self.__list_rte(self.user_rtes, args.long)
        elif args.available:
            self.system_rtes.update(self.user_rtes)
            self.__list_rte(self.system_rtes, args.long)
        elif args.dummy:
            self.__list_rte(self.dummy_rtes, args.long)
        elif args.long:
            self.__list_long()
        else:
            self.__list_brief()

    def __params_parse(self, rte):
        rte_file = self.__get_rte_file(rte)
        param_str = re.compile(r'#\s*param:([^:]+):([^:]+):([^:]*):(.*)$')
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
                        'default_value': param_re.group(3),
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
            os.makedirs(rte_params_path, mode=0o755)

        rte_dir_path = rte_params_path + '/'.join(rte.split('/')[:-1])
        if not os.path.exists(rte_dir_path):
            self.logger.debug('Making RunTimeEnvironment directory structure inside controldir %s', rte_dir_path)
            os.makedirs(rte_dir_path, mode=0o755)

        rte_params_file = rte_params_path + rte
        with open(rte_params_file, 'w') as rte_parm_f:
            for p in params.values():
                rte_parm_f.write('{name}="{value}"\n'.format(**p))

    def params_get(self, rte, is_long=False):
        params = self.__params_parse(rte)
        for pdescr in params.values():
            # output
            if is_long:
                # set strings for undefined values output
                if pdescr['value'] == '':
                    pdescr['value'] = 'undefined'
                if pdescr['default_value'] == '':
                    pdescr['default_value'] = 'undefined'
                print('{name:>16} = {value:10} {description} (default is {default_value}) '
                      '(allowed values are: {allowed_string})'.format(**pdescr))
            else:
                print('{name}={value}'.format(**pdescr))

    def params_unset(self, rte, parameter):
        self.params_set(rte, parameter, None, use_default=True)

    def params_set(self, rte, parameter, value, use_default=False):
        params = self.__params_parse(rte)
        if parameter not in params:
            self.logger.error('There is no such parameter %s for RunTimeEnvironment %s', parameter, rte)
            sys.exit(1)
        # use default value if requested
        if use_default:
            value = params[parameter]['default_value']
        # check type and allowed values
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
        # assign new value
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

    def enable(self, rtes_def, force=False, rtetype='enabled', dummy=False):
        """
        Entry point for enable operation.
        RTE definition can be glob wildcarded RTE name.
        """
        if dummy:
            # enable dummy rtes (linked to /dev/null) for provided names
            for rte in rtes_def:
                self.enable_rte({'name': rte, 'path': '/dev/null'}, force, rtetype)
        else:
            # find RTEs by name (including wildcards substitutions) and enable
            self.__fetch_rtes()
            for rte in self.__get_rte_list(rtes_def):
                self.enable_rte(rte, force, rtetype)

    def enable_rte(self, rteinfo, force=False, rtetype='enabled'):
        """
        Enables single RTE
        """
        rtename = rteinfo['name']
        rtepath = rteinfo['path']

        rte_enable_path = self.control_rte_dir + '/' + rtetype + '/'
        if not os.path.exists(rte_enable_path):
            self.logger.debug('Making control directory %s for %s RunTimeEnvironments', rte_enable_path, rtetype)
            os.makedirs(rte_enable_path, mode=0o755)

        rte_dir_path = rte_enable_path + '/'.join(rtename.split('/')[:-1])
        if not os.path.exists(rte_dir_path):
            self.logger.debug('Making RunTimeEnvironment directory structure inside controldir %s', rte_dir_path)
            os.makedirs(rte_dir_path, mode=0o755)

        rte_path = rte_enable_path + rtename
        if os.path.exists(rte_path):
            # handle case for already enabled RTEs
            if os.path.islink(rte_path):
                linked_to = os.readlink(rte_path)
                if linked_to != rtepath:
                    log_msg_base = 'RunTimeEnvironment %s is already enabled but linked to different location (%s).'
                    if not force:
                        self.logger.error(log_msg_base + 'Use \'--force\' to relink', rtename, linked_to)
                        sys.exit(1)
                    else:
                        self.logger.debug(log_msg_base + 'Removing previous link.', rtename, linked_to)
                        os.unlink(rte_path)
                else:
                    self.logger.warning('RunTimeEnvironment %s is already %s. Nothing to do.', rtename, rtetype)
                    return
            else:
                self.logger.error('RunTimeEnvironment file %s is already exists but it is not symlink as expected. '
                                  'Have you manually perform modifications of controldir content?', rte_path)
                sys.exit(1)
        elif os.path.islink(rte_path):
            # handle broken symlink case
            log_msg_base = 'RunTimeEnvironment %s is already enabled but points to not-existing location (%s).'
            if not force:
                self.logger.error(log_msg_base + 'Use \'--force\' to relink.', rtename, os.readlink(rte_path))
                sys.exit(1)
            else:
                self.logger.debug(log_msg_base + 'Removing broken link.', rtename, os.readlink(rte_path))
                os.unlink(rte_path)
        # create link to controldir
        try:
            self.logger.debug('Linking RunTimeEnvironment file %s to %s', rtepath, rte_enable_path)
            os.symlink(rtepath, rte_enable_path + rtename)
        except OSError as e:
            self.logger.error('Filed to link RunTimeEnvironment file %s to %s. Error: %s', rtepath,
                              rte_enable_path, e.strerror)
            sys.exit(1)

    def disable(self, rte_def, rtetype='enabled'):
        """
        Entry point for disable operation.
        RTE definition can be glob wildcarded RTE name.
        """
        self.__fetch_rtes()
        check_dict = getattr(self, rtetype + '_rtes', None)
        for rte in self.__get_rte_list(rte_def, check_dict):
            self.disable_rte(rte, rtetype)

    def disable_rte(self, rteinfo, rtetype='enabled'):
        """
        Disables single RTE
        """
        rtename = rteinfo['name']
        rtepath = rteinfo['path']

        enabled_rtes = getattr(self, rtetype + '_rtes', {})

        if rtename not in enabled_rtes:
            self.logger.error('RunTimeEnvironment \'%s\' is not %s.', rtename, rtetype)
            return

        if enabled_rtes[rtename] != rtepath:
            self.logger.error('RunTimeEnvironment \'%s\' is %s but linked to the %s instead of %s. '
                              'Please use either RTE name or correct path.',
                              rtename, rtetype, enabled_rtes[rtename], rtepath)
            return

        rte_enable_path = self.control_rte_dir + '/' + rtetype + '/'
        self.logger.debug('Removing RunTimeEnvironment link %s', rte_enable_path + rtename)
        os.unlink(rte_enable_path + rtename)

        rte_split = rtename.split('/')[:-1]
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
            self.enable(args.rte, args.force, dummy=args.dummy)
        elif args.action == 'default':
            self.enable(args.rte, args.force, 'default')
        elif args.action == 'disable':
            self.disable(args.rte)
        elif args.action == 'undefault':
            self.disable(args.rte, 'default')
        elif args.action == 'cat':
            self.cat_rte(args.rte)
        elif args.action == 'params-get':
            self.params_get(args.rte, args.long)
        elif args.action == 'params-set':
            self.params_set(args.rte, args.parameter, args.value)
        elif args.action == 'params-unset':
            self.params_unset(args.rte, args.parameter)
        else:
            self.logger.critical('Unsupported RunTimeEnvironment control action %s', args.action)
            sys.exit(1)

    def complete_enable(self):
        self.__fetch_rtes()
        return list(set(list(self.system_rtes.keys()) + list(self.user_rtes.keys())) - set(self.enabled_rtes.keys()))

    def complete_default(self):
        self.__fetch_rtes()
        return list(set(list(self.system_rtes.keys()) + list(self.user_rtes.keys())) - set(self.default_rtes.keys()))

    def complete_disable(self):
        self.__fetch_rtes()
        return list(self.enabled_rtes.keys())

    def complete_undefault(self):
        self.__fetch_rtes()
        return list(self.default_rtes.keys())

    def complete_all(self):
        self.__fetch_rtes()
        return list(self.all_rtes.keys())

    def complete_params(self, rte):
        self.__fetch_rtes()
        return list(self.__params_parse(rte).keys())

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
        rte_enable.add_argument('rte', nargs='+', help='RTE name').completer = complete_rte_name
        rte_enable.add_argument('-f', '--force', help='Force RTE enabling', action='store_true')
        rte_enable.add_argument('-d', '--dummy', action='store_true',
                                help='Enable dummy RTE that do nothing but published in the infosys')

        rte_disable = rte_actions.add_parser('disable', help='Disable RTE to be used by A-REX')
        rte_disable.add_argument('rte', nargs='+', help='RTE name').completer = complete_rte_name

        rte_list = rte_actions.add_parser('list', help='List RunTime Environments')
        rte_list.add_argument('-l', '--long', help='Detailed listing of RTEs', action='store_true')
        rte_list_types = rte_list.add_mutually_exclusive_group()
        rte_list_types.add_argument('-e', '--enabled', help='List enabled RTEs', action='store_true')
        rte_list_types.add_argument('-d', '--default', help='List default RTEs', action='store_true')
        rte_list_types.add_argument('-a', '--available', help='List available RTEs', action='store_true')
        rte_list_types.add_argument('-s', '--system', help='List available system RTEs', action='store_true')
        rte_list_types.add_argument('-u', '--user', help='List available user-defined RTEs', action='store_true')
        rte_list_types.add_argument('-n', '--dummy', help='List dummy enabled RTEs', action='store_true')

        rte_default = rte_actions.add_parser('default', help='Transparently use RTE for every A-REX job')
        rte_default.add_argument('rte', nargs='+', help='RTE name').completer = complete_rte_name
        rte_default.add_argument('-f', '--force', help='Force RTE enabling', action='store_true')

        rte_undefault = rte_actions.add_parser('undefault', help='Remove RTE from transparent A-REX usage')
        rte_undefault.add_argument('rte', nargs='+', help='RTE name').completer = complete_rte_name

        rte_cat = rte_actions.add_parser('cat', help='Print the content of RTE file')
        rte_cat.add_argument('rte', help='RTE name').completer = complete_rte_name

        rte_params_get = rte_actions.add_parser('params-get', help='List configurable RTE parameters')
        rte_params_get.add_argument('rte', help='RTE name').completer = complete_rte_name
        rte_params_get.add_argument('-l', '--long', help='Detailed listing of parameters', action='store_true')

        rte_params_set = rte_actions.add_parser('params-set', help='Set configurable RTE parameter')
        rte_params_set.add_argument('rte', help='RTE name').completer = complete_rte_name
        rte_params_set.add_argument('parameter', help='RTE parameter to configure').completer = complete_rte_params
        rte_params_set.add_argument('value', help='RTE parameter value to set').completer = complete_rte_params_values

        rte_params_unset = rte_actions.add_parser('params-unset', help='Use default value for RTE parameter')
        rte_params_unset.add_argument('rte', help='RTE name').completer = complete_rte_name
        rte_params_unset.add_argument('parameter', help='RTE parameter to unset').completer = complete_rte_params
