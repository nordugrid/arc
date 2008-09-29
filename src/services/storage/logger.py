import arc
import traceback

log_levels = {'VERBOSE':arc.VERBOSE, 'DEBUG':arc.DEBUG, 'INFO':arc.INFO, 
              'WARNING':arc.WARNING, 'ERROR':arc.ERROR, 'FATAL':arc.FATAL}

class Logger:

    def __init__(self, logger):
        self.logger = logger

    def msg(self, *args):
        """docstring for log"""
        # initializing logging facility
        args = list(args)
        if not args:
            severity = arc.ERROR
            args = ['Python exception:\n', traceback.format_exc()]
        elif args[0] in log_levels.values():
            severity = args.pop(0)
        else:
            severity = arc.DEBUG
        mesg = ' '.join([str(arg) for arg in args])
        self.logger.msg(severity, mesg)
        return mesg
