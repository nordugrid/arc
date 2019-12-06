from __future__ import print_function
from __future__ import absolute_import

from .AccountingDB import AccountingDB, AAR
from arc.paths import ARC_VERSION

import sys
import logging
import re
import datetime
import calendar

# SGASSender dependencies
import socket
import ssl
try:
    import httplib
except ImportError:
    import http.client as httplib

# APELSSMSender dependencies:
try:
    from dirq.QueueSimple import QueueSimple
except ImportError:
    QueueSimple = None

try:
    import cStringIO as StringIO
except ImportError:
    try:
        import io as StringIO
    except ImportError:
        import StringIO

apel_libs = None
try:
    # third-party SSM libraries
    from ssm import __version__ as ssm_version
    from ssm.ssm2 import Ssm2, Ssm2Exception
    from ssm.crypto import CryptoException
    apel_libs = 'apel'
except ImportError:
    # re-distributed with NorduGrid ARC
    try:
        from arc.ssm import __version__ as ssm_version
        from arc.ssm.ssm2 import Ssm2, Ssm2Exception
        from arc.ssm.crypto import CryptoException
        apel_libs = 'arc'
    except ImportError:
        ssm_version = (0, 0, 0)
        Ssm2 = None
        Ssm2Exception = None
        CryptoException = None

# module regexes init
__voms_fqan_re = re.compile(r'(?P<group>/[-\w.]+(?:/[-\w.]+)*)(?P<role>/Role=[-\w.]+)?(?P<cap>/Capability=[-\w.]+)?')

__url_re = re.compile(r'^(?P<url>(?P<protocol>[^:/]+)://(?P<host>[^:/]+)(?::(?P<port>[0-9]+))?/*(?:(?P<path>/.*))?)$')


# common helpers
def get_fqan_components(fqan):
    """Return tuple of parsed VOMS FQAN parts"""
    (group, role, capability) = (None, None, None)
    fqan_match = __voms_fqan_re.match(fqan)
    if fqan_match:
        fqan_dict = fqan_match.groupdict()
        if 'group' in fqan_dict and fqan_dict['group']:
            group = fqan_dict['group']
        if 'role' in fqan_dict and fqan_dict['role']:
            role = fqan_dict['role']
        if 'cap' in fqan_dict and fqan_dict['cap']:
            capability = fqan_dict['cap']
    return group, role, capability


def get_url_components(url):
    """Return a dict of parsed URL, None if malformed (no match)"""
    url_match = __url_re.match(url)
    if not url_match:
        return None
    url_dict = url_match.groupdict()
    # define well-known port for http(s) urls
    if url_dict['port'] is None:
        if url_dict['protocol'] == 'http':
            url_dict['port'] = 80
        elif url_dict['protocol'] == 'https':
            url_dict['port'] = 443
    else:
        url_dict['port'] = int(url_dict['port'])
    return url_dict


def duration_to_iso8601(seconds):
    """Convert seconds to ISO8601 period notation"""
    seconds = int(seconds)
    minutes, seconds = divmod(seconds, 60)
    hours, minutes = divmod(minutes, 60)
    days, hours = divmod(hours, 24)
    iso8601 = 'P'
    if days:
        iso8601 += '{0}D'.format(days)
    iso8601 += 'T'
    if hours:
        iso8601 += '{0}H'.format(hours)
    if minutes or hours:
        iso8601 += '{0}M'.format(minutes)
    iso8601 += '{0}S'.format(int(seconds))
    return iso8601


def datetime_to_iso8601(dt):
    """Return ISO8601 representation of datetime (without microseconds)"""
    return dt.strftime("%Y-%m-%dT%H:%M:%SZ")


def get_apel_benchmark(logger, benchmark):
    """Use only APEL supported benchmarks (fallback to HEPSPEC 1.0)"""
    __allowed_benchmark_types = ['Si2k', 'HEPSPEC']
    if not benchmark:
        logger.error('Missing benchmark value. "HEPSPEC:1.0" will be used as a fallback.')
        return 'HEPSPEC', '1.0'
    bsplit = benchmark.split(':', 2)
    if len(bsplit) != 2:
        logger.error('Malformed benchmark definition string "%s". "HEPSPEC:1.0" will be used instead.', benchmark)
        return 'HEPSPEC', '1.0'
    (benchmark_type, benchmark_value) = bsplit
    if benchmark_type not in __allowed_benchmark_types:
        logger.error('Benchmark type %s in not supported by APEL. "HEPSPEC:1.0" will be used instead.',
                     benchmark_type)
        return 'HEPSPEC', '1.0'
    return benchmark_type, benchmark_value


