from __future__ import print_function
from __future__ import absolute_import

from .ControlCommon import *

import time
import os
import subprocess
import socket
import stat
import sys
import shutil
import random
import uuid
import json
try:
    from jwcrypto import jwk, jwt
    from jwcrypto.common import json_decode, json_encode
except ImportError:
    jwt = None

default_token_validity = 12

arc_conf_snippet = """[authgroup:testjwt]
authtokens = * {0} arc * *


[mapping]
map_to_user = testjwt nobody:nobody

[arex/ws/jobs]
allowaccess = testjwt
"""

def default_jwk_dir():
    if os.getuid() == 0:
       return '/etc/grid-security/jwt'
    return os.path.expanduser('~/.arc/jwt')

class JWTIssuer(object):
    """Object to store public information about the JWT issuer"""
    def __init__(self, iss):
        self.logger = logging.getLogger('ARCCTL.JWTIssuer')
        self.iss = iss
        self.isshash = crc32_id(iss)
        self.jwks = {}
        self.metadata = {}
        self.logger.info('JWT issuer: %s', self.iss)

    def __json_dump(self, dst_file=None, data={}, description=''):
        """Save json to file"""
        if dst_file is None:
            raise Exception('Destination file is required')
        try:
            with open(dst_file, 'w') as fd:
                self.logger.debug("Saving %s to %s", description, dst_file)
                json.dump(data, fd)
        except IOError as err:
            self.logger.error('Failed to write %s file %s. Error: %s', description, dst_file, str(err))
            sys.exit(1)
        except ValueError as err:
            self.logger.error('Failed to dump %s as JSON. Error: %s', description, str(err))
            sys.exit(1)

    def __json_load(self, src_file=None, description=''):
        """Load json from file"""
        if src_file is None:
            raise Exception('Source file is required')
        try:
            with open(src_file, 'r') as fd:
                self.logger.debug("Loading %s from %s", description, src_file)
                return json.load(fd)
        except IOError as err:
            self.logger.error('Failed to open %s file %s. Error: %s', description, src_file, str(err))
            sys.exit(1)
        except ValueError as err:
            self.logger.error('Failed to parse %s file %s. Error: %s', description, src_file, str(err))
            sys.exit(1)

    # metadata
    def define_metadata(self):
        """Define basic metadata based on issuer URL"""
        self.metadata = {
            'issuer': self.iss,
            'token_endpoint': self.iss + '/token',
            'userinfo_endpoint': self.iss + '/userinfo',
            'introspection_endpoint': self.iss + '/introspect',
            'authorization_endpoint': self.iss + '/authorize',
            'jwks_uri': self.iss + '/jwks'
        }

    def fetch_metadata(self):
        """Fetch issuer metadata from well-known URL"""
        # TODO: implement
        pass

    def save_metadata(self, dst_file=None):
        """Save issuer metadata to file"""
        self.__json_dump(dst_file, data=self.metadata, description='JWT issuer metadata')

    def load_metadata(self, src_file=None):
        """Load issuer metadata from file"""
        self.metadata = self.__json_load(src_file, description='JWT issuer metadata')

    # jwks
    def define_jwks(self, jwks={}):
        """Define JWKS"""
        self.jwks = jwks

    def fetch_jwks(self):
        """Fetch data from JWKS URL"""
        # TODO: implement
        pass

    def save_jwks(self, dst_file=None):
        """Save JWKS to file"""
        self.__json_dump(dst_file, data=self.jwks, description='JWKS')

    def load_jwks(self, src_file=None):
        """Load JWKS from file"""
        self.jwks = self.__json_load(src_file, description='JWKS')

    # generic methods
    def save(self, dst_dir=None, metadata='metadata.json', jwks='jwks.json'):
        if dst_dir is None:
            raise Exception('Destination dir is required')
        # create dir if not exists
        try:
            os.makedirs(dst_dir)
        except FileExistsError:
            pass
        except IOError as err:
            self.logger.error('Failed to create JWT Issuer direcory %s. Error: %s',
                            dst_dir, str(err))
            sys.exit(1)
        # dump data
        self.save_metadata(os.path.join(dst_dir, metadata))
        self.save_jwks(os.path.join(dst_dir, jwks))

    def load(self, src_dir='.', metadata='metadata.json', jwks='jwks.json'):
        self.load_metadata(os.path.join(src_dir, metadata))
        self.load_jwks(os.path.join(src_dir, jwks))

    def info(self):
        """Print issuer info to end-user"""
        print('Issuer URL: {0}'.format(self.iss))
        print('JWKS:')
        print(json.dumps(self.jwks, indent=2))

    def url(self):
        """Return issuer URL"""
        return self.iss

    def hash(self):
        """Return issuer URL hash"""
        return self.isshash

    # Controldir operations
    def controldir_save(self, arcconfig=None):
        if not arcctl_server_mode():
            raise Exception("Deployment to controldir cannot run in arcctl client mode")
        path = self.__controldir_path(arcconfig)
        self.save(path, metadata='metadata', jwks='keys')
        with open(os.path.join(path, 'issuer'), 'w') as iss_f:
            iss_f.write(self.iss)
        self.logger.info('Issuer data for establishing trust is deployed to A-REX controldir: %s', path)

    def controldir_cleanup(self, arcconfig):
        path = self.__controldir_path(arcconfig)
        if os.path.exists(path):
            self.logger.info('Removing A-REX controldir issuer files: %s', path)
            shutil.rmtree(path)

    def __controldir_path(self, arcconfig):
        if arcconfig is None:
            self.logger.critical('Deploying issuer trust files to controldir is not possible without valid arc.conf')
            sys.exit(1)
        return os.path.join(arcconfig.get_value('controldir', 'arex'), 'tokenissuers/test-jwt-{0}'.format(self.isshash))


