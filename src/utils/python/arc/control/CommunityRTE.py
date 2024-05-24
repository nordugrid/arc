from __future__ import print_function
from __future__ import absolute_import

from .ControlCommon import *
import os
import sys
import shutil
import re
import base64
import json
import hashlib
# gpg command is widely available vs python-gnupg dedicated package
# to eliminate extra dependency we are relying on invoking the command
import subprocess
try:
    from urllib.request import Request, urlopen
    from urllib.error import URLError
    from urllib.parse import quote, unquote
except ImportError:
    from urllib2 import Request, urlopen
    from urllib2 import URLError
    from urllib import quote, unquote
try:
    import dns.resolver
    import dns.query
    from dns.exception import DNSException
    dnssupport = True
except ImportError:
    dnssupport = False


def complete_community(prefix, parsed_args, **kwargs):
    arcconf = get_parsed_arcconf(parsed_args.config)
    return CommunityRTEControl(arcconf).complete_community()


def complete_community_config_option(prefix, parsed_args, **kwargs):
    arcconf = get_parsed_arcconf(parsed_args.config)
    return CommunityRTEControl(arcconf).complete_community_config_option(parsed_args)


def complete_community_rtes(prefix, parsed_args, **kwargs):
    arcconf = get_parsed_arcconf(parsed_args.config)
    return CommunityRTEControl(arcconf).complete_community_rtes(parsed_args)


def complete_community_deployed_rtes(prefix, parsed_args, **kwargs):
    arcconf = get_parsed_arcconf(parsed_args.config)
    return CommunityRTEControl(arcconf).complete_community_rtes(parsed_args, rtype='deployed')