#
# Records publishing classes
#
class RecordsPublisher(object):
    """Class for publishing accounting records to central network services"""
    def __init__(self, arcconfig):
        self.logger = logging.getLogger('ARC.Accounting.Publisher')
        self.adb = None
        # arc config
        if arcconfig is None:
            self.logger.error('Failed to parse arc.conf. Accounting publihsing is not possible.')
            sys.exit(1)
        self.arcconfig = arcconfig
        # get configured accounting targets and options
        self.vomsless_vo = self.arcconfig.get_value('vomsless_vo', ['arex/jura'])
        self.extra_vogroups = self.arcconfig.get_value('vo_group', ['arex/jura'], force_list=True)
        self.conf_targets = map(lambda t: ('sgas', t), self.arcconfig.get_subblocks('arex/jura/sgas'))
        self.conf_targets += map(lambda t: ('apel', t), self.arcconfig.get_subblocks('arex/jura/apel'))
        # accounting database files and connection
        self.accounting_dir = arcconfig.get_value('controldir', 'arex').rstrip('/') + '/accounting'
        adb_file = self.accounting_dir + '/accounting.db'
        self.adb = AccountingDB(adb_file)
        # publishing state database file
        self.pdb_file = self.accounting_dir + '/publishing.db'
        self.logger.debug('Accounting records publisher had been initialized')

    def __del__(self):
        self.logger.debug('Shutting down accounting records publisher')
        if self.adb is not None:
            del self.adb
            self.adb = None

    def __get_target_confdict(self, target):
        """Get accounting target configuration and check mandatory options"""
        self.logger.debug('Searching for [%s] target configuration block in arc.conf', target)
        arcconfdict = self.arcconfig.get_config_dict([target])
        if target not in arcconfdict:
            self.logger.error('Failed to find %s target configuration.')
            return None
        return arcconfdict[target]

    def __check_target_confdict(self, ttype, targetconf):
        if 'targeturl' not in targetconf:
            self.logger.error('Target URL is mandatory for publishing but missing.'
                              'Target processing stopped.')
            return False
        targeturl = targetconf['targeturl']
        if get_url_components(targeturl) is None:
            self.logger.error('Target URL %s does not match URL syntax.', targeturl)
            return False
        if ttype == 'apel' and 'gocdb_name' not in targetconf:
            self.logger.error('GOCDB name is required for APEL publishing but missing in target configuration.'
                              'Records will not be reported to %s target.', targeturl)
            return False
        return True

    def __add_vo_filter(self, target_conf):
        """Add WLCG VO filtering if defined in arc.conf"""
        if 'vofilter' in target_conf:
            vos = []
            if not isinstance(target_conf['vofilter'], list):
                vos.append(target_conf['vofilter'])
            else:
                vos += target_conf['vofilter']
            self.logger.info('VO filtering is configured. Querying jobs owned by this WLCG VO(s): %s', ','.join(vos))
            self.adb.filter_wlcgvos(vos)

    def __init_adb_filters(self, endfrom=None, endtill=None):
        self.adb.filters_clean()
        # timerange filters first
        if endfrom is not None:
            self.adb.filter_endfrom(endfrom)
        if endtill is not None:
            self.adb.filter_endtill(endtill)
        # only finished jobs
        self.adb.filter_statuses(['completed', 'failed'])

    def publish_sgas(self, target_conf, endfrom, endtill=None):
        """Publish to SGAS target"""
        self.logger.debug('Assigning filters to SGAS publishing database query')
        self.__init_adb_filters(endfrom, endtill)
        # optional WLCG VO filtering
        self.__add_vo_filter(target_conf)
        # query latest job endtime in this records query
        latest_endtime = self.adb.get_latest_endtime()
        if latest_endtime is None:
            self.logger.info('There are no records to publish. Nothing to do')
            return None
        # parse targetURL and get necessary parameters for SGAS publishing
        urldict = get_url_components(target_conf['targeturl'])
        if urldict['protocol'] != 'https':
            self.logger.error('SGAS communication is only possible via HTTPS target URL.')
            return None
        target_conf['targethost'] = urldict['host']
        target_conf['targetport'] = urldict['port']
        urpath = urldict['path'].rstrip('/')
        if not urpath.endswith('/ur'):  # URs publishing path is '/sgas/ur'
            urpath += '/ur'
        target_conf['targetpath'] = urpath
        # add certificate/keys information to target config
        for k in ['x509_host_cert', 'x509_host_key', 'x509_cert_dir']:
            target_conf[k] = self.arcconfig.get_value(k, ['arex/jura'])
        # init SGAS sender
        sgassender = SGASSender(target_conf)
        # get AARs from database
        self.logger.debug('Querying AARS from accounting database')
        aars = self.adb.get_aars(resolve_ids=True)
        self.logger.info('Accounting database query returned %s AARs', len(aars))
        self.logger.debug('Querying additional job AARs information')
        self.adb.enrich_aars(aars, authtokens=True, rtes=True, dtrs=True, extra=True)
        # get UR parameters
        localid_prefix = target_conf['localid_prefix'] if 'localid_prefix' in target_conf else None
        # build URs
        self.logger.debug('Going to create OGF.98 Usage Records based on the AARs')
        urs = [UsageRecord(aar, localid_prefix=localid_prefix,
                           vomsless_vo=self.vomsless_vo, extra_vogroups=self.extra_vogroups) for aar in aars]
        if not urs:
            self.logger.error('Failed to build OGF.98 Usage Records for SGAS publishing')
            return None
        # send usage records
        if not sgassender.send(urs):
            self.logger.error('Failed to publish messages to SGAS target')
            return None
        # no failures on the way: return latest endtime for records published
        self.logger.debug('Accounting records have been published to SGAS target %s', target_conf['targethost'])
        return latest_endtime

    def publish_apel(self, target_conf, endfrom, endtill=None, regular=False):
        """Publish to all APEL targets"""
        # Parse targetURL and get necessary parameters for APEL publishing
        urldict = get_url_components(target_conf['targeturl'])
        target_conf['targethost'] = urldict['host']
        target_conf['targetport'] = urldict['port']
        target_conf['targetssl'] = True if urldict['protocol'] == 'https' else False
        target_conf['dirq_dir'] = self.accounting_dir + '/ssm/' + urldict['host']
        # add certificate/keys information to target config
        for k in ['x509_host_cert', 'x509_host_key', 'x509_cert_dir']:
            target_conf[k] = self.arcconfig.get_value(k, ['arex/jura'])
        # init SSM sender
        apelssm = APELSSMSender(target_conf)
        if not apelssm.init_dirq():
            self.logger.error('Failed to initialize APEL SSM message queue. Sending records to APEL will be disabled. Please check dirq and stomp python modules are installed on your system.')
            return None
        # database query filters
        self.logger.debug('Assigning filters to APEL usage records database query')
        self.__init_adb_filters(endfrom, endtill)
        # optional WLCG VO filtering
        self.__add_vo_filter(target_conf)
        # query latest job endtime in this records query
        latest_endtime = self.adb.get_latest_endtime()
        if latest_endtime is None:
            self.logger.info('There are no new job records to publish. Nothing to do.')
        else:
            # query and queue messages for sending
            if 'apel_messages' not in target_conf:
                self.logger.error('Cannot find APEL message type in the target configuration. Summaries will be sent.')
                target_conf['apel_messages'] = 'summaries'
            if target_conf['apel_messages'] == 'urs' or target_conf['apel_messages'] == 'both':
                # get individual AARs from database
                self.logger.debug('Querying AARS from accounting database')
                aars = self.adb.get_aars(resolve_ids=True)
                self.logger.info('Accounting database query returned %s AARs', len(aars))
                self.logger.debug('Querying additional job information about AARs')
                self.adb.enrich_aars(aars, authtokens=True, rtes=True, extra=True)
                # create EMI CAR URs
                self.logger.debug('Going to create EMI CAR v1.2 Compute Accounting Records based on the AARs')
                cars = [ComputeAccountingRecord(aar, gocdb_name=target_conf['gocdb_name'],
                               vomsless_vo=self.vomsless_vo, extra_vogroups=self.extra_vogroups) for aar in aars]
                if not cars:
                    self.logger.error('Failed to build EMI CAR v1.2 records for APEL publishing.')
                    return None
                # enqueue CARs
                apelssm.enqueue_cars(cars)
            if target_conf['apel_messages'] == 'summaries' or target_conf['apel_messages'] == 'both':
                # define summaries time interval
                summary_endfrom = endfrom
                if regular:
                    # regularly sent summaries include previous month
                    today_dt = datetime.datetime.today()
                    sef_year = today_dt.year
                    sef_month = today_dt.month - 1
                    if sef_month == 0:
                        sef_year -= 1
                        sef_month = 12
                    summary_endfrom = datetime.datetime(sef_year, sef_month, 1)
                # reinitialize filters to APEL summaries publishing ()
                self.logger.debug('Assigning filters for APEL summaries database query')
                self.__init_adb_filters(summary_endfrom, endtill)
                # optional WLCG VO filtering
                self.__add_vo_filter(target_conf)
                # summary records
                self.logger.debug('Querying APEL Summary data from accounting database')
                apelsummaries = [APELSummaryRecord(rec, target_conf['gocdb_name'])
                                 for rec in self.adb.get_apel_summaries()]
                if not apelsummaries:
                    self.logger.error('Failed to build APEL summaries for publishing')
                    return None
                # enqueue summaries
                apelssm.enqueue_summaries(apelsummaries)
        # Sync messages are always sent and contains job counts for all periods
        self.logger.debug('Assigning filters to APEL sync database query')
        self.__init_adb_filters()
        # optional WLCG VO filtering
        self.__add_vo_filter(target_conf)
        self.logger.debug('Querying APEL Sync data from accounting database')
        apelsyncs = [APELSyncRecord(rec, target_conf['gocdb_name']) for rec in self.adb.get_apel_sync()]
        # enqueue sync messages
        apelssm.enqueue_sync(apelsyncs)
        # publish
        if not apelssm.send():
            self.logger.error('Failed to publish messages to APEL broker')
            return None
        # no failures on the way: return latest endtime for records published
        self.logger.debug('Accounting records have been published to APEL broker %s', target_conf['targethost'])
        return latest_endtime

    def find_configured_target(self, targetname):
        for (ttype, target) in self.conf_targets:
            (_, tname) = target.split(':')
            if targetname == tname.strip():
                targetconf = self.__get_target_confdict(target)
                return targetconf, ttype
        return None, None

    def republish(self, targettype, targetconf, endfrom, endtill=None):
        self.logger.info('Starting republishing process to %s target', targettype.upper())
        # check config for mandatory options
        if not self.__check_target_confdict(targettype, targetconf):
            return False
        # publish without recording the last intervals
        latest = None
        if targettype == 'apel':
            latest = self.publish_apel(targetconf, endfrom, endtill)
        elif targettype == 'sgas':
            latest = self.publish_sgas(targetconf, endfrom, endtill)
        if latest is None:
            return False
        return True

    def publish_regular(self):
        """Publish to all configured targets regularly using latest published date stored in database"""
        self.logger.debug('Starting regular accounting publishing sequence')
        for (ttype, target) in self.conf_targets:
            # get target config dict
            targetconf = self.__get_target_confdict(target)
            if targetconf is None:
                continue
            # check general target options
            if not self.__check_target_confdict(ttype, targetconf):
                continue
            # skip legacy fallback targets
            if 'legacy_fallback' in targetconf:
                if targetconf['legacy_fallback'] == 'yes':
                    self.logger.warning(
                        'Legacy fallback enabled in the configuration for target in [%s] block. Skipping.', target)
                    continue
            # constrains on reporting frequency
            self.adb.attach_publishing_db(self.pdb_file)
            if 'urdelivery_frequency' in targetconf:
                lastreported = self.adb.get_last_report_time(target)
                unixtime_now = calendar.timegm(datetime.datetime.today().timetuple())
                if (unixtime_now - lastreported) < int(targetconf['urdelivery_frequency']):
                    self.logger.debug('Records have been reported to [%s] target less than %s seconds ago. '
                                      'Skipping publishing during this run due to "urdelivery_frequency" constraint.',
                                      target, targetconf['urdelivery_frequency'])
                    continue
            # fetch latest published record timestamp for this target
            endfrom = self.adb.get_last_published_endtime(target)
            self.logger.info('Publishing latest accounting data to [%s] target (jobs finished since %s).',
                             target, datetime.datetime.utcfromtimestamp(endfrom))
            # do publishing
            latest = None
            if ttype == 'apel':
                latest = self.publish_apel(targetconf, endfrom, regular=True)
            elif ttype == 'sgas':
                latest = self.publish_sgas(targetconf, endfrom)
            # update latest published record timestamp
            if latest is not None:
                self.adb.set_last_published_endtime(target, latest)
                self.logger.info('Accounting data for jobs finished before %s '
                                 'have been successfully published to [%s] target',
                                 datetime.datetime.utcfromtimestamp(latest), target)


