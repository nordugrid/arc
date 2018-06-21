import subprocess
import logging
import requests
import sys
import os


class OSPackageManagement(object):
    """This class aimed to handle both yum (RedHat) and apt (Debian) cases"""
    def __init__(self):
        self.command_base = []  # placeholder to include command's prefix, like ssh to host
        # self.command_base = ['ssh', 'arc6.grid.org.ua']
        self.logger = logging.getLogger('ARCCTL.OSPackageManagement')
        # detect yum (will also works with dnf wrapper)
        try:
            yum_output = subprocess.Popen(self.command_base + ['yum', '--version'],
                                          stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            stdout = yum_output.communicate()
            if yum_output.returncode == 0:
                self.pm = 'yum'
                self.pm_is_installed = 'rpm -q {}'
                self.pm_cmd = 'yum'
                self.pm_repodir = '/etc/yum.repos.d/'
                self.pm_version = stdout[0].split('\n')[0]
                return
        except OSError:
            pass
        # detect apt
        try:
            apt_output = subprocess.Popen(self.command_base + ['apt-get', '--version'],
                                          stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            stdout = apt_output.communicate()
            if apt_output.returncode == 0:
                self.pm = 'apt'
                self.pm_is_installed = 'dpkg -s {}'
                self.pm_cmd = 'apt-get'
                self.pm_repodir = '/etc/apt/sources.list.d/'
                self.pm_version = stdout[0].split('\n')[0].replace('apt ', '')
                return
        except OSError:
            pass
        self.logger.error('Cannot find yum or apt-get to manage OS packages. You distribution is not supported yet.')
        sys.exit(1)

    def version(self):
        print '{} version {}'.format(self.pm, self.pm_version)

    def __get_url_content(self, url):
        try:
            r = requests.get(url)
        except requests.exceptions.RequestException as e:
            self.logger.error('Failed to fetch content from URL: %s. Error: %s', url, e.strerror)
            sys.exit(1)
        return r.content

    def deploy_apt_key(self, keyurl):
        self.logger.info('Installing PGP key for apt from %s', keyurl)
        keystr = self.__get_url_content(keyurl)
        try:
            p = subprocess.Popen(self.command_base + ['apt-key', 'add', '-'], stdin=subprocess.PIPE)
            p.communicate(input=keystr)
        except OSError as e:
            self.logger.error('Failed to install PGP key for apt from %s. Error: %s.', keyurl, e.strerror)
            sys.exit(1)

    def deploy_repository(self, repoconf):
        urlkey = self.pm + '-url'
        strkey = self.pm + '-conf'
        if urlkey in repoconf:
            url = repoconf[urlkey]
            fname = url.split('/')[-1]
            fcontent = self.__get_url_content(url)
        elif strkey in repoconf:
            fcontent = repoconf[strkey]
            fname = repoconf[self.pm + '-name']
        else:
            self.logger.error('No repository source provided in the argument. Failed to deploy repofiles.')
            sys.exit(1)

        if self.pm == 'apt' and 'apt-key-url' in repoconf:
            self.deploy_apt_key(repoconf['apt-key-url'])

        fpath = self.pm_repodir + fname
        self.logger.info('Saving repository configuration to %s', fpath)
        try:
            with open(fpath, 'wb') as f:
                f.write(fcontent)
        except (IOError, OSError) as e:
            self.logger.error('Failed to save repository configuration to %s. Error: %s', fpath, e.strerror)
            sys.exit(1)

    def update_cache(self):
        if self.pm == 'yum':
            command = self.command_base + ['yum', 'makecache']
        elif self.pm == 'apt':
            command = self.command_base + ['apt-get', 'update']
        self.logger.info('Updating packages metadata from repositories')
        return subprocess.call(command)

    def install(self, packages):
        # no underscores according to Debian naming policy: https://www.debian.org/doc/debian-policy/
        if self.pm == 'apt':
            packages = list(map(lambda p: p.replace('_', '-'), packages))
        # install
        command = self.command_base + [self.pm_cmd, '-y', 'install'] + packages
        self.logger.info('Running the following command to install packages: %s', ' '.join(command))
        return subprocess.call(command)

    def is_installed(self, package):
        __DEVNULL = open(os.devnull, 'w')
        command = self.command_base + self.pm_is_installed.format(package).split()
        self.logger.debug('Running the following command to check package is installed: %s', ' '.join(command))
        return subprocess.call(command, stdout=__DEVNULL, stderr=subprocess.STDOUT) == 0


