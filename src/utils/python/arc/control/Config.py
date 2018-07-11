from ControlCommon import *
from arc.utils import reference
import sys
import shutil


def complete_block_name(prefix, parsed_args, **kwargs):
    arcconf = get_parsed_arcconf(parsed_args.config)
    return arcconf.get_config_blocks()


def complete_option_name(prefix, parsed_args, **kwargs):
    arcconf = get_parsed_arcconf(parsed_args.config)
    block = parsed_args.block
    confdict = arcconf.get_config_dict()
    options = []
    if block in confdict:
        options = confdict[block].keys()
    return options


def add_block_and_option_to_parser(parser):
    parser.add_argument('block',
                        help='Name of configuration block (without square breakets)').completer = complete_block_name
    parser.add_argument('option', help='Configuration option name').completer = complete_option_name


class ConfigControl(ComponentControl):
    __brief_list = {
        'storage': {
            'description': 'ARC Storage Areas',
            'options': [
                {'description': 'Control directory', 'params': ('arex', 'controldir')},
                {'description': 'Session directories', 'params': ('arex', 'sessiondir')},
                {'description': 'Scratch directory on Worker Node', 'params': ('arex', 'scratchdir')},
                {'description': 'Cache directories', 'params': ('arex/cache', 'cachedir')},
                {'description': 'Additional user-defined RTE directories', 'params': ('arex', 'runtimedir')},
                {'description': 'Accounting archive directory', 'params': ('arex/jura/archiving', 'archivedir')},
                {'description': 'Gridftpd file storage directory', 'params': ('gridftpd/filedir', 'mount')},
                {'description': 'DataDelivery Service transfers directories',
                 'params': ('datadelivery-service', 'transfer_dir')},
            ]
        },
        'logs': {
            'description': 'ARC Log Files',
            'options': [
                {'description': 'A-REX Service log', 'params': ('arex', 'logfile')},
                {'description': 'A-REX Jobs log', 'params': ('arex', 'joblog')},
                {'description': 'A-REX Helpers log', 'params': ('arex', 'helperlog')},
                {'description': 'A-REX WS Interface log', 'params': ('arex/ws', 'logfile')},
                {'description': 'A-REX Cache cleaning log', 'params': ('arex/cache/clean', 'logfile')},
                {'description': 'A-REX Data Staging central log', 'params': ('arex/data-staging', 'logfile')},
                {'description': 'Jura Accounting log', 'params': ('arex/jura', 'logfile')},
                {'description': 'Gridftp Interface log', 'params': ('gridftpd', 'logfile')},
                {'description': 'Infosys Infoproviders log', 'params': ('infosys', 'logfile')},
                {'description': 'Infosys LDAP/BDII logs', 'params': ('infosys/ldap', 'bdii_log_dir')},
                {'description': 'DataDelivery Service log', 'params': ('datadelivery-service', 'logfile')},
                {'description': 'ACIX Scanner log', 'params': ('acix-scanner', 'logfile')},
                {'description': 'Nordugridmap log', 'params': ('nordugridmap', 'logfile')},
            ]
        }
    }

    def __init__(self, arcconfig):
        self.logger = logging.getLogger('ARCCTL.Config')
        if arcconfig is None:
            self.logger.info('Controlling ARC Configuration is not possible without parsed arc.conf.')
            sys.exit(1)
        self.arcconfig = arcconfig

    def dump(self):
        for confline in self.arcconfig.yield_arc_conf():
            sys.stdout.write(confline)
        sys.stdout.flush()

    def varget(self, args):
        for opt in self.arcconfig.get_value(args.option, args.block, force_list=True):
            sys.stdout.write(opt + '\n')
        sys.stdout.flush()

    def __write_brief_options(self, binfo, textshift=''):
        for opt in binfo['options']:
            block, option = opt['params']
            if self.arcconfig.check_blocks(block):
                sys.stdout.write(textshift + opt['description'] + ':\n')
                isvalues = False
                for value in self.arcconfig.get_value(option, block, force_list=True):
                    if value:
                        sys.stdout.write(textshift + '    ' + value + '\n')
                        isvalues = True
                if not isvalues:
                    sys.stdout.write(textshift + '    Not configured\n')

    def brief(self, args):
        if args.type:
            self.__write_brief_options(self.__brief_list[args.type])
        else:
            for binfo in self.__brief_list.values():
                sys.stdout.write(binfo['description'] + ':\n')
                self.__write_brief_options(binfo, textshift='    ')

    def describe(self, args):
        for line in reference.get_option_description(args.reference, args.block, args.option):
            sys.stdout.write(line)

    def varset(self, args):
        # filter allowed values
        allowed_values = reference.allowed_values(args.reference, args.block, args.option)
        if allowed_values:
            for v in args.value:
                if v not in allowed_values:
                    self.logger.error('Value %s is not allowed. Allowed values are: %s', v, ','.join(allowed_values))
                    sys.exit(1)
        # get configured
        values = self.arcconfig.get_value(args.option, args.block, force_list=True)
        # handle multivalued
        multivalued = reference.is_multivalued(args.reference, args.block, args.option)
        if multivalued and not args.override:
            values.extend(args.value)
        else:
            values = args.value

        if not multivalued and len(values) > 1:
            self.logger.error('The option "%s" in the [%s] block is not multivalued.', args.option, args.block)
            sys.exit(1)
        # modify config
        if args.dry_run:
            for line in self.arcconfig.yield_modified_conf(args.block, args.option, values):
                sys.stdout.write(line)
            sys.stdout.flush()
        else:
            try:
                tmpfile = args.config + '.tmp'
                shutil.copy2(args.config, tmpfile)
                with open(tmpfile, 'w') as tf:
                    for line in self.arcconfig.yield_modified_conf(args.block, args.option, values):
                        tf.write(line)
                self.logger.info('Replacing %s with modified value', args.config)
                shutil.move(tmpfile, args.config)
            except IOError as err:
                self.logger.error('Failed to modify config. %s', str(err))

    def control(self, args):
        if args.action == 'dump':
            self.dump()
        elif args.action == 'get':
            self.varget(args)
        elif args.action == 'describe':
            self.describe(args)
        elif args.action == 'set':
            self.varset(args)
        elif args.action == 'brief':
            self.brief(args)
        else:
            self.logger.critical('Unsupported ARC config control action %s', args.action)
            sys.exit(1)

    @staticmethod
    def register_parser(root_parser):
        config_ctl = root_parser.add_parser('config', help='ARC CE configuration control')
        config_ctl.set_defaults(handler_class=ConfigControl)

        config_actions = config_ctl.add_subparsers(title='Config Actions', dest='action',
                                                   metavar='ACTION', help='DESCRIPTION')

        config_dump = config_actions.add_parser('dump', help='Dump ARC CE running configuration')

        config_get = config_actions.add_parser('get', help='Print configuration option value')
        add_block_and_option_to_parser(config_get)

        config_describe = config_actions.add_parser('describe', help='Describe configuration option')
        add_block_and_option_to_parser(config_describe)
        config_describe.add_argument('-r', '--reference', default=ARC_DATA_DIR+'/examples/arc.conf.reference',
                                     help='Redefine arc.conf.reference location (default is %(default)s)')

        config_set = config_actions.add_parser('set', help='Change configuration option value')
        add_block_and_option_to_parser(config_set)
        config_set.add_argument('value', nargs='+', help='Configuration option value')
        config_set.add_argument('-o', '--override', action='store_true',
                                help='For multivalued options override config values (default is add another one)')
        config_set.add_argument('-r', '--reference', default=ARC_DATA_DIR + '/examples/arc.conf.reference',
                                help='Redefine arc.conf.reference location (default is %(default)s)')
        config_set.add_argument('--dry-run', action='store_true',
                                help='Write the modified config to stdout instead of changing the file')

        config_brief = config_actions.add_parser('brief', help='Print configuration brief points')
        config_brief.add_argument('-t', '--type', help='Show brief only for provided options type',
                                  choices=ConfigControl.__brief_list.keys())