class SGASSender(object):
    """Send messages to SGAS server"""
    def __init__(self, targetconf):
        self.logger = logging.getLogger('ARC.Accounting.SGAS')
        self.conf = targetconf
        self.batchsize = int(self.conf['urbatchsize']) if 'urbatchsize' in self.conf else 50
        self.logger.debug('Initializing SGAS records sender for %s', self.conf['targethost'])

    class HTTPSClientAuthConnection(httplib.HTTPSConnection):
        """ Class to make a HTTPS connection, with support for full client-based SSL Authentication"""

        def __init__(self, host, port, key_file, cert_file, cacerts_path=None, timeout=None):
            httplib.HTTPSConnection.__init__(self, host, port, key_file=key_file, cert_file=cert_file)
            self.key_file = key_file
            self.cert_file = cert_file
            self.cacerts_path = cacerts_path
            self.timeout = timeout

        def connect(self):
            """ Connect to a host on a given (SSL) port.
                If ca_certs_path is pointing somewhere, use it to check Server Certificate.
                This is needed to pass ssl.CERT_REQUIRED as parameter to SSLContext for Python 2.6+
                which forces SSL to check server certificate against CA certificates in the defined location
            """
            sock = socket.create_connection((self.host, self.port), self.timeout)
            if self._tunnel_host:
                self.sock = sock
                self._tunnel()
            if self.cacerts_path and hasattr(ssl, 'SSLContext'):
                protocol = ssl.PROTOCOL_SSLv23
                if hasattr(ssl, 'PROTOCOL_TLS'):
                    protocol = ssl.PROTOCOL_TLS
                # create SSL context with CA certificates verification
                ssl_ctx = ssl.SSLContext(protocol)
                ssl_ctx.load_verify_locations(capath=self.cacerts_path)
                ssl_ctx.verify_mode = ssl.CERT_REQUIRED
                ssl_ctx.load_cert_chain(self.cert_file, self.key_file)
                ssl_ctx.check_hostname = True
                self.sock = ssl_ctx.wrap_socket(sock, server_hostname=self.host)
            else:
                self.sock = ssl.wrap_socket(sock, self.key_file, self.cert_file,
                                            cert_reqs=ssl.CERT_NONE)

    def __post_xml(self, xmldata):
        self.logger.debug('Initializing HTTPS connection to %s:%s', self.conf['targethost'], self.conf['targetport'])
        conn = self.HTTPSClientAuthConnection(
            host=self.conf['targethost'],
            port=self.conf['targetport'],
            key_file=self.conf['x509_host_key'],
            cert_file=self.conf['x509_host_cert'],
            cacerts_path=self.conf['x509_cert_dir']
        )
        sent = False
        try:
            # conn.set_debuglevel(1)
            headers = {'Content-type': 'application/xml; charset=UTF-8'}
            self.logger.debug('POSTing jobrecords batch to SGAS')
            conn.request('POST', self.conf['targetpath'], str(xmldata), headers)
            resp = conn.getresponse()
            if resp.status != 200:
                self.logger.error('Failed to POST records to SGAS server %s. Error code %s returned: %s',
                                  self.conf['targethost'], str(resp.status), resp.reason)
                # parse error from SGAS Traceback in returned HTML
                for line in resp.read().split('\n'):
                    if line.startswith('<p class="error"'):
                        errstr = line.replace('<p class="error">&lt;class', '').replace('&gt;', '').replace('</p>', '')
                        self.logger.error('SGAS returned the following error description:%s', errstr)
            else:
                self.logger.info('Records have been sent to SGAS server %s.', self.conf['targethost'])
                sent = True
        except Exception as e:
            self.logger.error('Error connecting to SGAS server %s. Error: %s', self.conf['targethost'], str(e))

        conn.close()
        return sent

    def send(self, urs):
        for x in range(0, len(urs), self.batchsize):
            self.logger.debug('Preparing JobUsageRecords batch of max %s records', self.batchsize)
            buf = StringIO.StringIO()
            buf.write(UsageRecord.batch_header())
            buf.write(UsageRecord.join_str().join(map(lambda ur: ur.get_xml(), urs[x:x + self.batchsize])))
            buf.write(UsageRecord.batch_footer())
            published = self.__post_xml(buf.getvalue())
            buf.close()
            del buf
            if not published:
                return False
        return True


