"""
Wrapper methods for stating, reading and writing files.
"""

from __future__ import absolute_import

import os
from .ssh import SSHSession
from .log import warn, error

def read(path, do_tail = 0):
    """
    Read file content.

    :param str path: path to file
    :param int tail: number of lines to read (from end), entire file if 0
    :return: file content splitted by newline
    :rtype: :py:obj:`list` [ :py:obj:`str` ... ]
    """

    try:
        if do_tail:
            return tail(path, 1000)
        else:
            with os.fdopen(os.open(path, os.O_RDONLY | os.O_NONBLOCK)) as f:
                return f.readlines()
    except Exception as e:
        warn('Cannot read file at %s:\n%s' % (path, str(e)), 'common.files')
        return []


def tail(path, n, BUFSIZ = 4096):
    """
    Similar to GNU tail -n [N].

    :param str path: path to file
    :param int n: number of lines to read
    :param int BUFSIZ: chunk size in bytes (default: 4096)
    """

    import os
    fsize = os.stat(path)[6]
    block = -1
    lines = 0
    data = ''
    with os.fdopen(os.open(path, os.O_RDONLY | os.O_NONBLOCK), 'rb') as f:
        f.seek(0,2)
        pos = f.tell()
        while pos > 0 and lines < n:
            if (pos - BUFSIZ) > 0:
                # seek back one BUFSIZ
                f.seek(block*BUFSIZ, 2)
                # read buffer
                new_data = f.read(BUFSIZ)
                data = new_data + data
                lines += new_data.count('\n')
            else:
                # file too small, start from beginning
                f.seek(0,0)
                # only read what was not read
                data = f.read(pos) + data
                break
            pos -= BUFSIZ
            block -= 1
    # return n last lines of file
    return data.split('\n')[-n:]


def write(path, buf, mode = 0o644, append = False, remote_host = None):
    """
    Write buffer to file.

    :param str path: path to file
    :param str buf: buffer
    :param int mode: file mode
    :param bool append: ``True`` if buffer should be appended to existing file
    :param bool remote_host: file will be opened with sftp at the specified hostname
    """

    w_or_a = 'a' if append else 'w'
    try:
        if remote_host:
            open_file = SSHSession[remote_host].open_sftp_client().file(path, w_or_a)
            open_file.chmod(mode)
            open_file.write(buf)
            open_file.close()
        else:
            with os.fdopen(os.open(path, os.O_WRONLY | os.O_CREAT, mode | 0x8000), w_or_a) as f:
                f.write(buf)

    except Exception as e:
        warn('Cannot write to file at %s:\n%s' % (path, str(e)), 'common.files')
        return False

    return True


def getmtime(path):
    """
    Get modification time of ``path``.

    :param str path: path to file
    :return str: modification time
    """

    try:
        return os.path.getmtime(path)
    except:
        error('Failed to stat file: %s\n%s' % (path, str(e)), 'common.files')
        return False
