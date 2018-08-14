"""
Execute bash commands locally or remotely (SSH).

:todo: split long list of args so that we don't exceed kernel ARG_MAX
"""

from __future__ import absolute_import

from .log import warn, ArcError
from .config import Config
from .ssh import SSHSession


def sliceargs(args, MAX = 4000):
    subargs = args[:MAX]
    i = 1
    while len(subargs) == 4000:
        subargs = args[i*MAX:(i+1)*MAX]
        yield subargs


def execute_local(args, env = None, zerobyte = False):
    """
    Execute a command locally. This method is a wrapper for
    :py:class:`subprocess.Popen` with stdout and stderr piped to
    temporary files and ``shell=True``.

    :param str args: command with arguments (e.g. 'sbatch myjob.sh')
    :param dict env: environment variables  (default: {})
    :return: object with attributes ``stdout``, ``stderr`` \
    and ``returncode``
    :rtype: :py:obj:`object`
    """

    from tempfile import TemporaryFile
    from subprocess import Popen

    # Note: PIPE will cause deadlock if output is larger than 65K
    stdout, stderr = TemporaryFile("w+"), TemporaryFile("w+")
    handle = type('Handle', (object,), {'stdout' : [], 'stderr' : [], 'returncode' : 0})()
    p = Popen(args, stdout = stdout, stderr = stderr, env = env, shell = True)
    p.wait()
    if zerobyte:
        strstdout = stdout.seek(0) or stdout.read()
        handle.stdout = strstdout.split('\0')
    else:
        handle.stdout = stdout.seek(0) or stdout.readlines()
    handle.stderr = stderr.seek(0) or stderr.readlines()
    handle.returncode = p.returncode
    return handle


def execute_remote(args, host = None, timeout = 10):
    """
    Execute a command on the remote host using the SSH protocol.

    :param str args: command with arguments (e.g. 'sbatch myjob.sh')
    :return: object with attributes ``stdout``, ``stderr`` \
    and ``returncode``
    :rtype: :py:obj:`object`
    """

    from time import sleep

    timeout = Config.ssh_timeout
    def is_timeout(test):
        wait_time = 0
        while not test():
            sleep(0.5)
            wait_time += 0.5
            if wait_time > timeout:
                return True
        return False

    try:
        handle = type('Handle', (object,), {'stdout' : [], 'stderr' : [], 'returncode' : 0})()
        if not SSHSession:
            raise ArcError('There is no active SSH session! Run lrms.common.ssh.ssh_connect', 'common.proc')
        session = SSHSession[host if host else list(SSHSession.keys())[-1]].open_session()
        session.exec_command(args)
        if is_timeout(session.exit_status_ready):
            warn('Session timed out. Some output might not be received. Guessing exit code from stderr.', 'common.proc')
        handle.returncode = session.exit_status

        chnksz = 2 << 9

        stdout = ''
        data = session.recv(chnksz)
        while data:
            stdout += data
            data = session.recv(chnksz)
        handle.stdout = stdout.split('\n')

        stderr = ''
        data = session.recv_stderr(chnksz)
        while data:
            stderr += data
            data = session.recv_stderr(chnksz)
        handle.stderr = stderr.split('\n')

        if handle.returncode == -1:
            handle.returncode = len(stderr) > 0
        return handle

    except Exception as e:
        raise ArcError('Failed to execute command \'%s (...) \':\n%s' % (args.split()[:4], str(e)), 'common.proc')