class APELSSMSender(object):
    """Send messages to APEL broker using SSM libraries"""
    def __init__(self, targetconf):
        self.logger = logging.getLogger('ARC.Accounting.APELSSM')
        self.conf = targetconf
        self._dirq = None
        self.batchsize = int(self.conf['urbatchsize']) if 'urbatchsize' in self.conf else 1000
        # adjust SSM and stomp logging
        ssmlogroot = 'arc.ssm' if apel_libs == 'arc' else 'ssm'
        self.__configure_third_party_logging(ssmlogroot)
        self.__configure_third_party_logging('stomp.py')
        self.logger.debug('Initializing APEL SSM records sender for %s', self.conf['targethost'])

    def __del__(self):
        # remove unused intermediate directories
        if self._dirq:
            self._dirq.purge(0, 0)

    def __configure_third_party_logging(self, logroot):
        tplogger = logging.getLogger(logroot)
        tplogger.setLevel(self.logger.getEffectiveLevel())
        log_handler_stderr = logging.StreamHandler()
        log_handler_stderr.setFormatter(
            logging.Formatter('[%(asctime)s] [%(name)s] [%(levelname)s] [%(process)d] [%(message)s]'))
        tplogger.addHandler(log_handler_stderr)

    def init_dirq(self):
        if apel_libs is None:
            return False
        if QueueSimple is None:
            return False
        dirq_path = self.conf['dirq_dir']
        self._dirq = QueueSimple(dirq_path)
        return True

    def enqueue_cars(self, carlist):
        for x in range(0, len(carlist), self.batchsize):
            self.logger.debug('Enqueuing EMI CARs batch of max %s records to be sent', self.batchsize)
            buf = StringIO.StringIO()
            buf.write(ComputeAccountingRecord.batch_header())
            buf.write(ComputeAccountingRecord.join_str().join(map(lambda ur: ur.get_xml(),
                                                                  carlist[x:x + self.batchsize])))
            buf.write(ComputeAccountingRecord.batch_footer())
            self._dirq.add(buf.getvalue())
            buf.close()
            del buf

    def enqueue_summaries(self, summaries):
        for x in range(0, len(summaries), self.batchsize):
            self.logger.debug('Enqueuing APEL Summaries batch of max %s records to be sent', self.batchsize)
            buf = StringIO.StringIO()
            buf.write(APELSummaryRecord.header())
            buf.write(APELSummaryRecord.join_str().join(map(lambda s: s.get_record(), summaries[x:x + self.batchsize])))
            buf.write(APELSummaryRecord.footer())
            self._dirq.add(buf.getvalue())
            buf.close()
            del buf

    def enqueue_sync(self, syncs):
        for x in range(0, len(syncs), self.batchsize):
            self.logger.debug('Enqueuing APEL Sync messages to be sent')
            buf = StringIO.StringIO()
            buf.write(APELSyncRecord.header())
            buf.write(APELSyncRecord.join_str().join(map(lambda s: s.get_record(), syncs[x:x + self.batchsize])))
            buf.write(APELSyncRecord.footer())
            self._dirq.add(buf.getvalue())
            buf.close()
            del buf

    def send(self):
        ssmsource = 'NorduGrid ARC redistributed' if apel_libs == 'arc' else 'system-wide installed'
        self.logger.info('Going to send records to %s APEL broker using %s SSM libraries version %s.%s.%s',
                         self.conf['targeturl'], ssmsource, *ssm_version)
        self.logger.debug('Processing records in the %s directory queue.', self.conf['dirq_dir'])
        brokers = [(self.conf['targethost'], self.conf['targetport'])]
        success = False
        try:
            # create SSM2 object for sender
            self.logger.debug('Initializing SSM2 sender to publish records into %s queue', self.conf['topic'])
            sender = Ssm2(brokers,
                          qpath=self.conf['dirq_dir'],
                          cert=self.conf['x509_host_cert'],
                          key=self.conf['x509_host_key'],
                          capath=self.conf['x509_cert_dir'],
                          dest=self.conf['topic'],
                          use_ssl=self.conf['targetssl'])

            if sender.has_msgs():
                sender.handle_connect()
                sender.send_all()
                self.logger.debug('SSM records sending to %s has finished.', self.conf['targeturl'])
            else:
                self.logger.info('No messages found in %s to be sent.', self.conf['dirq_dir'])
        except (Ssm2Exception, CryptoException) as e:
            self.logger.error('SSM failed to send messages successfully. Error: %s', e)
        except Exception as e:
            self.logger.error('Unexpected exception in SSM (type %s). Error: %s. ', e.__class__, e)
        else:
            success = True

        try:
            sender.close_connection()
        except UnboundLocalError:
            # SSM not set up.
            pass

        self.logger.debug('SSM has shut down.')
        return success


