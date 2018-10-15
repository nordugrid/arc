import subprocess
import logging
import sys
import os


class OSServiceManagement(object):
    """This class aimed to handle both Systemd and SysV cases"""
    def __init__(self, scripts_path=None):
        self.command_base = []  # placeholder to include command's prefix, like ssh to host
        # self.command_base = ['ssh', 'arc6.grid.org.ua']
        self.logger = logging.getLogger('ARCCTL.OSServiceManagement')
        # scripts_path is defined - no OS service management, only scripts
        if scripts_path is not None:
            self.logger.debug('Direct startup scripts invocation is requested instead of OS service management')
            self.sm = 'manual'
            self.sm_ctl = {
                'start': scripts_path + '{0} start',
                'stop': scripts_path + '{0} stop',
                'enable': 'true',  # no enable/disable in manual mode
                'disable': 'true'
            }
            self.sm_check = {
                'enabled': 'false',
                'installed': 'test -f ' + scripts_path + '{0}',
                'active': scripts_path + '{0} status'
            }
            self.sm_version = ''
            return
        # detect systemctl
        try:
            systemctl_output = subprocess.Popen(self.command_base + ['systemctl', '--version'],
                                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            stdout = systemctl_output.communicate()
            if systemctl_output.returncode == 0:
                self.sm = 'systemd'
                self.sm_ctl = {
                    'start': 'systemctl start {0}',
                    'stop': 'systemctl stop {0}',
                    'enable': 'systemctl enable {0}',
                    'disable': 'systemctl disable {0}'
                }
                self.sm_check = {
                    'enabled': 'systemctl -q is-enabled {0}',
                    'installed': 'systemctl status {0}',
                    'active': 'systemctl -q is-active {0}'
                }
                self.sm_version = stdout[0].split('\n')[0]
                self.logger.debug('Managing OS services using %s version %s', self.sm, self.sm_version)
                return
        except OSError:
            pass
        # detect apt
        try:
            service_output = subprocess.Popen(self.command_base + ['service', '--version'],
                                          stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            stdout = service_output.communicate()
            if service_output.returncode == 0:
                self.sm = 'sysV'
                self.sm_ctl = {
                    'start': 'service {0} start',
                    'stop': 'service {0} stop',
                    'enable': 'chkconfig {0} on',
                    'disable': 'chkconfig {0} off'
                }
                self.sm_check = {
                    'enabled': 'chkconfig {0}',
                    'installed': 'chkconfig --list {0}',
                    'active': 'service {0} status'
                }
                self.sm_version = stdout[0].split('\n')[0]
                self.logger.debug('Managing OS services using %s version %s', self.sm, self.sm_version)
                return
        except OSError:
            pass
        self.logger.error('Cannot find systemctl or service to manage OS services. '
                          'You distribution is not supported yet.')
        sys.exit(1)

    def __exec_service_cmd(self, service, action):
        action_str = self.sm_ctl[action]
        command = self.command_base + action_str.format(service).split()
        self.logger.info('Running the following command to %s service: %s', action, ' '.join(command))
        try:
            return subprocess.call(command)
        except OSError:
            return 1

    def __check_service(self, service, check):
        __DEVNULL = open(os.devnull, 'w')
        check_str = self.sm_check[check]
        command = self.command_base + check_str.format(service).split()
        try:
            self.logger.debug('Running the following command to check service is %s: %s', check, ' '.join(command))
            return subprocess.call(command, stdout=__DEVNULL, stderr=subprocess.STDOUT) == 0
        except OSError:
            return False

    def enable(self, service):
        return self.__exec_service_cmd(service, 'enable')

    def disable(self, service):
        return self.__exec_service_cmd(service, 'disable')

    def start(self, service):
        return self.__exec_service_cmd(service, 'start')

    def stop(self, service):
        return self.__exec_service_cmd(service, 'stop')

    def restart(self, service):
        stop_status = self.__exec_service_cmd(service, 'stop')
        if stop_status != 0:
            return stop_status
        return self.__exec_service_cmd(service, 'start')

    def is_enabled(self, service):
        return self.__check_service(service, 'enabled')

    def is_active(self, service):
        return self.__check_service(service, 'active')

    def is_installed(self, service):
        return self.__check_service(service, 'installed')
