from __future__ import absolute_import

from .ControlCommon import *
from .Validator import Validator
from arc.utils import reference
import os
import sys
import subprocess


class StartupControl(ComponentControl):

    def __init__(self, arcconfig):
        self.logger = logging.getLogger('ARCCTL.Startup')
        if arcconfig is None:
            self.logger.info('Controlling ARC Startup is not possible without parsed arc.conf.')
            sys.exit(1)
        self.arcconfig = arcconfig


    def verify(self, args):
        validator = Validator(args.reference, self.arcconfig, args.config)
        validator.validate_startup()
        if validator.errors:
            self.logger.error("Validation returned %d error(s) and %d warning(s)", validator.errors, validator.warnings)
        elif validator.warnings:
            self.logger.warning("Validation returned no errors and %d warning(s)", validator.warnings)
        else:
            self.logger.info("Validation returned no errors or warnings")
        return validator.errors

    def control(self, args):
        
        if args.action == 'verify':
            sys.exit(self.verify(args))
        else:
            self.logger.critical('Unsupported ARC startup control action %s', args.action)
            sys.exit(1)
            

    @staticmethod
    def register_parser(root_parser):
        startup_ctl = root_parser.add_parser('startup', help='ARC CE startup control')
        startup_ctl.set_defaults(handler_class=StartupControl)

        startup_actions = startup_ctl.add_subparsers(title='Startup Actions', dest='action',
                                                   metavar='ACTION', help='DESCRIPTION')
        startup_actions.required = True

        startup_verify = startup_actions.add_parser('verify', help='Verify ARC CE startup')
        startup_verify.add_argument('-r', '--reference', default=ARC_DOC_DIR+'/arc.conf.reference',
                                   help='Redefine arc.conf.reference location (default is %(default)s)')