class TestJWTControl(ComponentControl):
    def __init__(self, arcconfig):
        self.logger = logging.getLogger('ARCCTL.TestJWT')
        # module is not always in the system repos, but still can be installed from e.g. pip or third-party repos
        # we are distributing test-jwt without strong package-level requirements for such a systems
        if jwt is None:
            self.logger.critical('You need to install python jwcrypto module first to use Test JWT functionality.')
            sys.exit(1)
        self.jwk = None
        self.hostname = None
        self.iss = None
        self.jwt_conf = {}
        # use hostname from arc.conf if possible
        self.arcconfig = arcconfig
        if arcconfig is None:
            self.logger.debug('Working in config-less mode. ARCCTL defaults will be used.')
        else:
            self.hostname = arcconfig.get_value('hostname', 'common')
            self.logger.debug('Using hostname from arc.conf: %s', self.hostname)
        # if hostname is not defined via arc.conf
        if self.hostname is None:
            try:
                # try to get it from hostname -f
                hostname_f = subprocess.Popen(['hostname', '-f'], stdout=subprocess.PIPE)
                self.hostname = hostname_f.stdout.readline().decode().strip()
                self.logger.debug('Using hostname from \'hostname -f\': %s', self.hostname)
            except OSError:
                # fallback
                self.hostname = socket.gethostname()
                self.logger.warning('Cannot get hostname from \'hostname -f\'. '
                                    'Using %s that comes from name services.', self.hostname)

    def __define_iss(self, issid=None):
        """Internal function to define Issuer ID and file paths"""
        if issid is None:
            issid = self.hostname

        iss_url = 'https://{0}/arc/testjwt/{1}'.format(self.hostname, crc32_id(issid))
        if os.getuid() != 0:
            iss_url = iss_url + '/' + str(os.getuid())

        self.iss = JWTIssuer(iss_url)

        # paths
        self.iss_dir = os.path.join(self.jwk_dir, self.iss.hash())
        self.jwk_key = os.path.join(self.iss_dir, 'jwk.key')
        self.jwt_conf_f = os.path.join(self.jwk_dir, 'jwt.conf')

    def __define_jwk_dir(self, dir=None):
        """Define JWK dir"""
        if dir is None:
            dir = default_jwk_dir()
        self.jwk_dir = dir

    def __load_jwt_conf(self):
        """Load config for token generation"""
        if os.path.exists(self.jwt_conf_f):
            try:
                with open(self.jwt_conf_f, 'r') as jwt_cf:
                    self.logger.debug("Loading default token issuing settings from %s", self.jwt_conf_f)
                    self.jwt_conf = json.load(jwt_cf)
            except IOError as err:
                self.logger.error('Failed to open JWT config file %s. Error: %s', self.jwt_conf_f, str(err))
                sys.exit(1)
            except ValueError as err:
                self.logger.error('Failed to parse JWT config file %s. Error: %s', self.jwt_conf_f, str(err))
                sys.exit(1)

    def __save_jwt_conf(self):
        """Save config for token generation"""
        try:
            with open(self.jwt_conf_f, 'w') as jwt_cf:
                self.logger.debug("Saving default token issuing setting to %s", self.jwt_conf_f)
                json.dump(self.jwt_conf, jwt_cf)
        except IOError as err:
            self.logger.error('Failed to write JWT config file %s. Error: %s', self.jwt_conf_f, str(err))
            sys.exit(1)

    def __get_jwt_conf(self, profile, key):
        """Get value from token generation config"""
        if profile not in self.jwt_conf:
            return None
        if key not in self.jwt_conf[profile]:
            return None
        return self.jwt_conf[profile][key]

    def __set_jwt_conf(self, profile, key, value):
        """Set value in token generation config"""
        if profile not in self.jwt_conf:
            self.jwt_conf[profile] = {}
        self.jwt_conf[profile][key] = value

    def __del_jwt_conf(self, profile, key):
        """Delete value in token generation config"""
        if profile not in self.jwt_conf:
            self.jwt_conf[profile] = {}
        if key in self.jwt_conf[profile]:
            del self.jwt_conf[profile][key]

    def jwt_conf_get(self, args):
        """Return token generation config value to end-user"""
        self.__load_jwt_conf()

        profile = args.profile
        if profile not in self.jwt_conf:
                self.logger.info('No JWT configuration assigned to %s profile.', profile)
                sys.exit(1)

        key = args.key
        if key is None:
            # print all config
            print(json.dumps(self.jwt_conf[profile], indent=2))
        else:
            value = self.__get_jwt_conf(profile, key)
            if value is None:
                self.logger.info('Config value for "%s" in profile "%s" is not set.', key, profile)
            else:
                print(value)

    def jwt_conf_set(self, args):
        """Set token generation config value as provided by end-user"""
        self.__load_jwt_conf()
        if args.value:
            self.__set_jwt_conf(args.profile, args.key, args.value)
            self.logger.info('Value for %s in profile %s has been set to %s',
                             args.key, args.profile, args.value)
        else:
            self.__del_jwt_conf(args.profile, args.key)
            self.logger.info('Value for %s in profile %s has ben unset',
                             args.key, args.profile)
        self.__save_jwt_conf()

    def load_jwk(self):
        """Load tokens singing key from disk"""
        if os.path.exists(self.jwk_key):
            try:
                self.logger.debug('Initializing JWK from keyfile %s', self.jwk_key)
                with open(self.jwk_key, 'r') as jwk_f:
                    jwk_dict = json_decode(jwk_f.read())
                    self.jwk = jwk.JWK(**jwk_dict)
                self.logger.debug('Loading JWT Issue info from %s', self.iss_dir)
                self.iss.load(self.iss_dir)
            except IOError as err:
                self.logger.error('Failed to open JWK key file %s. Error: %s', self.jwk_key, str(err))
                sys.exit(1)
            except ValueError as err:
                self.logger.error('Failed to parse JWK key file %s. Error: %s', self.jwk_key, str(err))
                sys.exit(1)
        else:
            self.logger.error('JWK key file %s does not exist. Run "arctl test-jwt init" first.', self.jwk_key)
            sys.exit(1)

    def create_jwk(self, args):
        """Generane new RSA keypair for tokens signing"""
        if os.path.exists(self.jwk_key):
            # handle exist vs force
            if not args.force:
                self.logger.error('JWK key file %s aleady exists. Use --force if you want to re-init.',
                                  self.jwk_key)
                sys.exit(1)
        # create issuer JWK dir
        try:
            os.makedirs(self.iss_dir)
        except FileExistsError:
            pass
        except IOError as err:
            self.logger.error('Failed to create JWK direcory %s for issuer %s. Error: %s',
                            self.iss_dir, self.iss.url(), str(err))
            sys.exit(1)
        # generate RSA keypair
        self.jwk = jwk.JWK(generate='RSA', size=2048, kid="testjwt", use="sig")
        # write JWK private key
        try:
            with open(self.jwk_key, 'w') as jwk_f:
                jwk_f.write(self.jwk.export(private_key=True))
            os.chmod(self.jwk_key, stat.S_IRUSR | stat.S_IWUSR )
            self.logger.info('Keypair for tokens issuing generated successfully.')
        except IOError as err:
            self.logger.error('Failed to write JWK key file. Error: %s', str(err))
            sys.exit(1)
        except ValueError as err:
            self.logger.error('Failed to generate JWK key pair. Error: %s', str(err))
            sys.exit(1)
        # define metadata and JWKS
        jwkset = jwk.JWKSet()
        jwkset['keys'] = self.jwk
        self.iss.define_jwks(json.loads(jwkset.export(private_keys=False)))
        self.iss.define_metadata()
        self.iss.save(self.iss_dir)
        # install issuer files into controldir
        # TODO: make deploy configurable
        # TODO: provide export/import instruction instead
        if arcctl_server_mode() and os.getuid() == 0:
            self.iss.controldir_save(self.arcconfig)
        # print information to end-user
        self.issuer_info()

    def issuer_info(self, arc_conf=False):
        """Print information about token issuer"""
        # check init is done and key exists
        if self.jwk is None:
            self.load_jwk()
        # print general info about issuer
        self.iss.info()
        # print arc.conf snippet
        if arc_conf:
            print('To make A-REX authorize issued tokens add the following to "arc.conf":')
            print(arc_conf_snippet.format(self.iss.url()))

    def cleanup_files(self):
        """Cleanup test JWT files for issuer"""
        if os.path.exists(self.iss_dir):
            self.logger.info('Removing issuer JWK directory: %s', self.iss_dir)
            shutil.rmtree(self.iss_dir)
        # remove trust when running on server-side
        if arcctl_server_mode():
            self.iss.controldir_cleanup(self.arcconfig)

    def issue_token(self, args):
        """Issue signed token"""
        if self.jwk is None:
            self.load_jwk()

        profile = args.profile

        self.__load_jwt_conf()

        # username
        username = args.username
        if username is None:
            username = self.__get_jwt_conf(profile, 'username')
            if username is None:
                # generate persistent default username
                randidx = random.randint(10000000, 99999999)
                username = 'Test User {0}'.format(randidx)
                self.__set_jwt_conf(profile, 'username', username)
                self.__save_jwt_conf()

        unixtime_now = int(time.time())

        # token validity
        validity = args.validity
        if validity is None:
            validity = self.__get_jwt_conf(profile, 'validity')
            if validity is None:
                validity = default_token_validity
        validity_s = int(validity) * 3600

        # scope
        scope = "openid profile"
        extra_scopes = args.scopes
        if extra_scopes is None:
            extra_scopes = self.__get_jwt_conf(profile, 'scopes')
        if extra_scopes:
            scope = scope + ' ' + extra_scopes

        # arbitrary extra claims
        extra_claims = {}
        extra_claims_str = args.claims
        if extra_claims_str is None:
            extra_claims_str = self.__get_jwt_conf(profile, 'claims')
        if extra_claims_str is not None:
            try:
                extra_claims = json_decode(extra_claims_str)
            except ValueError as err:
                self.logger.error('Failed to parse extra claims JSON %s. Error: %s',
                                  extra_claims_str, str(err))
                sys.exit(1)

        claims = {
            "iat": unixtime_now,
            "nbf": unixtime_now - 300,  ## 5 min clock scew (same as grid-proxies)
            "exp": unixtime_now + validity_s,
            "jti": str(uuid.uuid1()),
            "iss": self.iss.url(),
            "aud": "arc",
            "azp": "arc",
            "sub": username.replace(':', ''),  ## RFC7519 StringOrURI
            "typ": "Bearer",
            "scope": scope,
            "name": username,
            "preferred_username": username
        }

        claims.update(extra_claims)
        header = {"alg": "RS256", "kid": "testjwt"}

        token = jwt.JWT(header=header, claims=claims)
        token.make_signed_token(self.jwk)

        self.logger.info('Token has been issued:\n{0}\n{1}'.format(
            json.dumps(header, indent=2),
            json.dumps(claims, indent=2)
        ))
        # token to stdout
        print(token.serialize())

    def control(self, args):
        # init issuer
        self.__define_jwk_dir(args.jwk_dir)
        self.__define_iss(args.iss_id)
        # parse actions
        if args.action == 'init':
            self.create_jwk(args)
        elif args.action == 'cleanup':
            self.cleanup_files()
        elif args.action == 'info':
            self.issuer_info(args.arc_conf)
        elif args.action == 'export':
            # TODO: export functionality (along with deployment)
            self.logger.error('Not implemented')
            pass
        elif args.action == 'config-get':
            self.jwt_conf_get(args)
        elif args.action == 'config-set':
            self.jwt_conf_set(args)
        elif args.action == 'token':
            self.issue_token(args)
        else:
            self.logger.critical('Unsupported ARC Test JWT action %s', args.action)
            sys.exit(1)

    @staticmethod
    def register_parser(root_parser):
        testjwt_ctl = root_parser.add_parser('test-jwt', help='ARC Test JWT control')
        testjwt_ctl.set_defaults(handler_class=TestJWTControl)

        testjwt_ctl.add_argument('--iss-id', action='store',
                                help='Define arcctl token Issuer ID to work with (default is hostname)')
        testjwt_ctl.add_argument('--jwk-dir', action='store', default=default_jwk_dir(),
                                help='Redefine path to JWK files directory (default is %(default)s)')

        testjwt_actions = testjwt_ctl.add_subparsers(title='Test JWT Actions', dest='action',
                                                   metavar='ACTION', help='DESCRIPTION')
        testjwt_actions.required = True

        testjwt_init = testjwt_actions.add_parser('init', help='Generate RSA key-pair for JWT signing')
        testjwt_init.add_argument('-f', '--force', action='store_true', help='Overwrite files if exist')

        testjwt_issuer = testjwt_actions.add_parser('info', help='Show information about Test JWT issuer')
        testjwt_issuer.add_argument('-a', '--arc-conf', action='store_true', help='Show arc.conf snippet for using issuer')

        testjwt_export = testjwt_actions.add_parser('export', help='Export JWT issuer information to be imported to ARC CE')

        testjwt_cleanup = testjwt_actions.add_parser('cleanup', help='Cleanup TestJWT files')

        testjwt_conf_get = testjwt_actions.add_parser('config-get', help='Get JWT token generation config')
        testjwt_conf_get.add_argument('-p', '--profile', action='store', default='default',
                                 help='Config named profile (default is %(default)s')
        testjwt_conf_get.add_argument('key', nargs='?', help='Config key')

        testjwt_conf_set = testjwt_actions.add_parser('config-set', help='Set JWT token generation config')
        testjwt_conf_set.add_argument('-p', '--profile', action='store', default='default',
                                 help='Config named profile (default is %(default)s')
        testjwt_conf_set.add_argument('key', choices=['username', 'validity', 'scopes', 'claims'],
                                      help='Config key as in token options')
        testjwt_conf_set.add_argument('value', help='Config value')

        testjwt_token = testjwt_actions.add_parser('token', help='Issue JWT token')
        testjwt_token.add_argument('-p', '--profile', action='store', default='default',
                                 help='Generate using token named profile (default is %(default)s')
        testjwt_token.add_argument('-n', '--username', action='store',
                                 help='Use specified username instead of automatically generated')
        testjwt_token.add_argument('-v', '--validity', type=int,
                                 help='Validity of the token in hours (default is {0})'.format(default_token_validity))
        testjwt_token.add_argument('-s', '--scopes', action='store',
                                 help='Additional scopes to include into the token')
        testjwt_token.add_argument('-c', '--claims', action='store',
                                 help='Additional claims (JSON) to include into the token')