#
# Different accounting usage records
#
class JobAccountingRecord(object):
    """Base class for representing XML Job Accounting Records that implements common methots for all formats"""
    def __init__(self, aar):
        """Create XML representation of Accounting Record"""
        self.xml = ''
        self.aar = aar  # type: AAR

    # General helper methods
    def _add_aar_node(self, node, field):
        """Return XML tag for AAR attribute"""
        return '<{0}>{1}</{0}>'.format(node, self.aar.get()[field])

    def _add_extra_node(self, node, extra):
        """Return optional XML tag for AAR extra info attribute"""
        aar_extra = self.aar.extra()
        if extra in aar_extra:
            return '<{0}>{1}</{0}>'.format(node, aar_extra[extra])
        return ''

    def get_xml(self):
        """Return record XML"""
        return self.xml


class UsageRecord(JobAccountingRecord):
    """Class representing SGAS (OFG.98 based) Usage Record format of job accounting records"""
    __xml_templates = {
        'ur-base': '<JobUsageRecord xmlns:arc="http://www.nordugrid.org/ws/schemas/ur-arc" '
                   'xmlns:ds="http://www.w3.org/2000/09/xmldsig#" '
                   'xmlns:tr="http://www.sgas.se/namespaces/2010/10/filetransfer" '
                   'xmlns:urf="http://schema.ogf.org/urf/2003/09/urf" '
                   'xmlns:vo="http://www.sgas.se/namespaces/2009/05/ur/vo" '
                   'xmlns:xsd="http://www.w3.org/2001/XMLSchema" '
                   'xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">{jobrecord}</JobUsageRecord>',
        'record-id': '<RecordIdentity urf:createTime="{createtime}" urf:recordId="{recordid}"/>',
        'job-id': '<JobIdentity><GlobalJobId>{globalid}</GlobalJobId>{localid}</JobIdentity>',
        'user-id': '<UserIdentity><GlobalUserName>{globalid}</GlobalUserName>{localid}{wlcgvos}</UserIdentity>',
        'vo-info': '<vo:VO vo:type="voms"><vo:Name>{voname}</vo:Name>{issuer}{attributes}</vo:VO>',
        'dtr-input': '<tr:FileDownload><tr:URL>{url}</tr:URL><tr:Size>{size}</tr:Size>'
                     '<tr:StartTime>{tstart}</tr:StartTime><tr:EndTime>{tend}</tr:EndTime>'
                     '<tr:RetrievedFromCache>{cache}</tr:RetrievedFromCache></tr:FileDownload>',
        'dtr-output': '<tr:FileUpload><tr:URL>{url}</tr:URL><tr:Size>{size}</tr:Size>'
                      '<tr:StartTime>{tstart}</tr:StartTime><tr:EndTime>{tend}</tr:EndTime></tr:FileUpload>'
    }

    def __init__(self, aar, localid_prefix=None, vomsless_vo=None, extra_vogroups=None):
        """Create XML representation of UsageRecord"""
        JobAccountingRecord.__init__(self, aar)
        self.logger = logging.getLogger('ARC.Accounting.UR')
        self.localid_prefix = localid_prefix
        self.vomsless_vo = vomsless_vo
        self.extra_vogroups = extra_vogroups
        if self.extra_vogroups is None:
            self.extra_vogroups = []
        self.__create_xml()

    def __record_identity(self):
        """Construct RecordIdentity"""
        return self.__xml_templates['record-id'].format(**{
            'createtime': datetime_to_iso8601(datetime.datetime.today()),
            'recordid': 'ur-' + self.aar.submithost() + '-' + self.aar.get()['JobID']
        })

    def __job_identity(self):
        """Construct JobIdentity"""
        # GlobalJobId and LocalJobID properties are optional in UR but at least one must be present
        # we always have GlobalJobId
        # ProcessID is another optional value we don't have
        localid = self.aar.get()['LocalJobID']
        if localid:
            # optional localid_prefix in arc.conf
            if self.localid_prefix is not None:
                localid = self.localid_prefix + localid
            # define localid XML
            localid = '<LocalJobId>{0}</LocalJobId>'.format(localid)
        return self.__xml_templates['job-id'].format(**{
            'globalid': self.aar.get()['JobID'],
            'localid': localid
        })

    def __voms_fqan_xml(self):
        """Construct SGAG VO Extension XML based on AAR and arc.conf info"""
        vomsxml = ''
        # VO info
        wlcgvo = self.aar.wlcgvo()
        voissuer = ''  # no issuer info in current implementation, is this mandatory?
        if not wlcgvo:
            # arc.conf: vomsless VO
            if self.vomsless_vo is not None:
                vomsless_data = self.vomsless_vo.split('#')
                wlcgvo = vomsless_data[0]
                if len(vomsless_data) > 1:
                    voissuer = vomsless_data[1]
        if not wlcgvo:
            return vomsxml
        # VOMS FQAN info
        vomsfqans = [t[1] for t in self.aar.authtokens() if t[0] == 'vomsfqan' or t[0] == 'mainfqan']
        fqangroups = []
        fqan_xml = ''
        if vomsfqans:
            for fqan in vomsfqans:
                (fqan_group, fqan_role, fqan_cap) = get_fqan_components(fqan)
                if fqan_group is not None:
                    fqan_xml += '<vo:Attribute>'
                    fqan_xml += '<vo:Group>{0}</vo:Group>'.format(fqan_group)
                    fqangroups.append(fqan_group)
                    if fqan_role is not None:
                        fqan_xml += '<vo:Role>{0}</vo:Role>'.format(fqan_role)
                    if fqan_cap is not None:
                        fqan_xml += '<vo:Capability>{0}</vo:Capability>'.format(fqan_cap)
                if fqan_xml:
                    fqan_xml += '</vo:Attribute>'
        # arc.conf: additional vo_group attributes
        for vg in self.extra_vogroups:
            if vg not in fqangroups:  # eliminate duplicates
                fqan_xml += '<vo:Attribute><vo:Group>{0}/vo:Group></vo:Attribute>'.format(vg)
        # xml VO issuer if defined
        if voissuer:
            voissuer = '<vo:Issuer>{0}</vo:Issuer>'.format(voissuer)
        # Return XML
        return self.__xml_templates['vo-info'].format(**{
            'voname': wlcgvo,
            'issuer': voissuer,
            'attributes': fqan_xml
        })

    def __user_identity(self):
        """Construct UserIdentity"""
        # LocalUserId is optional
        # OGF.98 does not restrict this value to username, so we can use name[:group]
        localuser = self._add_extra_node('LocalUserId', 'localuser')
        # GlobalUserId is optional but always present, so it is part of common UserIdentity template
        # WLCG VO attributes are part of SGAS VO Extension
        return self.__xml_templates['user-id'].format(**{
            'globalid': self.aar.get()['UserSN'],
            'localid': localuser,
            'wlcgvos': self.__voms_fqan_xml()
        })

    def __job_data(self):
        """Return XML nodes for resource usage and other informational job attributes in AAR"""
        xml = ''
        aar_extra = self.aar.extra()
        # JobName (optional)
        xml += self._add_extra_node('JobName', 'jobname')
        # Status (mandatory)
        xml += self._add_aar_node('Status', 'Status')
        # Memory (optional)
        xml += '<Memory urf:storageUnit="KB" urf:metric="max" urf:type="virtual">{virtual}</Memory>' \
               '<Memory urf:storageUnit="KB" urf:metric="max" urf:type="physical">{physical}</Memory>'.format(**{
            'virtual': self.aar.get()['UsedVirtMem'],
            'physical': self.aar.get()['UsedMemory']
        })
        # Wall/CPUDuration (optional)
        xml += '<WallDuration>{walltime}</WallDuration>' \
               '<CpuDuration urf:usageType="user">{cpuusertime}</CpuDuration>' \
               '<CpuDuration urf:usageType="system">{cpukerneltime}</CpuDuration>'.format(**{
            'walltime': duration_to_iso8601(self.aar.get()['UsedWalltime']),
            'cpuusertime': duration_to_iso8601(self.aar.get()['UsedCPUUserTime']),
            'cpukerneltime': duration_to_iso8601(self.aar.get()['UsedCPUKernelTime'])
        })
        # Start/EndTime (optional)
        xml += '<StartTime>{stime}</StartTime>' \
               '<EndTime>{etime}</EndTime>'.format(**{
            'stime': datetime_to_iso8601(self.aar.get()['SubmitTime']),
            'etime': datetime_to_iso8601(self.aar.get()['EndTime'])
        })
        # MachineName (optional)
        xml += '<MachineName>{0}</MachineName>'.format(self.aar.submithost())
        # Host (optional)
        if 'nodename' in aar_extra:
            primary_node = ' urf:primary="true"'
            for n in aar_extra['nodename'].split(':'):
                xml += '<Host{0}>{1}</Host>'.format(primary_node, n)
                primary_node = ''
        # SubmitHost (optional)
        if 'clienthost' in aar_extra:
            xml += '<SubmitHost>{0}</SubmitHost>'.format(aar_extra['clienthost'].split(':')[0])
        # Queue (optional)
        xml += self._add_aar_node('Queue', 'Queue')
        # ProjectName (optional)
        xml += self._add_extra_node('ProjectName', 'projectname')
        # NodeCount, Processors
        xml += self._add_aar_node('NodeCount', 'NodeCount')
        xml += self._add_aar_node('Processors', 'CPUCount')
        # Network (optional, new in jura-ng)
        network = self.aar.get()['StageInVolume'] + self.aar.get()['StageOutVolume']
        xml += '<Network urf:storageUnit="B" urf:metric="total">{0}</Network>'.format(network)
        # Disk (optional, new in jura-ng)
        scratch = self.aar.get()['UsedScratch']
        if scratch:
            xml += '<Disk urf:storageUnit="B" urf:metric="total" urf:type="scratch">{0}</Disk>'.format(scratch)
        # ServiceLevel (optional, new in jura-ng)
        xml += self._add_extra_node('ServiceLevel', 'wninstance')
        return xml

    def __rte_data(self):
        """Return ARC RTE extension XML"""
        xml = ''
        for rte in self.aar.rtes():
            xml += '<arc:RuntimeEnvironment>{0}</arc:RuntimeEnvironment>'.format(rte)
        return xml

    def __dtr_data(self):
        """Return FileTransfers extension XML"""
        xml = ''
        for dtr in self.aar.datatransfers():
            if dtr['type'] == 'output':
                xml += self.__xml_templates['dtr-output'].format(**{
                    'url': dtr['url'],
                    'size': dtr['size'],
                    'tstart': datetime_to_iso8601(dtr['timestart']),
                    'tend': datetime_to_iso8601(dtr['timeend']),
                })
            else:
                xml += self.__xml_templates['dtr-input'].format(**{
                    'url': dtr['url'],
                    'size': dtr['size'],
                    'tstart': datetime_to_iso8601(dtr['timestart']),
                    'tend': datetime_to_iso8601(dtr['timeend']),
                    'cache': 'true' if dtr['type'] == 'cache_input' else 'false'
                })
        if xml:
            xml = '<tr:FileTransfers>{0}</tr:FileTransfers>'.format(xml)
        return xml

    def __create_xml(self):
        """Create UR XML concatenating parts of the information"""
        xml = self.__record_identity()
        xml += self.__job_identity()
        xml += self.__user_identity()
        xml += self.__job_data()
        xml += self.__rte_data()
        xml += self.__dtr_data()
        self.xml += self.__xml_templates['ur-base'].format(**{'jobrecord': xml})

    @staticmethod
    def batch_header():
        return '<?xml version="1.0" encoding="utf-8"?>\n' \
               '<UsageRecords xmlns="http://schema.ogf.org/urf/2003/09/urf">'

    @staticmethod
    def join_str():
        return ''

    @staticmethod
    def batch_footer():
        return '</UsageRecords>'


