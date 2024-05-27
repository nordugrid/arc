from __future__ import print_function
from __future__ import absolute_import

from .ControlCommon import *
import sys
import datetime
import calendar

from .OSService import OSServiceManagement
from .TestCA import TestCAControl
from .TestJWT import JWTIssuer

def complete_jwt_issuer(prefix, parsed_args, **kwargs):
    arcconf = get_parsed_arcconf(parsed_args.config)
    return JWTIssuer.list_jwt_issuers(arcconf)

class CleanupControl(ComponentControl):
    def __init__(self, arcconfig):
        self.logger = logging.getLogger('ARCCTL.Cleanup')
        # config is mandatory
        if arcconfig is None:
            self.logger.error('Failed to get parsed arc.conf. Cleanup is not possible.')
            sys.exit(1)
        self.arcconfig = arcconfig
        # controldir is mandatory
        self.control_dir = self.arcconfig.get_value('controldir', 'arex').rstrip('/')
        if self.control_dir is None:
            self.logger.critical('Failed to get controldir location. Cleanup is not possible.')
            sys.exit(1)

    @staticmethod
    def unixtimestamp(t):
        """Return unix timestamp representation of date"""
        if isinstance(t, (datetime.datetime, datetime.date)):
            return calendar.timegm(t.timetuple())  # works in Python 2.6
        return t

    def controldir_cleanup(self, args):
        """Process controldir cleanup tasks"""
        if not args.force:
            sm = OSServiceManagement()
            if sm.is_active('arc-arex'):
                self.logger.error('A-REX service is currently running. '
                                  'Running cleanup with running A-REX is not supported and can break the system. '
                                  'If you whant to do it anyway, override with --force.')
                sys.exit(1)
            if not args.yes:
                print('This action will do the cleanup of the control directly that cannot be undone!')
                print('It is advised to have a backups.')
                if not ask_yes_no('Do you want to continue?'):
                    return
        if args.wipe:
            for d in ['accepting', 'finished', 'processing', 'restarting', 'jobs']:
                dpath = os.path.join(self.control_dir, d)
                if os.path.exists(dpath):
                    print_info(self.logger, 'Recursively removing the directory: %s', dpath)
                    run_subprocess('rm', '-rf', dpath)
        elif args.gc:
            jobsdir = os.path.join(self.control_dir, 'jobs')
            print_info(self.logger, 'Cleaning up empty job directories in %s', jobsdir)
            run_subprocess('find', jobsdir, '-type', 'd', '-depth', '-empty', '-delete')
            # TODO: threre should not be any jobs for which .status is missing
        elif args.finished_before is not None:
            print_warn(self.logger, 'Controldir cleanup will remove job metadata only! Session directories remains intact.')
            if not args.yes:
                if not ask_yes_no('Do you want to continue?'):
                    return
            print_info(self.logger, 'Cleaning up all jobs finished before %s', args.finished_before)
            # TODO: implement finished cleanup
            self.logger.error('Not implelemnted yet')
            sys.exit(1)

    def control(self, args):
        if args.action == 'controldir':
            self.controldir_cleanup(args)
        elif args.action == 'test-ca':
            args.action = 'cleanup'
            TestCAControl(self.arcconfig).control(args)
        elif args.action == 'jwt-issuer':
            issuers = JWTIssuer.list_jwt_issuers(self.arcconfig)
            if args.list:
                print('\n'.join(issuers))
            elif args.issuer in issuers:
                iss = JWTIssuer(args.issuer)
                iss.controldir_cleanup(self.arcconfig)
                iss.cleanup_conf_d()
                print_warn(self.logger, 'ARC services restart is needed to apply configuration changes.')
            else:
                self.logger.error('Issuer %s is not trusted by ARC CE. Nothing to cleanup.', args.issuer)
        else:
            self.logger.critical('Unsupported cleanup action %s', args.action)
            sys.exit(1)
    
    @staticmethod
    def register_parser(root_parser):
        cleaup_ctl = root_parser.add_parser('cleanup', help='ARC CE housekeeping')
        cleaup_ctl.set_defaults(handler_class=CleanupControl)

        cleanup_actions = cleaup_ctl.add_subparsers(title='Cleanup Actions', dest='action',
                                                    metavar='ACTION', help='DESCRIPTION')
        cleanup_actions.required = True

        cleanup_controldir = cleanup_actions.add_parser('controldir', help='Cleaup jobs metadata in controldir')
        cleanup_controldir.add_argument('--force', action='store_true', help='Override any safety checks')
        cleanup_controldir.add_argument('--yes', action='store_true', help='Answer yes to all questions')
        controldir_actions = cleanup_controldir.add_mutually_exclusive_group()
        controldir_actions.required = True
        controldir_actions.add_argument('-g', '--gc', action='store_true', help='Cleanup leftovers in control directory')
        controldir_actions.add_argument('-w', '--wipe', action='store_true', help='Cleanup all job records from control directory.')
        controldir_actions.add_argument('-f', '--finished-before', action='store',
                                        type=valid_datetime_type, help='Cleanup jobs finished before specified date (YYYY-MM-DD [HH:mm[:ss]])')

        cleanup_jwt = cleanup_actions.add_parser('jwt-issuer', help='Cleanup JWT issuer trust settings')
        cleanup_issuer = cleanup_jwt.add_mutually_exclusive_group()
        cleanup_issuer.required = True
        cleanup_issuer.add_argument('-l', '--list', action='store_true', help='List JWT issuers trusted by ARC CE')
        cleanup_issuer.add_argument('-i', '--issuer', action='store',
                                    help='JWT Issuer ID to cleanup').completer = complete_jwt_issuer

        if TestCAControl is not None:
            cleanup_testca = cleanup_actions.add_parser('test-ca',help='Cleanup TestCA files')
            cleanup_testca.add_argument('--ca-id', action='store',
                                        help='Define CA ID to work with (default is to use hostname-based hash)')
            cleanup_testca.add_argument('--ca-dir', action='store',
                                        help='Redefine path to CA files directory')