class CommunityRTEControl(ComponentControl):
    def __init__(self, arcconfig):
        self.logger = logging.getLogger('ARCCTL.RunTimeEnvironment.Community')
        if arcconfig is None:
            self.logger.critical('Controlling RunTime Environments is not possible without arc.conf defined controldir')
            sys.exit(1)
        self.arcconfig = arcconfig
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

    # methods to call from general RTE control
    def get_deployed_rtes(self, community=None):
        """Get all deployed community RTEs"""
        # use common RTEControl static methods for consistency
        from .RunTimeEnvironment import RTEControl
        deployed_rtes = {}
        for c in self.communities:
            if community is not None and c != community:
                continue
            self.logger.debug('Indexing deployed RTEs for %s community', c)
            cdir = os.path.join(self.community_rte_dir, c)
            deployed_rte_dir = os.path.join(cdir, 'rte')
            if os.path.isdir(deployed_rte_dir):
                deployed_rtes.update(RTEControl.get_dir_rtes(deployed_rte_dir))
        return deployed_rtes

    def get_rtes_dir(self):
        """Return base location to store community RTEs"""
        return self.community_rte_dir

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
        return response.read()

    def __get_community_json(self, url):
        """Fetch JSON file with community software definition from URL"""
        # fetch URL data
        urldata = self.__fetch_data(url)
        if urldata is None:
            self.logger.error('Failed to fetch software registry data.')
            return None
        # decode it as JSON
        try:
            data = json.loads(urldata.decode('utf-8'))
        except ValueError:
            self.logger.error('Failed to decode software registry data as a valid JSON')
            return None
        if data is None:
            self.logger.error('Community software registry data is empty.')
        return data

    def __fetch_community_rte_index(self, config):
        """Fetch RTEs data from community registry (JSON URL or ARCHERY)"""
        rtes = {}
        if config['type'] == 'manual':
            self.logger.warning('List of available community RTEs is not available without registry.')
            return rtes
        if config['type'] == 'archery':
            if not dnssupport:
                self.logger.error('Failed to find python-dns module. ARCHERY support is disabled.')
                return rtes
            cdata = self.__query_archery(config['url'])
        elif config['type'] == 'url':
            cdata = self.__get_community_json(config['url'])
        else:
            self.logger.error('Unsupported registry type %s in the community configuration.', config['type'])
            return rtes

        if cdata is None:
            self.logger.error('Failed to fetch community RTEs index.')
            return rtes
        if 'rtes' not in cdata:
            self.logger.debug('There are no RTEs in the software registry.')
            return rtes
        for rdata in cdata['rtes']:
            if 'name' not in rdata:
                self.logger.warning('Skipping malformed RTE record %s in the registry.', json.dumps(rdata))
            rtes[rdata['name']] = rdata
        return rtes

    def __parse_archery_txt(self, txtstr):
        """Get data dict from ARCHERY DNS TXT string representation"""
        rrdata = {}
        for kv in txtstr.split(' '):
            # in case of broken records
            if len(kv) < 3:
                self.logger.warning('Malformed archery TXT entry "%s" ("%s" too short for k=v)', txtstr, kv)
                continue
            # only one letter keys and 'id' is supported now
            if kv[1] == '=':
                rrdata[kv[0]] = kv[2:]
            elif kv.startswith('id='):
                rrdata['id'] = kv[3:]
            else:
                self.logger.warning('Malformed archery TXT entry "%s" (%s does not match k=value)', txtstr, kv)
        return rrdata

    def __query_archery_txt(self, dns_name):
        """Fetch TXT data from ARCHERY DNS name"""
        # define archery entry point DNS name
        if dns_name[0:6] == 'dns://':
            dns_name = dns_name[6:]
        else:
            dns_name = '_archery.' + dns_name
        dns_name = dns_name.rstrip('.') + '.'
        # start query
        resolver = dns.resolver.Resolver()
        self.logger.debug('Querying ARCHERY data from: %s', dns_name)
        txts = []
        try:
            archery_rrs = resolver.query(dns_name, 'TXT')
            for rr in archery_rrs:
                # merge strings in TXT (>255)
                txt = ''
                for rri in rr.strings:
                    txt += rri.decode()
                txts.append(txt)
        except DNSException as e:
            logger.warning('Failed to query ARCHERY data from %s (Error: %s)', dns_name, str(e))
        return txts

    def __query_archery_rte(self, dns_name):
        """Query and parse ARCHERY RTE object"""
        rtedata = {}
        for txt in self.__query_archery_txt(dns_name):
            rrdata = self.__parse_archery_txt(txt)
            if 'o' in rrdata:
                if not 'id' in rrdata:
                    self.logger.error('RTE object %s in ARCHERY does not contain id. Skipping.', dns_name)
                    return None
                rtedata['name'] = rrdata['id']
                if 'd' in rrdata:
                    rtedata['description'] = unquote(rrdata['d'])
            elif 'u' in rrdata:
                if 't' in rrdata:
                    if rrdata['t'] == 'gpg.signed':
                        rtedata['url'] = rrdata['u']
                    elif rrdata['t'] == 'gpg.signed.base64':
                        data = ''
                        for rd in self.__query_archery_txt(rrdata['u']):
                            data += rd
                        if data:
                            rtedata['data'] = data
        return rtedata

    def __query_archery_software(self, dns_name, fetch_rtes):
        """Query and parse ARCHERY software registry object"""
        adata = {'pubkey': {}, 'rtes': []}
        for txt in self.__query_archery_txt(dns_name):
            rrdata = self.__parse_archery_txt(txt)
            if 'u' in rrdata:
                if 't' in rrdata:
                    if rrdata['t'] == 'archery.rte':
                        if fetch_rtes:
                            rtedata = self.__query_archery_rte(rrdata['u'])
                            if rtedata:
                                adata['rtes'].append(rtedata)
                    elif rrdata['t'] == 'gpg.pubkey':
                        adata['pubkey']['keyurl'] = rrdata['u']
                    elif rrdata['t'] == 'gpg.pubkey.base64':
                        adata['pubkey']['keydata'] = ''
                        for kd in self.__query_archery_txt(rrdata['u']):
                            adata['pubkey']['keydata'] += kd
                    elif rrdata['t'] == 'gpg.keyserver':
                        adata['pubkey']['keyserver'] = rrdata['u']
        return adata

    def __query_archery(self, dns_name, fetch_rtes=True):
        """Query software information from community ARCHERY"""
        for txt in self.__query_archery_txt(dns_name):
            rrdata = self.__parse_archery_txt(txt)
            if 'u' in rrdata:
                if 't' in rrdata:
                    if rrdata['t'] == 'archery.software':
                        # found archery software object reference
                        adata = self.__query_archery_software(rrdata['u'], fetch_rtes)
                        if adata:
                            return adata
                    elif rrdata['t'] in ['archery.group', 'org.nordugrid.archery']:
                        # follow groupings for archery software objects
                        adata = self.__query_archery(rrdata['u'], fetch_rtes)
                        if adata:
                            return adata
        return None

    def __download_file(self, url, path, chunk_size=524288):
        """Download file in chunks (default is 512k)"""
        self.logger.debug('Fetching data from %s', url)
        req = Request(url)
        try:
            with open(path, 'wb') as dest_f:
                url_data = urlopen(req)
                data_len = 0
                if 'content-length' in url_data.headers:
                    data_len = int(url_data.headers['content-length'])
                    self.logger.info('Downloading %s bytes from %s', data_len, url)
                downloaded = 0
                reported_ratio = 0
                while True:
                    chunk = url_data.read(chunk_size)
                    if not chunk:
                        self.logger.info('Downloaded 100%')
                        break
                    downloaded += chunk_size
                    if data_len > 0:
                        ratio = (downloaded * 100.0)/data_len
                        if ratio - reported_ratio > 10:
                            self.logger.info('Downloaded %.2f%%', ratio)
                            reported_ratio = ratio
                    dest_f.write(chunk)
        except URLError as e:
            if hasattr(e, 'reason'):
                self.logger.error('Failed to fetch data from %s. Error: %s', url, e.reason)
            else:
                self.logger.error('Failed to fetch data from %s. Error code: %s', url, e.code)
            return False
        except IOError as e:
            self.logger.error('Failed to write data to destination file %s. Error: %s', path, str(e))
            return False
        return True

    def __get_keyfile(self, cdir, url):
        """Fetch public key from URL and store it to community directory"""
        keydata = self.__fetch_data(url)
        if keydata is None:
            self.logger.error('Failed to fetch community public key.')
            return None
        keyfile = os.path.join(cdir, 'pubkey.gpg')
        with open(keyfile, 'wb') as key_f:
            key_f.write(keydata)
        return keyfile

    def __cdir_cleanup(self, c, exitcode=1):
        """Remove community dir and exit"""
        cdir = os.path.join(self.community_rte_dir, c)
        shutil.rmtree(cdir)
        sys.exit(exitcode)

    def __load_community_config(self, c):
        """Load community configuration"""
        cdir = os.path.join(self.community_rte_dir, c)
        config = {}
        with open(os.path.join(cdir, 'config.json'), 'r') as c_conf_f:
            config = json.load(c_conf_f)
        return config

    def __save_community_config(self, c, config):
        """Save community configuration"""
        cdir = os.path.join(self.community_rte_dir, c)
        with open(os.path.join(cdir, 'config.json'), 'w') as c_conf_f:
            json.dump(config, c_conf_f)

    def __community_name(self, args):
        """Return community name from the command line arguments given. Failed if community is not configured"""
        c = args.community
        if c not in self.communities:
            self.logger.error('There is no such community %s in the trusted list.', c)
            sys.exit(1)
        return c

    def __deploy_pubkey(self, c, key_data):
        """Process public key data and store the key to community directory"""
        cdir = os.path.join(self.community_rte_dir, c)
        if 'keyurl' in key_data['pubkey']:
            keyfile = self.__get_keyfile(self, cdir, key_data['pubkey']['keyurl'])
            if keyfile is None:
                self.__cdir_cleanup(c)
            key_data['pubkey']['keyfile'] = keyfile
        if 'keydata' in key_data['pubkey']:
            try:
                decoded_keydata = base64.b64decode(key_data['pubkey']['keydata'])
                keyfile = os.path.join(cdir, 'pubkey.gpg')
                with open(keyfile, 'wb') as key_f:
                    key_f.write(decoded_keydata)
                key_data['pubkey']['keyfile'] = keyfile
            except TypeError as e:
                self.logger.error('Failed to decode retrieved base64 public key data. Error: %s', str(e))
                self.__cdir_cleanup(c)

    def __disable_undefault(self, community, rtes=None, action='error'):
        """Check community RTEs are enabled or default. Stop on error or disable/undefault if not."""
        from .RunTimeEnvironment import RTEControl
        rtectl = RTEControl(self.arcconfig)
        cdir = os.path.join(self.community_rte_dir, community)
        if rtes is None:
            # all deployed RTEs if not defined explicitly
            rtes = rtectl.get_dir_rtes(os.path.join(cdir, 'rte'))
        status = {}
        for rte in rtes:
            for epath, etype in [(rtectl.check_enabled(rte), 'enabled'),
                                 (rtectl.check_default(rte), 'default')]:
                if epath is not None and epath.startswith(cdir):
                    if action == 'disable':
                        rtectl.disable_rte({'name': rte, 'path': epath}, etype)
                    elif action == 'status':
                        if rte not in status:
                            status[rte] = []
                        status[rte].append(etype)
                    else:
                        needed_action = 'disable' if etype == 'enabled' else 'undefault'
                        self.logger.error('Community RTE %s is %s. Please %s it first or use "--force" '
                                          'to disable and undefault automatically', rte, etype, needed_action)
                        sys.exit(1)
        return status

    def add(self, args):
        """action: add new trusted community"""
        c = args.community
        if c in self.communities:
            self.logger.error('Cannot add community. Community %s is already trusted.', c)
            sys.exit(1)
        # create community directory
        cdir = os.path.join(self.community_rte_dir, c)
        os.makedirs(cdir, mode=0o755)
        # community config
        cconfig = {}
        # software location
        cconfig['userconf'] = {
            'SOFTWARE_DIR': {
                'description': 'Path to community software installation directory',
                'value': ''
            },
            'SOFTWARE_SHARED': {
                'description': 'Software directory is available on the worker nodes',
                'value': False,
                'type': 'bool'
            }
        }
        if self.sessiondir is None:
            self.logger.error('There is no sessiondir suitable for community software installation. '
                              'Please set SOFTWARE_DIR location manually!')
        else:
            cconfig['userconf']['SOFTWARE_DIR']['value'] = self.sessiondir + '/_software/' + c
            cconfig['userconf']['SOFTWARE_SHARED']['value'] = self.session_shared
        # get community data
        key_data = {}
        if args.url is not None:
            cconfig['type'] = 'url'
            cconfig['url'] = args.url
            key_data = self.__get_community_json(args.url)
            if key_data is None:
                self.logger.error('Failed to add community %s', c)
                self.__cdir_cleanup(c)
            if 'pubkey' not in key_data:
                self.logger.error('Community software registry at %s does not include pubic key data.', args.url)
                self.__cdir_cleanup(c)
            self.__deploy_pubkey(c, key_data)
        elif args.keyserver is not None:
            cconfig['type'] = 'manual'
            cconfig['url'] = None
            if args.fingerprint is None:
                self.logger.error('The fingerprint is required to be used with keyserver.')
                self.__cdir_cleanup(c)
            key_data = {
                'pubkey': {
                    'keyserver': args.keyserver,
                    'fingerprint': args.fingerprint
                }
            }
        elif args.pubkey is not None:
            cconfig['type'] = 'manual'
            cconfig['url'] = None
            keyfile = self.__get_keyfile(cdir, args.pubkey)
            if keyfile is None:
                self.__cdir_cleanup(c)
            key_data = {
                'pubkey': {
                    'keyfile': keyfile
                }
            }
        else:
            # default is ARCHERY registry
            cconfig['type'] = 'archery'
            cconfig['url'] = c
            if args.archery is not None:
                cconfig['url'] = args.archery
            if not dnssupport:
                self.logger.error('Failed to find python-dns module. ARCHERY support is disabled.')
                self.__cdir_cleanup(c)
            key_data = self.__query_archery(cconfig['url'], fetch_rtes=False)
            if key_data is None:
                self.logger.error('Failed to add community %s', c)
                self.__cdir_cleanup(c)
            if 'pubkey' not in key_data:
                self.logger.error('Community ARCHERY at %s does not include pubic key data.', cconfig['url'])
                self.__cdir_cleanup(c)
            self.__deploy_pubkey(c, key_data)

        # add key data to community config
        cconfig['keydata'] = key_data

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
                imported_messages.append(line.replace('gpg: ', ''))
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
        gpgcmd = ['gpg', '--with-colons', '--homedir', gpgdir, '--fingerprint']
        gpgproc = subprocess.Popen(gpgcmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        vfp = None
        keyuid = ''
        for rline in iter(gpgproc.stdout.readline, b''):
            line = rline.decode('utf-8').rstrip()
            if line.startswith('fpr:'):
                vfp = line.split(':')[-2].upper()
            elif line.startswith('uid:'):
                keyuid = line.split(':')[9]
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
                print('  ' + keyuid)
                print('  Fingerprint: ' + ' '.join([vfp[i:i+4] for i in range(0, len(vfp), 4)]))
                if not ask_yes_no('Is the community key fingerprint correct?'):
                    self.logger.info('Removing community %s from trusted.', c)
                    self.__cdir_cleanup(c)
        else:
            self.logger.error('Failed to check %s community public key fingerprint. GPG returns following output:', c)
            for m in all_messages:
                self.logger.error(m)
            self.__cdir_cleanup(c, gpgproc.returncode)

        # write the config
        self.__save_community_config(c, cconfig)

    def delete(self, args):
        """action: delete trusted community"""
        c = self.__community_name(args)
        # disable/undefault RTEs from this community
        action = 'disable' if args.force else 'error'
        self.__disable_undefault(c, None, action)
        # remove community directory
        cdir = os.path.join(self.community_rte_dir, c)
        shutil.rmtree(cdir)

    def list(self, args):
        """action: list available communities"""
        for c in self.communities:
            if not args.long:
                print(c)
            else:
                cconfig = self.__load_community_config(c)
                print('{0:<32} {1:<8} {2}'.format(c, cconfig['type'], cconfig['url']))

    def config_get(self, args):
        """action: Get configuration parameters for community"""
        c = self.__community_name(args)
        cconfig = self.__load_community_config(c)
        if 'userconf' not in cconfig:
            self.logger.error('Malformed community configuration. Consider to remove/add community again.')
            sys.exit(1)
        for opt in cconfig['userconf'].keys():
            if args.option:
                if opt not in args.option:
                    continue
            value = cconfig['userconf'][opt]['value']
            type = 'string'
            if 'type' in cconfig['userconf'][opt]:
                type = cconfig['userconf'][opt]['type']
            if type == 'bool':
                value = 'Yes' if value else 'No'
            if args.long:
                description = '# '
                if 'description' in cconfig['userconf'][opt]:
                    description += cconfig['userconf'][opt]['description']
                else:
                    description += 'No description available'
                description += ' (type: {0})'.format(type)
                print(description)
            print('{0}={1}'.format(opt, value))

    def config_set(self, args):
        """action: Modify configuration parameter for community"""
        c = self.__community_name(args)
        cconfig = self.__load_community_config(c)
        if 'userconf' not in cconfig:
            self.logger.error('Malformed community configuration. Consider to remove/add community again.')
            sys.exit(1)
        option = args.option
        if option not in cconfig['userconf']:
            self.logger.error('There is no option %s defined in community %s config', option, c)
            sys.exit(1)
        oconfig = cconfig['userconf'][option]
        type = 'string'
        if 'type' in oconfig:
            type = oconfig['type']
        value = args.value
        if type == 'bool':
            value = value.lower()
            if value == 'yes':
                oconfig['value'] = True
            elif value == 'no':
                oconfig['value'] = False
            else:
                self.logger.error('The value of option %s is boolean. Please type "Yes" or "No".', option)
                sys.exit(1)
        else:
            oconfig['value'] = value
        self.__save_community_config(c, cconfig)

    def _rte_list_brief(self, c, deployed_rtes, registry_rtes):
        """Show brief list of community-defined RTEs"""
        for rte in sorted(deployed_rtes):
            kind = ['deployed']
            # check exists in registry
            if rte in registry_rtes:
                kind.append('registry')
                registry_rtes[rte]['listed'] = True
            # check enabled/default
            active_status = self.__disable_undefault(c, None, 'status')
            if rte in active_status:
                for s in active_status[rte]:
                    kind.append(s)
            # print
            print('{0:32} ({1})'.format(rte, ', '.join(kind)))
        for rte in sorted(registry_rtes):
            if 'listed' in registry_rtes[rte]:
                continue
            kind = ['registry']
            print('{0:32} ({1})'.format(rte, ', '.join(kind)))

    def _rte_list_long(self, deployed_rtes, registry_rtes):
        """Show detailed list of community-defined RTEs"""
        # use common RTEControl static methods for consistency
        from .RunTimeEnvironment import RTEControl
        # deployed RTEs
        if not deployed_rtes:
            print('There are no community deployed RTEs')
        else:
            print('Community deployed RTEs:')
            for rte in sorted(deployed_rtes):
                if rte in registry_rtes:
                    registry_rtes[rte]['listed'] = True
                print('\t{0:32} # {1}'.format(rte, RTEControl.get_rte_description(deployed_rtes[rte])))
        # available in registry
        registry_avail = {}
        for rte in registry_rtes:
            if 'listed' not in registry_rtes[rte]:
                registry_avail[rte] = registry_rtes[rte]
        if not registry_avail:
            print('There are no additional RTEs available in the community registry')
        else:
            print('Additional community RTEs available in the registry:')
            for rte in sorted(registry_avail):
                description = 'RTE description is Not Available'
                if 'description' in registry_avail[rte]:
                    description = registry_avail[rte]['description']
                print('\t{0:32} # {1}'.format(rte, description))

    def rte_list(self, args):
        """action: show community RTEs"""
        # use common RTEControl static methods for consistency
        from .RunTimeEnvironment import RTEControl
        c = self.__community_name(args)
        # deployed RTEs
        cdir = os.path.join(self.community_rte_dir, c)
        deployed_rte_dir = os.path.join(cdir, 'rte')
        deployed_rtes = {}
        if os.path.isdir(deployed_rte_dir):
            deployed_rtes = RTEControl.get_dir_rtes(deployed_rte_dir)
        # available RTEs
        cconfig = self.__load_community_config(c)
        registry_rtes = self.__fetch_community_rte_index(cconfig)
        # Format data to print-out
        if args.long:
            self._rte_list_long(deployed_rtes, registry_rtes)
        elif args.available:
            for rte in sorted(registry_rtes):
                print(rte)
        elif args.deployed:
            for rte in sorted(deployed_rtes):
                print(rte)
        else:
            self._rte_list_brief(c, deployed_rtes, registry_rtes)

    def __fetch_signed_rte(self, c, rtename):
        """Fetch signed RTE data from community registry"""
        cconfig = self.__load_community_config(c)
        self.logger.debug('Checking RTE %s in the community %s software registry', rtename, c)
        registry_rtes = self.__fetch_community_rte_index(cconfig)
        if rtename not in registry_rtes:
            self.logger.error('RTE %s is not exists in community %s registry', rtename, c)
            sys.exit(1)
        rteconf = registry_rtes[rtename]
        sigrtedata = None
        # rte in the registry can be defined by URL or base64-encoded data
        if 'data' in rteconf:
            sigrtedata = base64.b64decode(rteconf['data'])
        elif 'url' in rteconf:
            sigrtedata = self.__fetch_data(rteconf['url'])
            if sigrtedata is None:
                self.logger.error('Failed to fetch signed RTE from %s', rteconf['url'])
        return sigrtedata

    def rte_cat(self, args):
        """Print the content of RTE"""
        c = self.__community_name(args)
        cconfig = self.__load_community_config(c)
        # paths
        cdir = os.path.join(self.community_rte_dir, c)
        deployed_rte_dir = os.path.join(cdir, 'rte') + '/'
        # RTE
        rtename = args.rtename
        # check already deployed (and print if it is)
        rte_path = deployed_rte_dir + rtename
        if os.path.exists(rte_path):
            self.logger.info('RTE %s already deployed, showing the local content.', rtename)
            with open(rte_path, 'r') as rte_f:
                print(rte_f.read())
            return
        # otherwise fetch from registry
        sigrtedata = self.__fetch_signed_rte(c, rtename)
        self.logger.debug('Decrypting downloaded signed RTE %s', rtename)
        if sigrtedata is not None:
            # decrypt and show
            gpgdir = os.path.join(cdir, '.gpg')
            gpgcmd = ['gpg', '--homedir', gpgdir, '--output', '-', '--decrypt']
            gpgproc = subprocess.Popen(gpgcmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            (gpg_stdout, gpg_stderr) = gpgproc.communicate(input=sigrtedata)
            if gpgproc.returncode == 0:
                self.logger.info('Showing the content of RTE %s fetched from the registry.', rtename)
                print(gpg_stdout.decode())
            else:
                self.logger.error('Failed to decrypt signed RTE %s. GPG errors are: %s', rtename, gpg_stderr)
                sys.exit(1)

    def rte_deploy(self, args):
        """Deploy community-defined RTE (from registry of URL)"""
        c = self.__community_name(args)
        cconfig = self.__load_community_config(c)
        # paths
        cdir = os.path.join(self.community_rte_dir, c)
        deployed_rte_dir = os.path.join(cdir, 'rte') + '/'
        # RTE
        rtename = args.rtename
        # check already deployed
        rte_dir_path = deployed_rte_dir + '/'.join(rtename.split('/')[:-1])
        rte_path = deployed_rte_dir + rtename
        if os.path.exists(rte_path):
            if not args.force:
                self.logger.error('RTE %s is already deployed. Use "--force" if you want to redeploy it.', rtename)
                sys.exit(1)
            else:
                self.logger.info('Forcing redeploying of RTE %s. Cleaning files from previous deployment.', rtename)
                self.rte_remove(args)
        # get RTE deta (from URL or registry)
        rte_content = None
        if args.url:
            self.logger.info('Deploying community %s RTE %s from manually specified location %s', c, rtename, args.url)
            if args.insecure:
                self.logger.info('Going to deploy unsigned RTE script from %s', args.url)
                rte_content = self.__fetch_data(args.url)
                if rte_content is None:
                    self.logger.error('Failed to fetch unsigned RTE from %s', args.url)
                sigrtedata = None
            else:
                sigrtedata = self.__fetch_data(args.url)
                if sigrtedata is None:
                    self.logger.error('Failed to fetch signed RTE from %s', args.url)
        else:
            sigrtedata = self.__fetch_signed_rte(c, rtename)
            if sigrtedata is None:
                self.logger.error('There is no signed RTE URL or data defined in the registry. Deploy failed.')
                sys.exit(1)

        if sigrtedata is not None:
            # store signed RTE data
            signed_rtes = os.path.join(cdir, 'signed')
            if not os.path.exists(signed_rtes):
                os.makedirs(signed_rtes, mode=0o755)
            signed_rte_file = os.path.join(signed_rtes, rtename.replace('/', '-') + '.signed')
            with open(signed_rte_file, 'wb') as srte_f:
                srte_f.write(sigrtedata)

            # verify
            gpgdir = os.path.join(cdir, '.gpg')
            gpgcmd = ['gpg', '--homedir', gpgdir, '--verify', signed_rte_file]
            gpgproc = subprocess.Popen(gpgcmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            gpgmessages = []
            for rline in iter(gpgproc.stdout.readline, b''):
                line = rline.decode('utf-8').rstrip()
                gpgmessages.append(line)
            gpgproc.wait()
            if gpgproc.returncode == 0:
                self.logger.info('RTE %s signature verified successfully.', rtename)
            else:
                self.logger.error('Failed to verify RTE %s signature. GPG returns following output:', rtename)
                for m in gpgmessages:
                    self.logger.error(m)
                os.unlink(signed_rte_file)
                sys.exit(1)

        # deploy
        if not os.path.exists(rte_dir_path):
            self.logger.debug('Making RunTimeEnvironment directory structure %s', rte_dir_path)
            os.makedirs(rte_dir_path, mode=0o755)

        if sigrtedata is not None:
            gpgcmd = ['gpg', '--homedir', gpgdir, '--output', rte_path, '--decrypt', signed_rte_file]
            gpgproc = subprocess.Popen(gpgcmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            for rline in iter(gpgproc.stdout.readline, b''):
                line = rline.decode('utf-8').rstrip()
                self.logger.info(line.replace('gpg: ', ''))
            gpgproc.wait()
            if gpgproc.returncode == 0:
                self.logger.info('RTE script deployed to %s', rte_path)
            else:
                self.logger.error('Failed to deploy RTE %s.', rtename)
                self.rte_remove(args)
                sys.exit(1)
        elif rte_content is not None:
            try:
                with open(rte_path, 'w') as rte_f:
                    rte_f.write(rte_content)
                self.logger.info('RTE script deployed to %s', rte_path)
            except IOError as e:
                self.logger.error('Failed to deploy RTE %s. Error: %s', rtename, str(e))
        else:
            self.logger.error('Failed to deploy RTE %s.', rtename)
            self.rte_remove(args)

        # download
        downloads = []
        # parse RTE headers to define what needs to be downloaded
        with open(rte_path, 'r') as rte_f:
            for line in rte_f:
                if line.startswith('# download'):
                    ditem = {}
                    for dinfo in line[10:].split(' '):
                        if dinfo.startswith('url:'):
                            ditem['url'] = dinfo[4:]
                        elif dinfo.startswith('checksum:'):
                            ditem['checksum_type'] = dinfo[9:].split(':', 2)[0]
                            ditem['checksum_value'] = dinfo[9:].split(':', 2)[1].strip()
                        elif dinfo.startswith('filename:'):
                            ditem['filename'] = dinfo[9:].strip()
                    if 'url' not in ditem:
                        self.logger.error('Failed to find URL in the download line: %s. Skipping!', line)
                        continue
                    if 'filename' not in ditem:
                        ditem['filename'] = ditem['url'].split('/')[-1]
                    if 'checksum_type' not in ditem:
                        self.logger.warning('No checksum defined for URL %s. Software download will be insecure!')
                        if not ask_yes_no('Are you want to continue?'):
                            self.rte_remove(args)
                            sys.exit(1)
                    downloads.append(ditem)
        # download into software dir
        swdir = cconfig['userconf']['SOFTWARE_DIR']['value']
        if not swdir:
            self.logger.error('No software installation directory defined for community %s. '
                              'Please use "arcctl rte community config-set" first.', c)
            self.rte_remove(args)
            sys.exit(1)
        rte_swdir = swdir.rstrip('/') + '/' + rtename
        cconfig['userconf']['SOFTWARE_DIR']['value'] = rte_swdir
        if not os.path.exists(rte_swdir):
            self.logger.debug('Making software directory for RTE: %s ', rte_swdir)
            os.makedirs(rte_swdir, mode=0o755)
        for d in downloads:
            swfile = rte_swdir + '/' + d['filename']
            self.logger.info('Downloading software (file %s) from %s', d['filename'], d['url'])
            if not self.__download_file(d['url'], swfile):
                self.logger.error('Download from %s failed.', d['url'])
                self.rte_remove(args)
                sys.exit(1)
            else:
                # calculate checksum
                if 'checksum_type' in d:
                    try:
                        h = hashlib.new(d['checksum_type'])
                        with open(swfile, 'rb') as h_f:
                            while True:
                                buf = h_f.read(4096)
                                if not buf:
                                    break
                                h.update(buf)
                        if h.hexdigest().lower() == d['checksum_value'].lower():
                            self.logger.info('Checksum verified successfully for file %s (%s:%s)',
                                             d['filename'], d['checksum_type'], h.hexdigest())
                        else:
                            self.logger.error('Checksum verification failed for file %s (expected %s, calculated %s)',
                                              d['filename'], d['checksum_value'], h.hexdigest())
                            self.rte_remove(args)
                            sys.exit(1)
                    except ValueError as e:
                        self.logger.error('Failed to verify downloaded file (%s) checksum. '
                                          'Error: %s', d['filename'], str(e))
                        self.rte_remove(args)
                        sys.exit(1)

        # write community params files (community configured locations)
        rte_params_path = self.control_rte_dir + '/params/'
        if not os.path.exists(rte_params_path):
            self.logger.debug('Making control directory %s for RunTimeEnvironments parameters', rte_params_path)
            os.makedirs(rte_params_path, mode=0o755)
        rte_dir_path = rte_params_path + '/'.join(rtename.split('/')[:-1])
        if not os.path.exists(rte_dir_path):
            self.logger.debug('Making RunTimeEnvironment directory structure inside controldir %s', rte_dir_path)
            os.makedirs(rte_dir_path, mode=0o755)
        self.logger.debug('Writing software location paths to RTE community parameters')
        community_param = rte_params_path + rtename + '.community'
        with open(community_param, 'w') as rte_parm_f:
            for p in cconfig['userconf'].keys():
                rte_parm_f.write('{0}="{1}"\n'.format(p, cconfig['userconf'][p]['value']))

    def rtefile_remove(self, rtename, basedir):
        """Removes RTE file in defined basedir and all empty directories"""
        basedir = basedir.rstrip('/') + '/'
        if os.path.exists(basedir + rtename):
            os.unlink(basedir + rtename)
        rte_split = rtename.split('/')[:-1]
        while rte_split:
            rte_dir = basedir + '/'.join(rte_split)
            if not os.listdir(rte_dir):
                self.logger.debug('Removing empty RunTimeEnvironment directory %s', rte_dir)
                os.rmdir(rte_dir)
            del rte_split[-1]

    def rte_remove(self, args):
        """Remove deployed community RTE"""
        c = self.__community_name(args)
        cdir = os.path.join(self.community_rte_dir, c)
        deployed_rte_dir = os.path.join(cdir, 'rte') + '/'
        rtename = args.rtename
        # check enabled/default already
        action = 'disable' if args.force else 'error'
        self.__disable_undefault(c, [rtename], action)
        # RTE script itself
        rte_path = deployed_rte_dir + rtename
        if os.path.exists(rte_path):
            self.logger.debug('Removing RTE script file: %s', rte_path)
        self.rtefile_remove(rtename, deployed_rte_dir)
        # params file
        rte_params_path = self.control_rte_dir + '/params/'
        rte_params = rte_params_path + rtename + '.commnity'
        # software dir (params contains deploy-time data)
        cconfig = self.__load_community_config(c)
        swdir = cconfig['userconf']['SOFTWARE_DIR']['value'].rstrip('/') + '/' + rtename
        if os.path.exists(rte_params):
            with open(rte_params, 'r') as p_f:
                for line in p_f:
                    if line.startswith('SOFTWARE_DIR='):
                        swdir = line[13:].strip('"')
        # remove software from dir
        if os.path.exists(swdir):
            for f in os.listdir(swdir):
                swfile = os.path.join(swdir, f)
                self.logger.debug('Removing RTE software file: %s', swfile)
                os.unlink(swfile)
            self.rtefile_remove(rtename + '/dummy', swdir.replace(rtename, ''))
        # remove params
        if os.path.exists(rte_params):
            self.logger.debug('Removing RTE community parameters file: %s', rte_params)
            self.rtefile_remove(rtename + '.community', rte_params_path)
        # signed RTE file
        signed_rtes = os.path.join(cdir, 'signed')
        signed_rte_file = os.path.join(signed_rtes, rtename.replace('/', '-') + '.signed')
        if os.path.exists(signed_rte_file):
            self.logger.debug('Removing signed RTE deploy file: %s', signed_rte_file)
            os.unlink(signed_rte_file)

    def control(self, args):
        if args.communityaction == 'add':
            self.add(args)
        elif args.communityaction == 'remove' or args.communityaction == 'delete':
            self.delete(args)
        elif args.communityaction == 'list':
            self.list(args)
        elif args.communityaction == 'config-get':
            self.config_get(args)
        elif args.communityaction == 'config-set':
            self.config_set(args)
        elif args.communityaction == 'rte-list':
            self.rte_list(args)
        elif args.communityaction == 'rte-deploy':
            self.rte_deploy(args)
        elif args.communityaction == 'rte-cat':
            self.rte_cat(args)
        elif args.communityaction == 'rte-remove':
            self.rte_remove(args)
        else:
            self.logger.critical('Unsupported RunTimeEnvironment control action %s', args.communityaction)
            sys.exit(1)

    def complete_community_config_option(self, args):
        c = self.__community_name(args)
        cconfig = self.__load_community_config(c)
        if 'userconf' not in cconfig:
            return []
        return cconfig['userconf'].keys()

    def complete_community(self):
        return self.communities

    def complete_community_rtes(self, args, rtype='registry'):
        c = self.__community_name(args)
        if rtype == 'deployed':
            deployed_rtes = self.get_deployed_rtes(c)
            return deployed_rtes.keys()
        cconfig = self.__load_community_config(c)
        registry_rtes = self.__fetch_community_rte_index(cconfig)
        return registry_rtes.keys()

    @staticmethod
    def register_parser(root_parser):
        crte_ctl = root_parser.add_parser('community', help='Operating community-defined RunTimeEnvironments')
        crte_ctl.set_defaults(handler_class=CommunityRTEControl)

        crte_actions = crte_ctl.add_subparsers(title='Community RTE Actions', dest='communityaction',
                                               metavar='ACTION', help='DESCRIPTION')
        crte_actions.required = True

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
        cdel.add_argument('community', help='Trusted community name').completer = complete_community

        clist = crte_actions.add_parser('list', help='List trusted communities')
        clist.add_argument('-l', '--long', help='Print more information', action='store_true')

        cconfget = crte_actions.add_parser('config-get', help='Get config variables for trusted community')
        cconfget.add_argument('community', help='Trusted community name').completer = complete_community
        cconfget.add_argument('option', help='Configuration option name',
                              nargs='*').completer = complete_community_config_option
        cconfget.add_argument('-l', '--long', help='Print more information', action='store_true')

        cconfset = crte_actions.add_parser('config-set', help='Set config variable for trusted community')
        cconfset.add_argument('community', help='Trusted community name').completer = complete_community
        cconfset.add_argument('option', help='Configuration option name').completer = complete_community_config_option
        cconfset.add_argument('value', help='Configuration option value')

        crtelist = crte_actions.add_parser('rte-list', help='List RTEs provided by community')
        crtelist.add_argument('community', help='Trusted community name').completer = complete_community
        crtelist_g = crtelist.add_mutually_exclusive_group(required=False)
        crtelist_g.add_argument('-l', '--long', help='Print more information', action='store_true')
        crtelist_g.add_argument('-a', '--available', help='List RTEs available in the software registry',
                                action='store_true')
        crtelist_g.add_argument('-d', '--deployed', help='List deployed community RTEs', action='store_true')

        crtecat = crte_actions.add_parser('rte-cat', help='Print the content of RTEs provided by community')
        crtecat.add_argument('community', help='Trusted community name').completer = complete_community
        crtecat.add_argument('rtename', help='RunTimeEnvironment name').completer = complete_community_rtes

        cdeploy = crte_actions.add_parser('rte-deploy', help='Deploy RTE provided by community')
        cdeploy.add_argument('community', help='Trusted community name').completer = complete_community
        cdeploy.add_argument('rtename', help='RunTimeEnvironment name').completer = complete_community_rtes
        cdeploy.add_argument('-u', '--url', help='Explicitly define URL to signed RTE file')
        cdeploy.add_argument('-f', '--force', help='Force RTE files redeployment if already exists',
                             action='store_true')
        cdeploy.add_argument('--insecure', help='Do not validate community signature for URL-based deployment',
                             action='store_true')

        cremove = crte_actions.add_parser('rte-remove', help='Remove deployed community RTE')
        cremove.add_argument('community', help='Trusted community name').completer = complete_community
        cremove.add_argument('rtename', help='RunTimeEnvironment name').completer = complete_community_deployed_rtes
        cremove.add_argument('-f', '--force', help='Disable and undefault RTE automatically', action='store_true')