class ComputeAccountingRecord(JobAccountingRecord):
    """Class representing EMI CAR v1.2 format of job accounting records"""
    __xml_templates = {
        'ur-base': '<UsageRecord xmlns:urf="http://eu-emi.eu/namespaces/2012/11/computerecord" '
                   'xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" '
                   'xsi:schemaLocation="http://eu-emi.eu/namespaces/2012/10/computerecord car_v1.2.xsd">'
                   '{jobrecord}</UsageRecord>',
        'record-id': '<RecordIdentity urf:createTime="{createtime}" urf:recordId="{recordid}"/>',
        'job-id': '<JobIdentity><GlobalJobId>{globalid}</GlobalJobId><LocalJobId>{localid}</LocalJobId></JobIdentity>',
        'user-id': '<UserIdentity><GlobalUserName urf:type="opensslCompat">{globalid}</GlobalUserName>'
                   '{localuser}{localgroup}{groups}</UserIdentity>',
    }

    def __init__(self, aar, vomsless_vo=None, extra_vogroups=None, gocdb_name=None):
        """Create XML representation of Compute Accounting Record"""
        JobAccountingRecord.__init__(self, aar)
        self.logger = logging.getLogger('ARC.Accounting.CAR')
        self.vomsless_vo = vomsless_vo
        self.extra_vogroups = extra_vogroups
        if self.extra_vogroups is None:
            self.extra_vogroups = []
        self.gocdb_name = gocdb_name
        self.__create_xml()

    def __record_identity(self):
        """Construct RecordIdentity"""
        return self.__xml_templates['record-id'].format(**{
            'createtime': datetime_to_iso8601(datetime.datetime.today()),
            'recordid': 'ur-' + self.aar.submithost() + '-' + self.aar.get()['JobID']
        })

    def __job_identity(self):
        """Construct JobIdentity"""
        # Both GlobalJobId and LocalJobID are mandatory for grid jobs
        # ProcessID is another optional value we don't have
        localid = self.aar.get()['LocalJobID']
        globalid = self.aar.get()['JobID']
        if not localid:
            localid = globalid
        return self.__xml_templates['job-id'].format(**{
            'globalid': globalid,
            'localid': localid
        })

    def __user_groups_xml(self):
        xml = ''
        # Group (optional recommended) - the effective User VO of the user running the job
        wlcgvo = self.aar.wlcgvo()
        if not wlcgvo:
            # arc.conf: vomsless VO
            if self.vomsless_vo is not None:
                vomsless_data = self.vomsless_vo.split('#')
                wlcgvo = vomsless_data[0]
        if wlcgvo:
            xml += '<Group>{0}</Group>'.format(wlcgvo)
        else:
            return xml
        # VOMS FQAN info
        # EMI CAR and APEL has a concept of effective FQAN.
        # ARC writes main (first) FQAN as a dedicated entry in AuthTokenAttributes to match this case.
        vomsfqans = [t[1] for t in self.aar.authtokens() if t[0] == 'mainfqan']
        fqangroups = []
        if vomsfqans:
            fqan = vomsfqans[0]  # there should not be more than one
            (fqan_group, fqan_role, _) = get_fqan_components(fqan)
            if fqan_group is not None:
                xml += '<GroupAttribute urf:type="FQAN">{0}</GroupAttribute>'.format(fqan)
                xml += '<GroupAttribute urf:type="vo-group">{0}</GroupAttribute>'.format(fqan_group)
                fqangroups.append(fqan_group)
                if fqan_role is not None:
                    xml += '<GroupAttribute urf:type="vo-role">{0}</GroupAttribute>'.format(fqan_role)
        else:
            # arc.conf: vo_group attributes
            # as APEL works only with single FQAN, it has no sense to add more if already defined
            # also this code adds only the first one if user put many in arc.conf
            for vg in self.extra_vogroups:
                if vg not in fqangroups:  # eliminate duplicates
                    xml += '<GroupAttribute urf:type="vo-group">{0}</GroupAttribute>'.format(vg)
                    break
        # ProjectName represented as GroupAttribute in CAR
        if 'projectname' in self.aar.extra():
            xml += '<GroupAttribute urf:type="ProjectName">{0}</GroupAttribute>'.format(self.aar.extra()['projectname'])
        # Return XML
        return xml

    def __user_identity(self):
        """Construct UserIdentity"""
        # LocalUserId is mandatory (nobody is fallback)
        # LocalGroup is recommended optional
        localuser = 'nobody'
        localgroup = ''
        # Check data in AAR extra info
        aar_extra = self.aar.extra()
        if 'localuser' in aar_extra:
            localuserdata = aar_extra['localuser'].split(':', 2)
            localuser = localuserdata[0]
            if len(localuserdata) > 1:
                localgroup = localuserdata[1]
        # Create XML
        localuser = '<LocalUserId>{0}</LocalUserId>'.format(localuser)
        if localgroup:
            localgroup = '<LocalGroup>{0}</LocalGroup>'.format(localgroup)
        # GlobalUserName is optional (we always have the UserSN)
        # Group and GroupAttributes are optional recommended
        return self.__xml_templates['user-id'].format(**{
            'globalid': self.aar.get()['UserSN'],
            'localuser': localuser,
            'localgroup': localgroup,
            'groups': self.__user_groups_xml()
        })

    def __job_data(self):
        """Return XML nodes for resource usage and other informational job attributes in AAR"""
        xml = ''
        aar_extra = self.aar.extra()
        # JobName (optional)
        xml += self._add_extra_node('JobName', 'jobname')
        # Status (mandatory)
        xml += self._add_aar_node('Status', 'Status')
        # ExitStatus (optional)
        xml += self._add_aar_node('ExitStatus', 'ExitCode')
        # Wall/CPUDuration (mandatory)
        xml += '<WallDuration>{walltime}</WallDuration>' \
               '<CpuDuration urf:usageType="all">{cputime}</CpuDuration>' \
               '<CpuDuration urf:usageType="user">{cpuusertime}</CpuDuration>' \
               '<CpuDuration urf:usageType="system">{cpukerneltime}</CpuDuration>'.format(**{
            'walltime': duration_to_iso8601(self.aar.get()['UsedWalltime']),
            'cputime': duration_to_iso8601(self.aar.get()['UsedCPUTime']),
            'cpuusertime': duration_to_iso8601(self.aar.get()['UsedCPUUserTime']),
            'cpukerneltime': duration_to_iso8601(self.aar.get()['UsedCPUKernelTime'])
        })
        # Start/EndTime (mandatory)
        xml += '<StartTime>{stime}</StartTime>' \
               '<EndTime>{etime}</EndTime>'.format(**{
            'stime': datetime_to_iso8601(self.aar.get()['SubmitTime']),
            'etime': datetime_to_iso8601(self.aar.get()['EndTime'])
        })
        # MachineName (mandatory)
        #   According the CAR specification: This SHOULD be the LRMS server host name.
        #   We do not have such info. Old jura reports ARC host name.
        xml += '<MachineName>{0}</MachineName>'.format(self.aar.submithost())
        # Host (optional, recommended)
        if 'nodename' in aar_extra:
            primary_node = ' urf:primary="true"'
            for n in aar_extra['nodename'].split(':'):
                xml += '<Host{0}>{1}</Host>'.format(primary_node, n)
                primary_node = ''
        # SubmitHost (mandatory)
        # Unlike OGF.98 in CAR "this MUST report the Computing Element Uniqe ID"
        xml += '<SubmitHost urf:type="{Interface}">{EndpointURL}</SubmitHost>'.format(**self.aar.get())
        # Queue (mandatory)
        xml += '<Queue urf:description="execution">{0}</Queue>'.format(self.aar.get()['Queue'])
        # Site (mandatory)
        xml += '<Site urf:type="gocdb">{0}</Site>'.format(self.gocdb_name)
        # Infrastructure (mandatory)
        lrms = aar_extra['lrms'] if 'lrms' in aar_extra else 'fork'
        xml += '<Infrastructure urf:type="grid" urf:description="JURA-ARC-{0}"></Infrastructure>'.format(lrms.upper())
        # Middleware (mandatory)
        xml += '<Middleware urf:name="arc" urf:version="{0}" urf:description="NorduGrid ARC {0}"/>'.format(ARC_VERSION)
        # Memory (optional)
        xml += '<Memory urf:storageUnit="KB" urf:metric="max" urf:type="Shared">{virtual}</Memory>' \
               '<Memory urf:storageUnit="KB" urf:metric="max" urf:type="Physical">{physical}</Memory>'.format(**{
            'virtual': self.aar.get()['UsedVirtMem'],
            'physical': self.aar.get()['UsedMemory']
        })
        # Swap (optional) we don't have
        # NodeCount, Processors (optional, recommended)
        xml += self._add_aar_node('NodeCount', 'NodeCount')
        xml += self._add_aar_node('Processors', 'CPUCount')
        # TimeInstant (optional)
        #   Technically we can publish this based on JobEvents but there was no such info previously
        #   and looks like it is more theoretical rather than for real analyses (especially in view of APEL summaries)
        # ServiceLevel (mandatory) represents benchmarking normalization
        if 'benchmark' not in aar_extra:
            aar_extra['benchmark'] = ''
        (bechmark_type, bechmark_value) = get_apel_benchmark(self.logger, aar_extra['benchmark'])
        xml += '<ServiceLevel urf:type="{0}">{1}</ServiceLevel>'.format(bechmark_type, bechmark_value)
        return xml

    def __create_xml(self):
        """Create CAR XML concatenating parts of the information"""
        xml = self.__record_identity()
        xml += self.__job_identity()
        xml += self.__user_identity()
        xml += self.__job_data()
        self.xml += self.__xml_templates['ur-base'].format(**{'jobrecord': xml})

    @staticmethod
    def batch_header():
        return '<?xml version="1.0" encoding="utf-8"?>\n' \
               '<UsageRecords xmlns="http://eu-emi.eu/namespaces/2012/11/computerecord">'

    @staticmethod
    def join_str():
        return ''

    @staticmethod
    def batch_footer():
        return '</UsageRecords>'


