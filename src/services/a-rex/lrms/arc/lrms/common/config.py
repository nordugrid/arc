"""
Provides the ``Config`` object, with each arc.conf option as an attribute.
"""

from __future__ import absolute_import

import sys, os, socket

import arc
from .log import debug
from .files import getmtime

class _object(object): pass # Pickle can't serialize inline classes

Config = _object()


def is_conf_setter(f):
    """
    Decorator for functions that set :py:data:`~lrms.common.Config` attributes.
    """
    f.is_conf_setter = True
    return f


def configure(configfile, *func):
    """
    Parse arc.conf using :py:meth:`ConfigParser.RawConfigParser.read` and
    set all dicovered options as :py:data:`~lrms.common.Config` attributes. 
    Additional setter functions ``*func`` will only be executed if they have
    been decorated with :py:meth:`lrms.common.config.is_conf_setter`.

    The ``Config`` object will be pickled to ``/tmp/python_lrms_arc.conf``.
    In case the pickle file exists and its modification time is newer than
    that of arc.conf, the ``Config`` object will be loaded from this file.
    
    :param str configfile: path to arc.conf
    :param func: variable length setter function list - functions must be
      decorated with :py:meth:`lrms.common.config.is_conf_setter`.
    :type func: :py:obj:`list` [ :py:obj:`function` ... ]
    """

    os.environ["ARC_CONFIG"] = configfile

    import pickle
    pickle_conf = '/tmp/python_lrms_arc.conf'
    global Config

    try:
        if not (getmtime(pickle_conf) > getmtime(configfile)):
            raise Exception
        debug('Loading pickled config from ' + pickle_conf, 'common.config')
        with open(pickle_conf, 'rb') as f:
            Config = pickle.load(f)
    except:
        try:
            import configparser as ConfigParser
        except ImportError:
            import ConfigParser
        cfg = ConfigParser.ConfigParser()
        cfg.read(configfile)

        set_common(cfg)
        set_gridmanager(cfg)
        set_cluster(cfg)
        set_queue(cfg)
        for fun in func:
            getattr(fun, 'is_conf_setter', False) and fun(cfg)
        try:
            with open(pickle_conf, 'wb') as f:
                pickle.dump(Config, f)
        except IOError:
            pass


def set_common(cfg):
    """
    Set [common] :py:data:`~lrms.common.Config` attributes.

    :param cfg: parsed arc.conf
    :type cfg: :py:class:`ConfigParser.ConfigParser`
    """

    global Config
    Config.hostname = str(cfg.get('common', 'hostname')).strip('"') \
        if cfg.has_option('common', 'hostname') else socket.gethostname()


