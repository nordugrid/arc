from __future__ import absolute_import

from .ControlCommon import *
from .Validator import Validator
from arc.utils import reference
import os
import sys
import subprocess


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
                {'description': 'Infosys Infoproviders log', 'params': ('infosys', 'logfile')},
                {'description': 'Infosys LDAP/BDII logs', 'params': ('infosys/ldap', 'bdii_log_dir')},
                {'description': 'DataDelivery Service log', 'params': ('datadelivery-service', 'logfile')},
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

    def var_get(self, args):
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
        self.logger.debug('Fetching option description from reference file %s', args.reference)
        for line in reference.get_option_description(args.reference, args.block, args.option):
            sys.stdout.write(line)

    def verify(self, args):
        validator = Validator(args.reference, self.arcconfig, args.config)
        validator.validate()
        if validator.errors:
            self.logger.error("Validation returned %d error(s) and %d warning(s)", validator.errors, validator.warnings)
        elif validator.warnings:
            self.logger.warning("Validation returned no errors and %d warning(s)", validator.warnings)
        else:
            self.logger.info("Validation returned no errors or warnings")
        return validator.errors

    def control(self, args):
        if args.action == 'dump':
            self.dump()
        elif args.action == 'get':
            self.var_get(args)
        elif args.action == 'describe':
            self.describe(args)
        elif args.action == 'brief':
            self.brief(args)
        elif args.action == 'verify':
            sys.exit(self.verify(args))
        else:
            self.logger.critical('Unsupported ARC config control action %s', args.action)
            sys.exit(1)

    @staticmethod
    def register_parser(root_parser):
        config_ctl = root_parser.add_parser('config', help='ARC CE configuration control')
        config_ctl.set_defaults(handler_class=ConfigControl)

        config_actions = config_ctl.add_subparsers(title='Config Actions', dest='action',
                                                   metavar='ACTION', help='DESCRIPTION')
        config_actions.required = True

        config_dump = config_actions.add_parser('dump', help='Dump ARC CE running configuration')

        config_get = config_actions.add_parser('get', help='Print configuration option value')
        add_block_and_option_to_parser(config_get)

        config_describe = config_actions.add_parser('describe', help='Describe configuration option')
        add_block_and_option_to_parser(config_describe)
        config_describe.add_argument('-r', '--reference', default=ARC_DOC_DIR+'/arc.conf.reference',
                                     help='Redefine arc.conf.reference location (default is %(default)s)')

        config_brief = config_actions.add_parser('brief', help='Print configuration brief points')
        config_brief.add_argument('-t', '--type', help='Show brief only for provided options type',
                                  choices=ConfigControl.__brief_list.keys())

        config_verify = config_actions.add_parser('verify', help='Verify ARC CE configuration syntax')
        config_verify.add_argument('-r', '--reference', default=ARC_DOC_DIR+'/arc.conf.reference',
                                   help='Redefine arc.conf.reference location (default is %(default)s)')