class APELSummaryRecord(object):
    """Class representing APEL Summary Record"""
    __voinfo_template = '''
VO: {wlcgvo}
VOGroup: {fqangroup}
VORole: {fqanrole}'''

    __record_template = '''Site: {gocdb_name}
Month: {month}
Year: {year}
GlobalUserName: {userdn}{voinfo}
SubmitHost: {endpoint}
InfrastructureType: grid
ServiceLevelType: {benchmark_type}
ServiceLevel: {benchmark_value}
NodeCount: {nodecount}
Processors: {cpucount}
EarliestEndTime: {timestart}
LatestEndTime: {timeend}
WallDuration: {walltime}
CpuDuration: {cputime}
NumberOfJobs: {count}'''

    def __init__(self, recorddata, gocdb_name):
        self.logger = logging.getLogger('ARC.Accounting.APELSummary')
        self.recorddata = recorddata
        self.recorddata['gocdb_name'] = gocdb_name
        self.recorddata['voinfo'] = self.__vo_info()
        (benchmark_type, benchmark_value) = get_apel_benchmark(self.logger, self.recorddata['benchmark'])
        self.recorddata['benchmark_type'] = benchmark_type
        self.recorddata['benchmark_value'] = benchmark_value

    def __vo_info(self):
        """Parse WLCG VO info"""
        if self.recorddata['wlcgvo'] is None:
            return ''
        wlcgvo = self.recorddata['wlcgvo']
        (group, role) = ('/' + wlcgvo, 'Role=NULL')
        if self.recorddata['fqan'] is not None:
            (fqan_group, fqan_role, _) = get_fqan_components(self.recorddata['fqan'])
            if fqan_group is not None:
                group = fqan_group
            if fqan_role is not None:
                role = fqan_role
        return self.__voinfo_template.format(**{
            'wlcgvo': wlcgvo,
            'fqangroup': group,
            'fqanrole': role
        })

    def get_record(self):
        return self.__record_template.format(**self.recorddata)

    @staticmethod
    def header():
        return 'APEL-summary-job-message: v0.2\n'

    @staticmethod
    def join_str():
        return '\n%%\n'

    @staticmethod
    def footer():
        return '\n%%\n'


class APELSyncRecord(object):
    """Class representing APEL Sync Record"""
    __record_template = '''Site: {gocdb_name}
SubmitHost: {endpoint}
NumberOfJobs: {count}
Month: {month}
Year: {year}'''

    def __init__(self, recorddata, gocdb_name):
        self.logger = logging.getLogger('ARC.Accounting.APELSync')
        self.recorddata = recorddata
        self.recorddata['gocdb_name'] = gocdb_name

    def get_record(self):
        return self.__record_template.format(**self.recorddata)

    @staticmethod
    def header():
        return 'APEL-sync-message: v0.1\n'

    @staticmethod
    def join_str():
        return '\n%%\n'

    @staticmethod
    def footer():
        return '\n%%\n'
