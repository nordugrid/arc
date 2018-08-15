"""
Module for job cancelling.
"""

from __future__ import absolute_import

from .config import Config
from .log import debug, error
from .ssh import ssh_connect
from .proc import *

def cancel(cmd, jobid):
     """
     Cancel job with ID ``jobid`` in the given batch system.

     :param list cmd: first item should be executable, other items should be arguments to executable in order
     :param str jobid: local job ID in the batch system
     :return: command return code
     :rtype: :py:obj:`int`
     """

     if Config.remote_host:
          ssh_connect(Config.remote_host, Config.remote_user, Config.private_key)

     from os.path import basename
     executable = basename(cmd[0])
     debug('executing %s with job id %s' % (executable, jobid), 'common.cancel')
     execute = execute_local if not Config.remote_host else execute_remote
     handle = execute(' '.join(cmd))
     rc = handle.returncode

     if rc:
          error('%s failed' % executable, 'common.cancel')
     return not rc
