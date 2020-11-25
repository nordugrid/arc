from socket import socket, getfqdn, AF_INET, SOCK_DGRAM
import glob
import logging
import os
import re
import stat
import struct
import subprocess
import time
from arc.paths import ARC_DATA_DIR

class Validator(object):
    """Class for validating host setup and arc configuration"""

    # List of blocks which have user-defined subblocks
    __dynamic_blocks = ["userlist",
                        "authgroup",
                        "arex/jura/sgas",
                        "arex/jura/apel",
                        "queue",
                        "custom"]

    # Mandatory blocks
    __mandatory_blocks = ["common",
                          "mapping",
                          "arex",
                          "infosys",
                          "infosys/glue2",
                          "lrms"]

    # Options which are allowed to have an empty value
    __empty_value_options = ["mapped_unixid",
                             "joblog",
                             "speedcontrol"]

    # Blocks and options removed from arc.conf.reference but still allowed to pass validation
    __deprecated_blocks = ["arex/jura/archiving"]
    __deprecated_options = {"arex/jura": ["urdelivery_keepfailed"],
                            "arex/jura/sgas": ["legacy_fallback"],
                            "arex/jura/apel": ["use_ssl",
                                               "benchmark_type",
                                               "benchmark_value",
                                               "benchmark_description",
                                               "legacy_fallback"]
                           }

    def __init__(self, arcconf, arcconffile):
        self.logger = logging.getLogger('ARCCTL.Validator')
        self.arcconf = arcconf
        conffile = arcconffile or os.environ.get('ARC_CONFIG') or '/etc/arc.conf'
        # Force re-parsing of arc.conf to set defaults
        self.arcconf.parse_arc_conf(conffile)
        self.errors = 0
        self.warnings = 0

    def error(self, msg):
        """Log an error and increase the error count"""
        self.errors += 1
        self.logger.error(msg)

    def warning(self, msg):
        """Log a warning and increase the warning count"""
        self.warnings += 1
        self.logger.warning(msg)

    def validate(self):
        """Run all the validation checks"""
        self.errors = 0
        self.warnings = 0
        self.validate_time()
        self.validate_configuration()
        self.validate_certificates()

    def validate_time(self):
        """Check host time against NTP server"""
        time_server = 'europe.pool.ntp.org'
        try:
            ntp_time = self._get_ntp_time(time_server)
        except Exception as exc:
            self.warning("Unable to get time from europe.pool.ntp.org: %s" % str(exc))
            return

        time_diff = abs(ntp_time - time.time())
        if time_diff > 60:
            self.error("Time differs by %d seconds from the time server %s" %
                       (time_diff, time_server))
        elif time_diff > 1:
            self.warning("Time differs by %d seconds from the time server %s" %
                         (time_diff, time_server))

    def _get_ntp_time(self, time_server):
        port = 123
        buf = 1024
        address = (time_server, port)
        msg = '\x1b' + 47 * '\0'

        # reference time (in seconds since 1900-01-01 00:00:00)
        time1970 = 2208988800 # 1970-01-01 00:00:00

        # connect to server
        client = socket(AF_INET, SOCK_DGRAM)
        client.settimeout(10)
        client.sendto(msg.encode('utf-8'), address)
        msg, address = client.recvfrom(buf)
        ntp_t = struct.unpack("!12I", msg)[10]
        ntp_t -= time1970
        return ntp_t

    def validate_configuration(self):
        """Check everything in the configuration file is ok"""
        config_dict = self.arcconf.get_config_dict()
        config_defaults = self.arcconf.get_default_config_dict()
        self._check_config_blocks(config_dict, config_defaults)
        self._extra_config_checks(config_dict)

    def _check_config_blocks(self, config_dict, config_defaults):

        for block, options in config_dict.items():
            # Spaces are not allowed in block names except after :
            if re.search(r'[^:]\s', block):
                self.error("Whitespace not allowed in [%s]" % block)

            # Check if block is deprecated
            if block in self.__deprecated_blocks:
                self.warning("The [%s] block is deprecated" % block)
                continue

            # Check if block is in the list of dynamic blocks
            is_dynamic = [b for b in self.__dynamic_blocks if re.match(b, block)]
            if is_dynamic:
                block = is_dynamic[0]

            # Check if block is allowed
            if block not in config_defaults:
                self.error("Unknown block [%s]" % block)
                continue

            # Ignore custom blocks
            if block == 'custom':
                continue

            # Check for options allowed in this block
            for option, value in options.items():
                if option not in config_defaults[block]['__options']:
                    self.warning("'%s' is not a valid option in [%s]" % (option, block))
                    continue

                if not value and option not in self.__empty_value_options:
                    self.error("%s option in [%s] has an empty value" % (option, block))
                    continue

                if not isinstance(value, list):
                    value = [value]

                for val in value:
                    if re.match(r'".*"', val) or re.match(r"'.*'", val):
                        self.error("Values must not be enclosed in quotes (%s=%s in [%s])" %
                                   (option, val, block))

                    self._check_config_option(block, option, val, config_defaults)


    def _check_config_option(self, block, option, value, config_defaults):

        def _default_value(block, option):
            # Return the default value of the given option
            try:
                return config_defaults[block]['__values'] \
                       [config_defaults[block]['__options'].index(option)]
            except ValueError:
                return None

        # Extra checks for certain options
        if block == 'arex/cache' and option == 'cachedir':
            if not os.path.exists(value.split()[0]):
                self.warning("cachedir doesn't exist at %s" % value.split()[0])

        if block == 'arex' and option == 'sessiondir':
            if not os.path.exists(value.split()[0]):
                self.warning("sessiondir doesn't exist at %s" % value.split()[0])
            if len(value.split()) == 2 and value.split()[1] != 'drain':
                self.error("Second option in sessiondir must be 'drain' or empty")

        if block == 'arex' and option == 'controldir':
            if not os.path.exists(value):
                self.warning("controldir doesn't exist at %s" % value)
            elif os.stat(value).st_mode & 0o022:
                self.error("%s must have permissions 0700 or 0755" % value)

        if block == 'arex' and option == 'tmpdir':
            if not os.path.exists(value):
                self.warning("tmpdir doesn't exist at %s" % value)
            elif not os.stat(value).st_mode & stat.S_ISVTX:
                self.error("tmpdir %s must have the sticky bit set" % value)

        if option in ('logfile', 'pidfile'):
            default_file = _default_value(block, option)
            # Optional params have the default value "undefined"
            if value != default_file and default_file != 'undefined':
                self.warning("%s is not in the default location (%s). " \
                             "Please adjust log rotation as necessary" % (value, default_file))

        if block == 'infosys/ldap':
            if option == 'slapd_loglevel' and value != '0':
                self.warning('slapd_loglevel > 0 should only be used for development. ' \
                             'DO NOT USE for production and testing. ' \
                             'Configure bdii_debug_level=DEBUG instead.')
            if option.startswith('bdii') \
              and option != 'bdii_debug_level' \
              and value != _default_value(block, option) \
              and '$VAR' not in _default_value(block, option):
                self.warning('%s is set. BDII configuration commands are strongly deprecated. ' \
                             'Use only for Development. ' \
                             'The only allowed command is bdii_debug_level' % option)

        # check compliance of AdminDomain name as LDAP DN
        if block == 'infosys/glue2' and option == 'admindomain_name':
            if value == 'UNDEFINEDVALUE':
                self.warning('admindomain_name not configured in [infosys/glue2]. ' \
                             'Default AdminDomain name is UNDEFINEDVALUE')
            syntax_errors = []
            # bad symbols from RFC 4514
            for char in ('"', '+', ',', ';', '<', '>', '\\'):
                if char in value:
                    syntax_errors.append("Character '%s' not allowed" % char)
            if re.match(r'^[ #]', value):
                syntax_errors.append('Blank space or "#" at the beginning of admindomain_name ' \
                                     'are not allowed')
            if re.search(r'\s$', value):
                syntax_errors.append('Blank space at the end of name is not allowed')
            if syntax_errors:
                self.error('admindomain_name in [infosys/glue2] contains the following errors:' \
                           ' %s. \n\tLDAP Infosys will not start. \n\t' \
                           'Please change AdminDomain name accordingly' % ', '.join(syntax_errors))


    def _extra_config_checks(self, config_dict):
        # Check for mandatory blocks
        for block in self.__mandatory_blocks:
            if block not in config_dict:
                self.error("Mandatory block [%s] is missing" % block)

        # Check hostname setting matches actual hostname
        hostname = getfqdn()
        if hostname != config_dict['common']['hostname']:
            self.warning('The hostname set in arc.conf (%s) does not match ' \
                         'the actual hostname (%s)' \
                         % (config_dict['common']['hostname'], hostname))

        # Mapping cannot be empty if running as root
        if os.getuid() == 0 and not config_dict['mapping']:
            self.error("[mapping] block cannot be empty if running as root")

        # Warn if no allow/denyaccess blocks are present in interface blocks
        if 'arex/ws/jobs' in config_dict and 'allowaccess' not in config_dict['arex/ws/jobs'] \
          and 'denyaccess' not in config_dict['arex/ws/jobs']:
            self.warning("No allowaccess or denyaccess defined in [arex/ws/jobs]. " \
                         "Interface will be open for all mapped users")
        if 'gridftpd/jobs' in config_dict and 'allowaccess' not in config_dict['gridftpd/jobs'] \
          and 'denyaccess' not in config_dict['gridftpd/jobs']:
            self.warning("No allowaccess or denyaccess defined in [gridftpd/jobs]. " \
                         "Interface will be open for all mapped users")

        # Check lrms name is correct
        lrms_submit = os.path.join(ARC_DATA_DIR, 'submit-%s-job' % config_dict['lrms']['lrms'])
        if not os.path.exists(lrms_submit):
            # Special exception for slurm/SLURM - maybe remove in ARC 7
            if config_dict['lrms']['lrms'].lower() != 'slurm':
                self.error("%s is not an allowed lrms name" % config_dict['lrms']['lrms'])

    def validate_certificates(self):
        """Check the certificate setup is ok"""
        config_dict = self.arcconf.get_config_dict()
        x509_host_cert = config_dict['common']['x509_host_cert']
        x509_host_key = config_dict['common']['x509_host_key']
        x509_cert_dir = config_dict['common']['x509_cert_dir']

        # Check cert
        if not os.path.exists(x509_host_cert):
            self.error("%s does not exist" % x509_host_cert)
        else:
            # Verify cert
            try:
                output = subprocess.check_output(["openssl", "verify", "-CApath",
                                                  x509_cert_dir, x509_host_cert])
                self.logger.info(output.decode().strip())
            except subprocess.CalledProcessError as cpe:
                self.error("Host certificate verification failed: %s" % cpe.output)
            else:
                # Check expiration date, warn if less than one week away
                enddate = subprocess.check_output(["openssl", "x509", "-enddate", "-noout", "-in",
                                                   x509_host_cert]).decode().strip().split('=')[-1]
                # Redirect output (-noout doesn't work in openssl1.1.1)
                # With python3-only we can use subprocess.DEVNULL
                with open(os.devnull, 'w') as dev_null:
                    if subprocess.call(["openssl", "x509", "-checkend", "604800", "-noout",
                                        "-in", x509_host_cert], stdout=dev_null, stderr=dev_null):
                        self.warning("Host certificate will expire on %s" % enddate)
                    else:
                        self.logger.info("Host certificate will expire on %s", enddate)

        # Check key
        if not os.path.exists(x509_host_key):
            self.error("%s does not exist" % x509_host_key)
        elif os.stat(x509_host_key).st_mode & 0o177:
            self.error("%s must have permissions 0600 or 0400" % x509_host_key)
        elif not os.access(x509_host_key, os.R_OK):
            self.error("%s is not owned by this user" % x509_host_key)

        # Check CA dir
        if not os.path.isdir(x509_cert_dir):
            self.error("Directory %s does not exist" % x509_cert_dir)
        else:
            # Check CRLs
            crls = glob.glob(os.path.join(x509_cert_dir, "*.r0"))
            if not crls:
                self.warning("No certificate revocation lists in %s" % x509_cert_dir)
            else:
                now = time.time()
                for crl in crls:
                    if now - os.path.getmtime(crl) > 2 * 86400:
                        self.warning("%s is older than 2 days, rerun fetch-crl" % crl)
