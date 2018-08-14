"""
Provides the ``SSHSession`` dictionary, which maps host
to :py:class:`paramiko.transport.Transport`.
"""

from __future__ import absolute_import

from .log import ArcError

SSHSession = {}

def ssh_connect(host, user, pkey, window_size = (2 << 15) - 1):
    """
    Creates a :py:class:`paramiko.Transport` object and adds it to
    ``SSHSession``.

    :param str host: remote host
    :param str user: username at remote host
    :param str pkey: path to private RSA key
    :param int window_size: TCP window size
    :note: if command execution times out and output is truncated, it is likely that the TCP window is too small
    """
    from paramiko.transport import Transport
    from paramiko import RSAKey

    global SSHSession
    try:
        SSHSession[host] = Transport((host, 22))
        SSHSession[host].window_size = window_size
        pkey = RSAKey.from_private_key_file(pkey, '')
        SSHSession[host].connect(username = user, pkey = pkey)
        SSHSession[host].__del__ = SSHSession[host].close
    except Exception as e:
        raise ArcError('Failed to connect to host %s:\n%s' % (host, str(e)), 'common.ssh')
