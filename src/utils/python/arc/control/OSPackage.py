import subprocess
import logging
import sys


class OSPackageManagement(object):
    """This class aimed to handle both yum (RedHat) and apt (Debian) cases"""
    def __init__(self):
        self.command_base = []  # placeholder to include command's prefix, like ssh to host
        self.logger = logging.getLogger('ARCCTL.OSPackageManagement')
        # detect yum (will also works with dnf wrapper)
        try:
            yum_output = subprocess.Popen(self.command_base + ['yum', '--version'],
                                          stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            stdout = yum_output.communicate()
            if yum_output.returncode == 0:
                self.pm_version = stdout[0].split('\n')[0]
                self.pm = 'yum'
                return
        except OSError:
            pass
        # detect apt
        try:
            apt_output = subprocess.Popen(self.command_base + ['apt-get', '--version'],
                                          stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            stdout = apt_output.communicate()
            if apt_output.returncode == 0:
                self.pm_version = stdout[0].split('\n')[0].replace('apt ', '')
                self.pm = 'apt'
                return
        except OSError:
            pass
        self.logger.error('Cannot find both yum and apt-get to manage OS packages.')
        sys.exit(1)

    def version(self):
        print '{} version {}'.format(self.pm, self.pm_version)


