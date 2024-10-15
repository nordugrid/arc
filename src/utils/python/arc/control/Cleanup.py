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
        print_warn(self.logger, 'Controldir cleanup will remove job metadata only! Session directories will remain intact.')
        if not args.yes and not args.dry_run:
            print('This action will do the cleanup of the control directly that cannot be undone!')
            print('It is advised to have a backups.')
            if not ask_yes_no('Do you want to continue?'):
                return
        # remove all jobs metadata (keep accounting, rtes, etc)
        if args.wipe:
            for d in ['accepting', 'finished', 'processing', 'restarting', 'jobs', 'delegations']:
                dpath = os.path.join(self.control_dir, d)
                if os.path.exists(dpath):
                    print_info(self.logger, 'Recursively removing the directory: %s', dpath)
                    run_subprocess('rm', '-rf', dpath, dry_run=args.dry_run)
        # garbage collection (orphaned directories and jobs cleanup)
        elif args.gc:
            jobsdir = os.path.join(self.control_dir, 'jobs')
            # erphaned empty directories in the jobs structure
            print_info(self.logger, 'Cleaning up empty job directories in %s', jobsdir)
            run_subprocess('find', jobsdir, '-depth', '-type', 'd', '-empty', '-delete', dry_run=args.dry_run)
            # threre should not be any jobs for which .status is missing
            print_info(self.logger, 'Indexing the jobs managed by A-REX in %s', jobsdir)
            jobs_s = set()
            for d in ['accepting', 'finished', 'processing', 'restarting']:
                dpath = os.path.join(self.control_dir, d)
                self.logger.debug('Indexing jobs in %s', dpath)
                for j in run_subprocess('find', dpath, '-type', 'f', '-name', '*.status', '-printf', '%P\n').splitlines():
                    jobs_s.add(j[:-7])
            if '' in jobs_s:
                jobs_s.remove('')
            self.logger.debug('Found %s jobs managed by A-REX', len(jobs_s))
            print_info(self.logger, 'Cleaning up files for orphaned jobs without the status in %s', jobsdir)
            gc_jobs = 0
            for j in run_subprocess('find', jobsdir, '-mindepth', '4', '-type', 'd', '-printf', '%P\n').splitlines():
                j_split = j.split('/')
                if len(j_split) != 4:
                    self.logger.error('Malformed path found: %s', os.path.join(jobsdir, j))
                    continue
                jobid = ''.join(j_split)
                if jobid not in jobs_s:
                    orphaned_dir = os.path.join(jobsdir, j)
                    print_info(self.logger, 'Removing job directory %s for orphaned job without status', orphaned_dir)
                    run_subprocess('rm', '-rf', orphaned_dir, dry_run=args.dry_run)
                    gc_jobs += 1
            if not gc_jobs:
                print_info(self.logger, 'No orphaned jobs found')
            else:
                print_info(self.logger, 'Cleaned up metadata for %s orphaned jobs', gc_jobs)
                print_info(self.logger, 'Cleaning up empty orphaned jobs directories in %s', jobsdir)
                run_subprocess('find', jobsdir, '-depth', '-type', 'd', '-empty', '-delete', dry_run=args.dry_run)
        # cleanup old finished jobs (force-cleanup outside of A-REX ttl/ttr handling)
        elif args.finished_before is not None:
            print_info(self.logger, 'Cleaning up all jobs finished before %s', args.finished_before)
            jobsdir = os.path.join(self.control_dir, 'jobs')
            finisheddir = os.path.join(self.control_dir, 'finished')
            jobs_f = []
            for j in run_subprocess('find', finisheddir, '!', '-newermt', str(args.finished_before),
                                    '-type', 'f', '-name', '*.status', '-printf', '%P\n').splitlines():
                jobs_f.append(j[:-7])
            if not jobs_f:
                print_info(self.logger, 'No jobs found to cleanup')
                return
            print_info(self.logger, 'Found %s jobs to clean up', len(jobs_f))
            for j in jobs_f:
                print_info(self.logger, 'Cleaning up job %s', j)
                run_subprocess('rm', '-f', os.path.join(finisheddir, j + '.status'), dry_run=args.dry_run)
                run_subprocess('rm', '-rf', control_path(self.control_dir, j, ''), dry_run=args.dry_run)

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
        elif args.action == 'accounting':
            print_info(self.logger, 'To cleanup accounting database use "arcctl accounting database cleanup"')
            print_info(self.logger, 'Please note, you can also backup, rotate and optimize database via "arcctl accounting database"')
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
        cleanup_controldir.add_argument('--dry-run', action='store_true', help='Do not perform any actions, print the intended actions instead')
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


        cleanup_accounting = cleanup_actions.add_parser('accounting', help='Cleanup accounting database')

        if TestCAControl is not None:
            cleanup_testca = cleanup_actions.add_parser('test-ca',help='Cleanup TestCA files')
            cleanup_testca.add_argument('--ca-id', action='store',
                                        help='Define CA ID to work with (default is to use hostname-based hash)')
            cleanup_testca.add_argument('--ca-dir', action='store',
                                        help='Redefine path to CA files directory')
