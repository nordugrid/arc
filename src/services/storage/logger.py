import arc, sys

class Logger:

    def __init__(self, service_name, log_level):
        self.service_name = service_name
        self.log_levels = {'VERBOSE':arc.VERBOSE, 'DEBUG':arc.DEBUG, 'INFO':arc.INFO, 
                           'WARNING':arc.WARNING, 'ERROR':arc.ERROR, 'FATAL':arc.FATAL}
        self.log_level = log_level
        self.root_logger = arc.Logger_getRootLogger()
        self.logger = arc.Logger(self.root_logger, self.service_name)
        if self.log_level:
            self.logger.setThreshold(self.log_levels[self.log_level])
        else:
            self.logger.setThreshold(self.root_logger.getThreshold())
        self.logger.addDestination(arc.LogStream(sys.stderr))

    def msg(self, *args):
        """docstring for log"""
        # initializing logging facility
        try:
            system = self.service_name
        except:
            system = 'Storage'
        try:
            log_level = self.log_level
        except:
            log_level = 'DEBUG'
        args = list(args)
        if not args:
            severity = arc.ERROR
            args = ['Python exception:\n', traceback.format_exc()]
        elif self.log_levels.has_key(args[0]):
            # if severity is given as a string
            severity = self.log_levels[args.pop(0)]
        elif args[0] in self.log_levels.values():
            severity = args.pop(0)
        else:
            severity = arc.DEBUG
        mesg = ' '.join([str(arg) for arg in args])
        self.logger.msg(severity, mesg)
        sys.stdout.flush()
        #logger.removeDestinations()
        return mesg