def set_gridmanager(cfg):
   """
   Set [arex] :py:data:`~lrms.common.Config` attributes.

   :param cfg: parsed arc.conf
   :type cfg: :py:class:`ConfigParser.ConfigParser`
   """

   global Config

   # log threshold
   Config.log_threshold = arc.INFO
   if cfg.has_option('arex', 'debug'):
       try:
           value = int(cfg.get('arex', 'debug')).strip('"')
           Config.log_threshold = (arc.FATAL, arc.ERROR, arc.WARNING, arc.INFO, arc.VERBOSE, arc.DEBUG)[value]
       except:
           pass
   # joboption_directory
   Config.sessiondir = str(cfg.get('arex', 'sessiondir')).strip('"') \
       if cfg.has_option('arex', 'sessiondir') else ''
   Config.controldir = str(cfg.get('arex', 'controldir')).strip('"') \
       if cfg.has_option('arex', 'controldir') else ''
   Config.runtimedir = str(cfg.get('arex', 'runtimedir')).strip('"') \
       if cfg.has_option('arex', 'runtimedir') else ''
   # RUNTIME_FRONTEND_SEES_NODE
   Config.shared_scratch = \
       str(cfg.get('arex', 'shared_scratch')).strip('"') \
       if cfg.has_option('arex', 'shared_scratch') else ''
   # RUNTIME_NODE_SEES_FRONTEND
   Config.shared_filesystem = \
       not cfg.has_option('arex', 'shared_filesystem') or \
       str(cfg.get('arex', 'shared_filesystem')).strip('"') != 'no'
   # RUNTIME_LOCAL_SCRATCH_DIR
   Config.scratchdir = \
       str(cfg.get('arex', 'scratchdir')).strip('"') \
       if cfg.has_option('arex', 'scratchdir') else ''
   Config.scanscriptlog = \
       str(cfg.get('arex', 'logfile')).strip('"') \
       if cfg.has_option('arex', 'logfile') \
       else '/var/log/arc/arex.log'
   Config.gnu_time = \
       str(cfg.get('lrms', 'gnu_time')).strip('"') \
       if cfg.has_option('lrms', 'gnu_time') else '/usr/bin/time'
   Config.nodename = \
       str(cfg.get('lrms', 'nodename')).strip('"') \
       if cfg.has_option('lrms', 'nodename') else '/bin/hostname -f'
   # SSH
   from pwd import getpwuid
   Config.remote_host = \
       str(cfg.get('ssh', 'remote_host')).strip('"') \
       if cfg.has_option('ssh', 'remote_host') else ''
   Config.remote_user = \
       str(cfg.get('ssh', 'remote_user')).strip('"') \
       if cfg.has_option('ssh', 'remote_user') \
       else getpwuid(os.getuid()).pw_name
   Config.private_key = \
       str(cfg.get('ssh', 'private_key')).strip('"') \
       if cfg.has_option('ssh', 'private_key') \
       else os.path.join('/home', getpwuid(os.getuid()).pw_name, '.ssh', 'id_rsa')
   Config.remote_endpoint = \
       'ssh://%s@%s:22' % (Config.remote_user, Config.remote_host) \
       if Config.remote_host else ''
   Config.remote_sessiondir = \
       str(cfg.get('ssh', 'remote_sessiondir')).strip('"') \
       if cfg.has_option('ssh', 'remote_sessiondir') else ''
   Config.remote_runtimedir = \
       str(cfg.get('ssh', 'remote_runtimedir')).strip('"') \
       if cfg.has_option('ssh', 'remote_runtimedir') else ''
   Config.ssh_timeout = \
       int(cfg.get('ssh', 'ssh_timeout')).strip('"') \
       if cfg.has_option('ssh', 'ssh_timeout') else 10


def set_cluster(cfg):
    """
    Set [cluster] :py:data:`~lrms.common.Config` attributes.
    
    :param cfg: parsed arc.conf
    :type cfg: :py:class:`ConfigParser.ConfigParser`
    """

    global Config
    Config.gm_port = 2811
    Config.gm_mount_point = '/jobs'
    Config.defaultmemory = int(cfg.get('lrms', 'defaultmemory').strip('"')) \
        if cfg.has_option('lrms', 'defaultmemory') else 0
    Config.nodememory = int(cfg.get('infosys/cluster', 'nodememory').strip('"')) \
        if cfg.has_option('infosys/cluster', 'nodememory') else 0
    Config.hostname = str(cfg.get('common', 'hostname')).strip('"') \
        if cfg.has_option('common', 'hostname') else socket.gethostname()
       

def set_queue(cfg):
    """
    Set [queue] :py:data:`~lrms.common.Config` attributes.
    
    :param cfg: parsed arc.conf
    :type cfg: :py:class:`ConfigParser.ConfigParser`
    """

    global Config
    Config.queue = {}

    for section in cfg.sections():
        if section[:6] != 'queue:' or not section[6:]:
            continue
        name = section[6:].strip()
        if name not in Config.queue:
            Config.queue[name] = _object()
            if cfg.has_option(section, 'defaultmemory'):
                Config.queue[name].defaultmemory = \
                    int(cfg.get(section, 'defaultmemory').strip('"'))

