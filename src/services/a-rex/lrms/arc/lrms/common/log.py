"""
Methods for logging.
"""


import sys, time
import arc


def debug(message = '', caller = None):
    """
    Print a ``DEBUG`` message to the log.

    :param str message: log message
    :param str caller: name of function
    """
    _log(arc.DEBUG, message, caller)

def verbose(message = '', caller = None):
    """
    Print a ``VERBOSE`` message to the log.

    :param str message: log message
    :param str caller: name of function
    """
    _log(arc.VERBOSE, message, caller)

def info(message = '', caller = None):
    """
    Print an ``INFO`` message to the log.

    :param str message: log message
    :param str caller: name of function
    """
    _log(arc.INFO, message, caller)

def warn(message = '', caller = None):
    """
    Print a ``WARNING`` message to the log.

    :param str message: log message
    :param str caller: name of function
    """
    _log(arc.WARNING, message, caller)

def error(message = '', caller = None):
    """
    Print an ``ERROR`` message to the log.

    :param str message: log message
    :param str caller: name of function
    """
    _log(arc.ERROR, message, caller)

def _log(level, message, caller = None):
    """
    Print a message to the log. This method is usually wrapped by the above.

    :param int level: log level
    :param str message: log message
    :param str caller: name of function
    """
    if caller:
        caller = 'PythonLRMS.%s' % caller
    else:
        caller = 'PythonLRMS'
    arc.Logger(arc.Logger.getRootLogger(), caller).msg(level, message)


class ArcError(Exception):
    """
    Print an ``ERROR`` to the log, sleep for 10 seconds and exit.

    :param str msg: error message
    """

    def __init__(self, msg, caller = None):
        error(msg, caller)
