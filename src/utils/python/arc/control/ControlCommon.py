import os
import logging
import datetime
import argparse
from arc.utils import config
from arc.paths import *

logger = logging.getLogger('ARCCTL.Common')

# Runtime configuration (used for root only)
arcctl_runtime_config = ARC_RUN_DIR + '/arcctl.runtime.conf'
if os.geteuid() != 0:
    arcctl_runtime_config = None


def remove_runtime_config():
    if arcctl_runtime_config is not None:
        if os.path.exists(arcctl_runtime_config):
            os.unlink(arcctl_runtime_config)


# Return parsed arc.conf (conditionally dump and load config with included defaults)
def get_parsed_arcconf(conf_f):
    # handle default
    def_conf_f = config.arcconf_defpath()
    runconf_load = False
    if conf_f is None:
        if os.path.exists(def_conf_f):
            conf_f = def_conf_f
        elif def_conf_f != '/etc/arc.conf' and os.path.exists('/etc/arc.conf'):
            conf_f = '/etc/arc.conf'
            logger.warning('There is no arc.conf in ARC installation prefix (%s). '
                           'Using /etc/arc.conf that exists.', def_conf_f)
        else:
            logger.error('Cannot find ARC configuration file in the default location.')
            return None
        if arcctl_runtime_config is not None:
            runconf_load = os.path.exists(arcctl_runtime_config)
    else:
        logger.debug('Custom ARC configuration file location used. Runtime configuration will not be used.')

    try:
        logger.debug('Getting ARC configuration (config file: %s)', conf_f)
        if runconf_load:
            arcconf_mtime = os.path.getmtime(conf_f)
            default_mtime = os.path.getmtime(config.defaults_defpath())
            runconf_mtime = os.path.getmtime(arcctl_runtime_config)
            if runconf_mtime < arcconf_mtime or runconf_mtime < default_mtime:
                runconf_load = False
        if runconf_load:
            logger.debug('Loading cached parsed configuration from %s', arcctl_runtime_config)
            config.load_run_config(arcctl_runtime_config)
        else:
            logger.debug('Parsing configuration options from %s (with defaults in %s)',
                         conf_f, config.defaults_defpath())
            config.parse_arc_conf(conf_f)
            if arcctl_runtime_config is not None:
                config.save_run_config(arcctl_runtime_config)
        arcconfig = config
        arcconfig.conf_f = conf_f
    except IOError:
        logger.error('Failed to open ARC configuration file %s', conf_f)
        arcconfig = None
    return arcconfig


def valid_datetime_type(arg_datetime_str):
    """Argparse datetime-as-an-argument helper"""
    try:
        return datetime.datetime.strptime(arg_datetime_str, "%Y-%m-%d")
    except ValueError:
        try:
            return datetime.datetime.strptime(arg_datetime_str, "%Y-%m-%d %H:%M")
        except ValueError:
            try:
                return datetime.datetime.strptime(arg_datetime_str, "%Y-%m-%d %H:%M:%S")
            except ValueError:
                msg = "Timestamp format ({0}) is not valid! " \
                      "Expected format: YYYY-MM-DD [HH:mm[:ss]]".format(arg_datetime_str)
                raise argparse.ArgumentTypeError(msg)


class ComponentControl(object):
    """ Common abstract class to ensure all implicit calls to methods are defined """
    def control(self, args):
        raise NotImplementedError()

    @staticmethod
    def register_parser(root_parser):
        raise NotImplementedError()

