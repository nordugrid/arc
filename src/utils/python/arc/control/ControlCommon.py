from __future__ import absolute_import
import sys
import os
import logging
import datetime
import argparse
import zlib
from arc.utils import config

logger = logging.getLogger('ARCCTL.Common')

try:
    from .ServiceCommon import *
except ImportError:
    arcctl_runtime_config = None

    def arcctl_server_mode():
        return False

    def remove_runtime_config():
        pass


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


def get_human_readable_size(sizeinbytes):
    """generate human-readable size representation like du command"""
    sizeinbytes = abs(sizeinbytes)
    output_fmt = '{0}'
    for unit in ['bytes', 'K', 'M', 'G', 'T', 'P']:
        if sizeinbytes < 1024.0:
            return output_fmt.format(sizeinbytes, unit)
        output_fmt = '{0:.1f}{1}'
        sizeinbytes /= 1024.0
    return '{0:.1f}E'.format(sizeinbytes)

def crc32_id(data_str):
    """returns hexadecimal CRC32 to use as checksum ID"""
    return hex(zlib.crc32(data_str.encode('utf-8')) & 0xffffffff)[2:]


def ensure_root():
    """Prints error and exit if not root"""
    if os.geteuid() != 0:
        logger.error("This functionality is accessible to root only")
        sys.exit(1)


def ensure_path_writable(path):
    """Prints error and exit if there are no write permission to specified path"""
    if not os.path.exists(path):
        # if no such path - do not complain (error will be on dir creation)
        return
    mode = os.W_OK
    if os.path.isdir(path):
        mode |= os.X_OK
    if not os.access(path, mode):
        logger.error("The path '%s' is not writable for user running arcctl", path)
        sys.exit(1)


def get_parsed_arcconf(conf_f):
    """Return parsed arc.conf (taking into account defaults and cached running config if applicable"""
    # handle default
    def_conf_f = config.arcconf_defpath()
    runconf_load = False
    # if not overrided by command line --config argument (passed as a parameter to this function)
    if conf_f is None:
        # check location is defined by ARC_CONFIG
        if 'ARC_CONFIG' in os.environ:
            conf_f = os.environ['ARC_CONFIG']
            logger.debug('Will use ARC configuration file %s as defined by ARC_CONFIG environmental variable', conf_f)
            if not os.path.exists(conf_f):
                logger.error('Cannot find ARC configuration file %s (defined by ARC_CONFIG environmental variable)',
                             conf_f)
                return None
        # use installation prefix location
        elif os.path.exists(def_conf_f):
            conf_f = def_conf_f
            runconf_load = True
        # or fallback to /etc/arc.conf
        elif def_conf_f != '/etc/arc.conf' and os.path.exists('/etc/arc.conf'):
            conf_f = '/etc/arc.conf'
            logger.warning('There is no arc.conf in ARC installation prefix (%s). '
                           'Using /etc/arc.conf that exists.', def_conf_f)
            runconf_load = True
        else:
            if arcctl_server_mode():
                logger.error('Cannot find ARC configuration file in the default location.')
            return None

    # save/load running config cache only for default arc.conf
    runconf_save = False
    if runconf_load:
        if arcctl_runtime_config is None:
            runconf_load = False
        else:
            runconf_load = os.path.exists(arcctl_runtime_config)
            runconf_save = not runconf_load
    else:
        logger.debug('Custom ARC configuration file location is specified. Ignoring cached runtime configuration usage.')


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
            if runconf_save:
                logger.debug('Saving cached parsed configuration to %s', arcctl_runtime_config)
                config.save_run_config(arcctl_runtime_config)
        arcconfig = config
        arcconfig.conf_f = conf_f
    except IOError:
        if arcctl_server_mode():
            logger.error('Failed to open ARC configuration file %s', conf_f)
        else:
            logger.debug('arcctl is working in config-less mode relying on defaults only')
        arcconfig = None
    return arcconfig

def control_path(control_dir, job_id, file_type):
    """Returns the fragmented controldir path to the job file"""
    # variable split for consistency with shell function
    job_path = ''
    for n in range(3):
        job_path += job_id[0:3] + '/'
        job_id = job_id[3:]
        if not job_id:
            break
    if job_id:
        job_path += job_id + '/'
    if not job_path:
        logger.error('The jobid "%s" does not have the right format/length', job_id)
        return ''
    return '{0}/jobs/{1}/{2}'.format(control_dir, job_path, file_type)

class ComponentControl(object):
    """ Common abstract class to ensure all implicit calls to methods are defined """
    def control(self, args):
        raise NotImplementedError()

    @staticmethod
    def register_parser(root_parser):
        raise NotImplementedError()

