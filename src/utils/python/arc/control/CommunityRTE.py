from __future__ import print_function
from __future__ import absolute_import

from .ControlCommon import *
import os
import sys
import shutil
import re
import base64
import json
try:
    from urllib.request import Request, urlopen
    from urllib.error import URLError
except ImportError:
    from urllib2 import Request, urlopen
    from urllib2 import URLError

# gpg command is widely available vs python-gnupg dedicated package
# to eliminate extra dependency we are relying on invoking the command
import subprocess


class CommunityRTEControl(ComponentControl):
    def __init__(self, arcconfig):
        self.logger = logging.getLogger('ARCCTL.RunTimeEnvironment.Community')
        if arcconfig is None:
            self.logger.critical('Controlling RunTime Environments is not possible without arc.conf defined controldir')
            sys.exit(1)
        self.control_rte_dir = arcconfig.get_value('controldir', 'arex').rstrip('/') + '/rte'
        # get first sessiondir that is not draining now
        self.sessiondir = None
        for sdc in arcconfig.get_value('sessiondir', 'arex', force_list=True):
            if sdc.endswith(' drain'):
                self.logger.debug('Session directory %s is draining now. Skipping.')
                continue
            self.sessiondir = sdc.rstrip('/')
        # shared sessiondir
        self.session_shared = arcconfig.get_value('shared_filesystem', 'arex', bool_yesno=True)
        # define communities directory
        self.community_rte_dir = self.control_rte_dir + '/community'
        if not os.path.exists(self.community_rte_dir):
            os.makedirs(self.community_rte_dir, mode=0o755)
        # get added (trusted) communities
        self.communities = []
        for _, c, _ in os.walk(self.community_rte_dir):
            self.communities = c
            break

    def __fetch_data(self, url):
        """Fetch data from file:/// or http(s):// sources"""
        if not url.startswith('file:///'):
            # check URL is http(s) valid URL
            __uri_re = re.compile(r'^(?P<protocol>[^:]+)://(?P<host>[^:/]+)(?::(?P<port>[0-9]+))?/*.*')
            uri_data = __uri_re.match(url)
            if not uri_data:
                self.logger.error('Cannot parse the provided data (%s) as URL.', url)
                sys.exit(1)
            uri_parms = uri_data.groupdict()
            if uri_parms['protocol'] != 'http' and uri_parms['protocol'] != 'https' and uri_parms['protocol'] != 'file':
                self.logger.error('Protocol %s is not supported.', uri_parms['protocol'])
                sys.exit(1)
        # fetch data
        self.logger.debug('Fetching data from %s', url)
        req = Request(url)
        try:
            response = urlopen(req)
        except URLError as e:
            if hasattr(e, 'reason'):
                self.logger.error('Failed to fetch data from %s. Error: %s', url, e.reason)
            else:
                self.logger.error('Failed to fetch data from %s. Error code: %s', url, e.code)
            return None
        # get response
        return response.read().decode('utf-8')

    def __get_community_json(self, url):
        # fetch URL data
        urldata = self.__fetch_data(url)
        if urldata is None:
            self.logger.error('Failed to fetch software registry description.')
            return None
        # decode it as JSON
        try:
            data = json.loads(urldata)
        except ValueError:
            self.logger.error('Failed to decode software registry description as a valid JSON')
            return None
        if data is None:
            self.logger.error('Community software registry description is empty. Failed to add community.')
        return data

    def __get_keyfile(self, cdir, url):
        keydata = self.__fetch_data(url)
        if keydata is None:
            self.logger.error('Failed to fetch community public key.')
            return None
        keyfile = os.path.join(cdir, 'pubkey.gpg')
        with open(keyfile, 'w') as key_f:
            key_f.write(keydata)
        return keyfile

    def __cdir_cleanup(self, c, exitcode=1):
        cdir = os.path.join(self.community_rte_dir, c)
        shutil.rmtree(cdir)
        sys.exit(exitcode)

    def __ask_yes_no(self, question, default_yes=False):
        yes_no = ' (YES/no): ' if default_yes else ' (yes/NO): '
        reply = str(raw_input(question + yes_no)).lower().strip()
        if reply == 'yes':
            return True
        if reply == 'no':
            return False
        if reply == '':
            return default_yes
        return self.__ask_yes_no("Please type 'yes' or 'no'", default_yes)

    def add(self, args):
        c = args.community
        if c in self.communities:
            self.logger.error('Cannot add community. Community %s is already trusted.', c)
            # TODO: maybe more consistency checks are needed? e.g. check gpg and config is there
            sys.exit(1)
        # create community directory
        cdir = os.path.join(self.community_rte_dir, c)
        os.makedirs(cdir, mode=0o755)
        # software directory
        cconfig = {
            'SOFTWARE_DIR': {
                'description': 'Path to community software installation directory',
                'value': ''
            },
            'SOFTWARE_SHARED': {
                'description': 'Software directory is alwailable on the worker nodes',
                'value': False
            }
        }
        if self.sessiondir is None:
            self.logger.error('There is no sessiondir suitable for community software installation. '
                              'Please set SOFTWARE_DIR location manually!')
        else:
            cconfig['SOFTWARE_DIR'] = self.sessiondir + '/_software/' + c
            cconfig['SOFTWARE_SHARED'] = self.session_shared
        # get community data
        key_data = {}
        if args.url is not None:
            key_data = self.__get_community_json(args.url)
            if key_data is None:
                self.__cdir_cleanup(c)
            if 'pubkey' not in key_data:
                self.logger.error('Community software registry at %s does not include pubic key data.', args.url)
                self.__cdir_cleanup(c)
            if 'keyurl' in key_data['pubkey']:
                keyfile = self.__get_keyfile(self, cdir, key_data['pubkey']['keyurl'])
                if keyfile is None:
                    self.__cdir_cleanup(c)
                key_data['pubkey']['keyfile'] = keyfile
            if 'keydata' in key_data['pubkey']:
                try:
                    decoded_keydata = base64.b64decode(key_data['pubkey']['keydata'])
                    keyfile = os.path.join(cdir, 'pubkey.gpg')
                    with open(keyfile, 'w') as key_f:
                        key_f.write(decoded_keydata)
                    key_data['pubkey']['keyfile'] = keyfile
                except TypeError as e:
                    self.logger.error('Failed to decode retrieved base64 public key data. Error: %s', str(e))
                    self.__cdir_cleanup(c)
        elif args.keyserver is not None:
            if args.fingerprint is None:
                self.logger.error('The fingerprint is required to be used with keyserver.')
                sys.exit(1)
            key_data = {
                'pubkey': {
                    'keyserver': args.keyserver,
                    'fingerprint': args.fingerprint
                }
            }
        elif args.pubkey is not None:
            keyfile = self.__get_keyfile(cdir, args.pubkey)
            if keyfile is None:
                self.__cdir_cleanup(c)
            key_data = {
                'pubkey': {
                    'keyfile': keyfile
                }
            }
        elif args.archery is not None:
            # TODO: get pubkey from ARCHERY
            self.logger.error('Not implemented yet')
            self.__cdir_cleanup(c, 0)
        else:
            # TODO: same but use community name
            self.logger.error('Not implemented yet')
            self.__cdir_cleanup(c, 0)

        # add initial key data to community config
        cconfig['KEYDATA'] = key_data

        # fingerprint is given from the commandline - use for verification
        if args.fingerprint is not None:
            cmdfp = args.fingerprint.replace(' ', '').upper()
            if len(cmdfp) < 8:
                self.logger.error('Provided fingerprint value "%s" is too short for verification. Ignoring it.',
                                  args.fingerprint)
            else:
                key_data['pubkey']['verify'] = cmdfp

        # write the GPG trust database
        gpgdir = os.path.join(cdir, '.gpg')
        if not os.path.exists(gpgdir):
            os.makedirs(gpgdir, mode=0o700)
        # using keyfile vs keyserver
        gpgcmd = ['gpg', '--homedir', gpgdir]
        if 'keyfile' in key_data['pubkey']:
            gpgcmd += ['--import', key_data['pubkey']['keyfile']]
        elif 'keyserver' in key_data['pubkey']:
            gpgcmd += ['--keyserver', key_data['pubkey']['keyserver'], '--recv-key', key_data['pubkey']['fingerprint']]
        else:
            self.logger.error('There is no public key defilend for %s community. Failed to establish trust.', c)
            self.__cdir_cleanup(c)

        # gpg subprocess to import the key
        gpgproc = subprocess.Popen(gpgcmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        __import_re = re.compile(r'^gpg: key .* imported$')
        imported_messages = []
        all_messages = []
        for rline in iter(gpgproc.stdout.readline, b''):
            line = rline.decode('utf-8').rstrip()
            all_messages.append(line)
            if __import_re.match(line):
                imported_messages.append(line.lstrip('gpg: '))
        gpgproc.wait()
        if gpgproc.returncode == 0:
            for m in imported_messages:
                self.logger.info("%s to establish %s community trust", m, c)
        else:
            self.logger.error('Failed to import %s community public key. GPG returns following output:', c)
            for m in all_messages:
                self.logger.error(m)
            self.__cdir_cleanup(c, gpgproc.returncode)

        # verify fingerprint
        gpgcmd = ['gpg', '--homedir', gpgdir, '--fingerprint']
        gpgproc = subprocess.Popen(gpgcmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        __fingerprint_re = re.compile(r'^\s*Key fingerprint =\s*(.*)\s*$')
        vfp = None
        all_messages = []
        for rline in iter(gpgproc.stdout.readline, b''):
            line = rline.decode('utf-8').rstrip()
            all_messages.append(line)
            fp_match = __fingerprint_re.match(line)
            if fp_match:
                vfp = fp_match.group(1)
                vfp = vfp.replace(' ', '').upper()
        gpgproc.wait()
        if gpgproc.returncode == 0:
            if vfp is None:
                self.logger.error('Cannot parse imported key fingerprint. Removing community %s from trusted.', c)
                self.__cdir_cleanup(c)
            if 'verify' in key_data['pubkey']:
                self.logger.debug('Verifying imported key fingerprint against value passed to commandline')
                if vfp.endswith(key_data['pubkey']['verify'].upper()):
                    self.logger.info('Fingerprint value %s match the %s passed from commandline. '
                                     'Trust is established.', vfp, key_data['pubkey']['verify'])
                else:
                    self.logger.error('Fingerprint value %s does not match the %s passed from commandline. '
                                      'Removing community %s from trusted.', vfp, key_data['pubkey']['verify'], c)
                    self.__cdir_cleanup(c)
            else:
                # fingerprint should be confirmed interactively
                print('The imported community public key data is:')
                for m in all_messages[2:]:
                    print('  ' + m)
                if not self.__ask_yes_no('Is the community key fingerprint correct?'):
                    self.logger.info('Removing community %s from trusted.', c)
                    self.__cdir_cleanup(c)
        else:
            self.logger.error('Failed to check %s community public key fingerprint. GPG returns following output:', c)
            for m in all_messages:
                self.logger.error(m)
            self.__cdir_cleanup(c, gpgproc.returncode)

        # write the config
        with open(os.path.join(cdir, 'config.json'), 'w') as c_conf_f:
            json.dump(cconfig, c_conf_f)

    def delete(self, args):
        c = args.community
        if c not in self.communities:
            self.logger.error('There is no such community %s in the trusted list.', c)
            sys.exit(1)
        # TODO: disable/undefault handling
        # remove community directory
        cdir = os.path.join(self.community_rte_dir, c)
        shutil.rmtree(cdir)

    def control(self, args):
        if args.communityaction == 'add':
            self.add(args)
        elif args.communityaction == 'remove' or args.communityaction == 'delete':
            self.delete(args)
        else:
            self.logger.critical('Unsupported RunTimeEnvironment control action %s', args.communityaction)
            sys.exit(1)

    @staticmethod
    def register_parser(root_parser):
        crte_ctl = root_parser.add_parser('community', help='Operating community-defined RunTimeEnvironments')
        crte_ctl.set_defaults(handler_class=CommunityRTEControl)

        crte_actions = crte_ctl.add_subparsers(title='Community RTE Actions', dest='communityaction',
                                               metavar='ACTION', help='DESCRIPTION')

        cadd = crte_actions.add_parser('add', help='Add new trusted community to ARC CE')
        cadd.add_argument('-f', '--fingerprint', help='Fingerprint of the community key', action='store')
        cadd_kinds = cadd.add_mutually_exclusive_group(required=False)
        cadd_kinds.add_argument('-a', '--archery', action='store',
                                help='Use ARCHERY domain name (this is the default with community name as a domain)')
        cadd_kinds.add_argument('-u', '--url', help='Use JSON URL', action='store')
        cadd_kinds.add_argument('--pubkey', help='Manually defined location (URL) of the public key', action='store')
        cadd_kinds.add_argument('--keyserver', help='Manually defined location of PGP keyserver', action='store')
        cadd.add_argument('community', help='Trusted community name')

        cdel = crte_actions.add_parser('remove', help='Remove trusted community from ARC CE')
        cdel.add_argument('-f', '--force', help='Disable and undefault all community RTEs automatically',
                          action='store_true')
        cdel.add_argument('community', help='Trusted community name')

        clist = crte_actions.add_parser('list', help='List trusted communities')
        clist.add_argument('-l', '--long', help='Print more information')

        cconfget = crte_actions.add_parser('config-get', help='Get config variables for trusted community')
        cconfget.add_argument('community', help='Trusted community name')
        cconfget.add_argument('option', help='Configuration option name')

        cconfset = crte_actions.add_parser('config-set', help='Set config variable for trusted community')
        cconfset.add_argument('community', help='Trusted community name')
        cconfset.add_argument('option', help='Configuration option name')
        cconfset.add_argument('value', help='Configuration option value')

        cdeploy = crte_actions.add_parser('deploy', help='Deploy RTE provided by community')
        cdeploy.add_argument('community', help='Trusted community name')
        cdeploy.add_argument('rtename', help='RunTimeEnvironment name')
        cdeploy.add_argument('-u', '--url', help='Explicitly define URL to signed RTE file')