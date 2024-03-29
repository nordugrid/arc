from __future__ import print_function
from __future__ import absolute_import

from .ControlCommon import *
import sys
from .OSService import OSServiceManagement
from .OSPackage import OSPackageManagement


def complete_service_name(prefix, parsed_args, **kwargs):
    arcconf = get_parsed_arcconf(parsed_args.config)
    return ServicesControl(arcconf).get_all_services()


def add_services_to_parser(parser):
    services_list = parser.add_mutually_exclusive_group(required=True)
    services_list.add_argument('-a', '--as-configured', action='store_true',
                               help='Use information from arc.conf to get services list')
    services_list.add_argument('-s', '--service', action='append',
                               help='Service name').completer = complete_service_name


class ServicesControl(ComponentControl):
    __blocks_map = {
        'arex': {
            'package': 'arex',
            'service': 'arc-arex'
        },
        'arex/ws': {
            'package': 'arex',
            'service': 'arc-arex-ws'
        },
        'gridftpd': {
            'package': 'gridftpd',
            'service': 'arc-gridftpd'
        },
        'infosys/ldap': {
            'package': 'infosys-ldap',
            'service': 'arc-infosys-ldap'
        },
        'datadelivery-service': {
            'package': 'datadelivery-service',
            'service': 'arc-datadelivery-service'
        },
        'acix-scanner': {
            'package': 'acix-scanner',
            'service': 'arc-acix-scanner'
        },
        'acix-index': {
            'package': 'acix-index',
            'service': 'arc-acix-index'
        },
        'nordugridmap': {
            'package': 'nordugridmap',
            'service':  None
        }
    }

    def __init__(self, arcconfig):
        self.logger = logging.getLogger('ARCCTL.Services')
        if arcconfig is None:
            self.logger.info('Controlling ARC CE Services is not possible without arc.conf.')
            sys.exit(1)
        self.arcconfig = arcconfig
        self.sm = None
        self.pm = None
        self.package_base = 'nordugrid-arc'

    def __get_pm_sm(self):
        if self.sm is None:
            self.pm = OSPackageManagement()
            # check is arcctl-service package (that contains service control modules) installed via packet manager
            if self.pm.is_installed(self.package_base + '-arcctl-service'):
                self.sm = OSServiceManagement()
            # epel6 and epel7 contains 'nordugrid-arc6' base to coexist with ARC5 release
            elif self.pm.is_installed(self.package_base + '6-arcctl-service'):
                self.package_base += '6'
                self.sm = OSServiceManagement()
            # ARC installed without known packet manager
            else:
                self.pm = None
                self.sm = OSServiceManagement(ARC_LOCATION + '/etc/rc.d/init.d/')
        return self.pm, self.sm

    def __get_configured(self):
        packages_needed = set()
        services_needed = set()
        services_all = set()
        for block in self.__blocks_map:
            bservice = self.__blocks_map[block]['service']
            if bservice is not None:
                services_all.add(bservice)
            if self.arcconfig.check_blocks(block):
                if self.__blocks_map[block]['package'] is not None:
                    packages_needed.add(self.package_base + '-' + self.__blocks_map[block]['package'])
                if bservice is not None:
                    services_needed.add(bservice)
        return packages_needed, services_all, services_needed

    def __packages_install(self, packages_needed):
        pm, _ = self.__get_pm_sm()
        if pm is None:
            self.logger.info('ARC is installed from sources. Skipping OS packages management.')
            return
        install_list = []
        for p in packages_needed:
            if not pm.is_installed(p):
                install_list.append(p)
        if install_list:
            self.logger.info('Installing the following needed packages: %s', ','.join(install_list))
            pm.install(install_list)

    def __services_stop(self, services_stop, sm):
        for ds in services_stop:
            self.logger.debug('Checking %s service is already stopped', ds)
            if sm.is_active(ds):  # if service not installed is_active also returns False
                self.logger.info('Stopping %s service in accordance to arc.conf configuration', ds)
                sm.stop(ds)

    def __services_start(self, services_start, sm):
        for ss in services_start:
            self.logger.debug('Checking %s service is already started', ss)
            if not sm.is_active(ss):
                self.logger.info('Starting %s service in accordance to arc.conf configuration', ss)
                sm.start(ss)

    def __services_enable(self, services, sm, now=False):
        for es in services:
            self.logger.debug('Checking %s service is already enabled', es)
            if not sm.is_enabled(es):
                self.logger.info('Enabling %s service in accordance to arc.conf configuration', es)
                sm.enable(es)
        if now:
            # start services as configured in current arc.conf
            self.__services_start(services, sm)

    def __services_disable(self, services, sm, now=False):
        for ds in services:
            self.logger.debug('Checking %s service is already disabled', ds)
            if sm.is_enabled(ds):  # if service not installed is_enabled also returns False
                self.logger.info('Disabling %s service in accordance to arc.conf configuration', ds)
                sm.disable(ds)
        if now:
            # stop services not configured in current arc.conf
            self.__services_stop(services, sm)

    def start_as_configured(self):
        pm, sm = self.__get_pm_sm()
        packages_needed, services_all, services_needed = self.__get_configured()
        # ensure packages are installed
        self.__packages_install(packages_needed)
        # stop services not configured in current arc.conf
        self.__services_stop(list(services_all - services_needed), sm)
        # start services as configured in current arc.conf
        self.__services_start(services_needed, sm)

    def enable_as_configured(self, now=False):
        pm, sm = self.__get_pm_sm()
        packages_needed, services_all, services_needed = self.__get_configured()
        # ensure packages are installed
        self.__packages_install(packages_needed)
        # disable services not configured in arc.conf
        self.__services_disable(list(services_all - services_needed), sm, now)
        # enable necessary services
        self.__services_enable(services_needed, sm, now)

    def list_services(self, args):
        pm, sm = self.__get_pm_sm()
        services = {}
        for s in self.__blocks_map.values():
            """ N/A for services that do not have their own package, i.e. s['package'] is None. """
            installed_str = 'N/A'
            sname = s['service']
            if sname is None:
                continue
            if sname in services:
                continue
            if pm is None:
                installed = sm.is_installed(s['service'])
                installed_str = 'Built from source' if installed else 'Not built'
            else:
                if s['package'] is not None:
                    installed = pm.is_installed(self.package_base + '-' + s['package'])
                    installed_str = 'Installed' if installed else 'Not installed'
            active = sm.is_active(s['service'])
            enabled = sm.is_enabled(s['service'])
            services[sname] = {
                'name': sname,
                'installed': installed,
                'installed_str': installed_str,
                'active': active,
                'active_str': 'Running' if active else 'Stopped',
                'enabled': enabled,
                'enabled_str': 'Enabled' if enabled else 'Disabled'
            }
        if args.installed:
            print(' '.join(sorted([s['name'] for s in [s for s in services.values() if s['installed']]])))
        elif args.enabled:
            print(' '.join(sorted([s['name'] for s in [s for s in services.values() if s['enabled']]])))
        elif args.active:
            print(' '.join(sorted([s['name'] for s in [s for s in services.values() if s['active']]])))
        else:
            for ss in sorted(list(services.values()), key=lambda k: k['name']):
                print('{name:32} ({installed_str}, {enabled_str}, {active_str})'.format(**ss))

    def control(self, args):
        _, sm = self.__get_pm_sm()
        if args.action == 'enable':
            if args.as_configured:
                self.enable_as_configured(args.now)
            else:
                self.__services_enable(args.service, sm, args.now)
        elif args.action == 'disable':
            if args.as_configured:
                services = self.get_all_services()
            else:
                services = args.service
            self.__services_disable(services, sm, args.now)
        elif args.action == 'start':
            if args.as_configured:
                self.start_as_configured()
            else:
                self.__services_start(args.service, sm)
        elif args.action == 'stop':
            if args.as_configured:
                services = self.get_all_services()
            else:
                services = args.service
            self.__services_stop(services, sm)
        elif args.action == 'restart':
            if args.as_configured:
                self.__services_stop(self.get_all_services(), sm)
                self.start_as_configured()
            else:
                self.__services_stop(args.service, sm)
                self.__services_start(args.service, sm)
            pass
        elif args.action == 'list':
            self.list_services(args)
        else:
            self.logger.critical('Unsupported ARC services control action %s', args.action)
            sys.exit(1)

    def get_all_services(self):
        services = set()
        for s in self.__blocks_map.values():
            if s['service'] is not None:
                services.add(s['service'])
        return list(services)

    @staticmethod
    def register_parser(root_parser):
        services_ctl = root_parser.add_parser('service', help='ARC CE services control')
        services_ctl.set_defaults(handler_class=ServicesControl)

        services_actions = services_ctl.add_subparsers(title='Services Actions', dest='action',
                                                       metavar='ACTION', help='DESCRIPTION')
        services_actions.required = True

        services_enable = services_actions.add_parser('enable', help='Enable ARC CE services')
        services_enable.add_argument('--now', help='Start the services just after enable', action='store_true')
        add_services_to_parser(services_enable)

        services_disable = services_actions.add_parser('disable', help='Disable ARC CE services')
        services_disable.add_argument('--now', help='Stop the services just after disable', action='store_true')
        add_services_to_parser(services_disable)

        services_start = services_actions.add_parser('start', help='Start ARC CE services')
        add_services_to_parser(services_start)

        services_restart = services_actions.add_parser('restart', help='Restart ARC CE services')
        add_services_to_parser(services_restart)

        services_stop = services_actions.add_parser('stop', help='Stop ARC CE services')
        add_services_to_parser(services_stop)

        services_list = services_actions.add_parser('list', help='List ARC CE services and their states')
        services_filter = services_list.add_mutually_exclusive_group(required=False)
        services_filter.add_argument('-i', '--installed', help='Show only installed services', action='store_true')
        services_filter.add_argument('-e', '--enabled', help='Show only enabled services', action='store_true')
        services_filter.add_argument('-a', '--active', help='Show only running services', action='store_true')

