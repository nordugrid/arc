import arc
import traceback

log_levels = [arc.VERBOSE, arc.DEBUG, arc.INFO, arc.WARNING, arc.ERROR, arc.FATAL]

def get_logger(system = '<UNKNOWN>'):
    return Logger(arc.Logger(arc.Logger_getRootLogger(), system))

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
        elif args[0] in log_levels:
            severity = args.pop(0)
        else:
            severity = arc.DEBUG
        mesg = ' '.join([str(arg) for arg in args])
        self.logger.msg(severity, mesg)
        return mesg
