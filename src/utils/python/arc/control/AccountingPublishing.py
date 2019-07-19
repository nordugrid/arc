from __future__ import print_function
from __future__ import absolute_import

from .AccountingDB import AccountingDB, AAR

from arc.paths import ARC_VERSION

import sys
import logging
import re
import datetime
import time

import random

__voms_fqan_re = re.compile(r'(?P<group>/[-\w.]+(?:/[-\w.]+)*)(?P<role>/Role=[-\w.]+)?(?P<cap>/Capability=[-\w.]+)?')

__url_re = re.compile(r'^(?P<url>(?P<protocol>[^:/]+)://(?P<host>[^:/]+)(?::(?P<port>[0-9]+))?/*(?:(?P<path>/.*))?)$')


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
        accounting_dir = arcconfig.get_value('controldir', 'arex').rstrip('/')
        adb_file = accounting_dir + '/accounting.db'
        self.adb = AccountingDB(adb_file)
        # publishing state database file
        self.pdb_file = accounting_dir + '/publishing.db'

    def __del__(self):
        if self.adb is not None:
            del self.adb
            self.adb = None

    def __get_target_confdict(self, target):
        """Get accounting target configuration and check mandatory options"""
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

    def __config_benchmark_value(self, confdict):
        """Return fallback benchmark value in accordance to arc.conf"""
        if 'benchmark_type' in confdict and 'benchmark_value' in confdict:
            return '{benchmark_type}:{benchmark_value}'.format(**confdict)
        self.logger.warning('There is no benchmark value defined in the accounting target configuration. '
                            'All records without associated benchmark value will be reported with "HEPSPEC:1.0"')
        return 'HEPSPEC:1.0'

    def __add_vo_filter(self, target_conf):
        """Add WLCG VO filtering if defined in arc.conf"""
        if 'vofilter' in target_conf:
            vos = []
            if not isinstance(target_conf['vofilter'], list):
                vos.append(target_conf['vofilter'])
            else:
                vos += target_conf['vofilter']
            self.logger.info('VO filtering is configured. Quierying jobs owned by this WLCG VO(s): %s', ','.join(vos))
            self.adb.filter_wlcgvos(vos)

    def __init_adb_filters(self, endfrom, endtill):
        self.adb.filters_clean()
        # timerange filters first
        self.adb.filter_endfrom(endfrom)
        if endtill is not None:
            self.adb.filter_endtill(endtill)
        # only finished jobs
        self.adb.filter_statuses(['completed', 'failed'])

    def publish_sgas(self, target_conf, endfrom, endtill=None):
        """Publish to SGAS target"""
        self.__init_adb_filters(endfrom, endtill)
        # optional WLCG VO filtering
        self.__add_vo_filter(target_conf)
        # query latest job endtime in this records query
        latest_endtime = self.adb.get_latest_endtime()
        if latest_endtime is None:
            self.logger.info('There are no records to publish. Nothing to do')
            return None

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
            self.logger.error('Failed to build OGF.98 Usage Records for SGAS publishing.')
            return None
        # TODO: real publishing
        print('<?xml version="1.0" encoding="utf-8"?>')
        print(random.choice(urs).get_xml())
        # no failures on the way: return latest endtime for records published
        return latest_endtime

    def publish_apel(self, target_conf, endfrom, endtill=None):
        """Publish to all APEL targets"""
        self.__init_adb_filters(endfrom, endtill)
        # optional WLCG VO filtering
        self.__add_vo_filter(target_conf)
        # query latest job endtime in this records query
        latest_endtime = self.adb.get_latest_endtime()
        if latest_endtime is None:
            self.logger.info('There are no records to publish. Nothing to do.')
            return None
        # get fallback benchmark values from config
        conf_benchmark = self.__config_benchmark_value(target_conf)
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
            cars = [ComputeAccountingRecord(aar, gocdb_name=target_conf['gocdb_name'], benchmark=conf_benchmark,
                           vomsless_vo=self.vomsless_vo, extra_vogroups=self.extra_vogroups) for aar in aars]
            if not cars:
                self.logger.error('Failed to build EMI CAR v1.2 for APEL publishing.')
                return None
            # TODO: write it down to target outgoing
        if target_conf['apel_messages'] == 'summaries' or target_conf['apel_messages'] == 'both':
            # summary records
            self.logger.debug('Querying APEL Summary data from accounting database')
            apelsummaries = [
                APELSummaryRecord(rec, target_conf['gocdb_name'], conf_benchmark).get_record()
                for rec in self.adb.get_apel_summaries()
            ]
            if not apelsummaries:
                self.logger.error('Failed to build APEL summaries for publishing.')
                return None
            # TODO: write it down to target outgoing
            print('=========SUMMARIES==========')
            print(APELSummaryRecord.join_str().join(apelsummaries))
        # Sync messages are always present
        self.logger.debug('Querying APEL Sync data from accounting database')
        apelsyncs = [
            APELSyncRecord(rec, target_conf['gocdb_name']).get_record()
            for rec in self.adb.get_apel_sync()
        ]
        # TODO: write it down to target outgoing
        print('=========SYNC==========')
        print(APELSyncRecord.join_str().join(apelsyncs))
        # no failures on the way: return latest endtime for records published
        return latest_endtime

    def find_configured_target(self, targetname):
        for (ttype, target) in self.conf_targets:
            (_, tname) = target.split(':')
            if targetname == tname.strip():
                targetconf = self.__get_target_confdict(target)
                return targetconf, ttype
        return None, None

    def republish(self, targettype, targetconf, endfrom, endtill=None):
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
                    self.logger.error(
                        'Legacy fallback enabled in the configuration for target in [%s] block. Skipping.', target)
                continue
            # constrains on reporting frequency
            if 'urdelivery_frequency' in targetconf:
                lastreported = self.adb.get_last_report_time(target)
                unixtime_now = time.mktime(datetime.datetime.today().timetuple())
                if (unixtime_now - lastreported) < int(targetconf['urdelivery_frequency']):
                    self.logger.debug('Records are reported to [%s] target less than %s seconds ago. '
                                      'Skipping publishing during this run due to "urdelivery_frequency" constraint.',
                                      target, targetconf['urdelivery_frequency'])
                    continue
            # fetch latest published record timestamp for this target
            self.adb.attach_publishing_db(self.pdb_file)
            endfrom = self.adb.get_last_published_endtime(target)
            self.logger.info('Publishing latest accounting records to [%s] target (finished since %s).',
                             target, datetime.datetime.fromtimestamp(endfrom))
            # do publishing
            latest = None
            if ttype == 'apel':
                latest = self.publish_apel(targetconf, endfrom)
            elif ttype == 'sgas':
                latest = self.publish_sgas(targetconf, endfrom)
            # update latest published record timestamp
            if latest is not None:
                self.adb.set_last_published_endtime(target, latest)


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
        vomsfqans = [t[1] for t in self.aar.authtokens() if t[0] == 'vomsfqan' or t == 'mainfqan']
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

    def __init__(self, aar, vomsless_vo=None, extra_vogroups=None, gocdb_name=None, benchmark=None):
        """Create XML representation of Compute Accounting Record"""
        JobAccountingRecord.__init__(self, aar)
        self.logger = logging.getLogger('ARC.Accounting.CAR')
        self.vomsless_vo = vomsless_vo
        self.extra_vogroups = extra_vogroups
        if self.extra_vogroups is None:
            self.extra_vogroups = []
        self.gocdb_name = gocdb_name
        self.benchmark = benchmark
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
        # LocalUserId is mandatory
        localuser = self._add_extra_node('LocalUserId', 'localuser')
        if not localuser:
            localuser = '<LocalUserId>nobody</LocalUserId>'
        # LocalGroup is recommended but optional (TODO: consider to add to AAR extended attributes)
        localgroup = self._add_extra_node('LocalGroup', 'localgroup')
        # GlobalUserName is optional (we always have the UserSN)
        # Group and GroupAttributes are optional recommended
        return self.__xml_templates['user-id'].format(**{
            'globalid': self.aar.get()['UserSN'],
            'localuser': localuser,
            'localgroup': localgroup,
            'groups': self.__user_groups_xml()
        })

    def __filter_benchmark(self, benchmark):
        """Use only APEL supported benchmarks"""
        __allowed_benchmark_types = ['Si2k', 'HEPSPEC']
        bsplit = benchmark.split(':', 2)
        if len(bsplit) != 2:
            self.logger.error('Malformed benchmark definition string: "HEPSPEC:1.0" will be used instead.')
            return 'HEPSPEC', '1.0'
        (benchmark_type, benchmark_value) = bsplit
        if benchmark_type not in __allowed_benchmark_types:
            self.logger.error('Benchmark type %s in not supported by APEL. "HEPSPEC:1.0" will be used instead.',
                              benchmark_type)
            return 'HEPSPEC', '1.0'
        return benchmark_type, benchmark_value

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
            aar_extra['benchma'] = self.benchmark if self.benchmark is not None else ''
        (bechmark_type, bechmark_value) = self.__filter_benchmark(aar_extra['benchmark'])
        xml += '<ServiceLevel urf:type="{0}">{1}</ServiceLevel>'.format(bechmark_type, bechmark_value)
        return xml

    def __create_xml(self):
        """Create CAR XML concatenating parts of the information"""
        xml = self.__record_identity()
        xml += self.__job_identity()
        xml += self.__user_identity()
        xml += self.__job_data()
        self.xml += self.__xml_templates['ur-base'].format(**{'jobrecord': xml})


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

    def __init__(self, recorddata, gocdb_name, conf_benchmark):
        self.recorddata = recorddata
        self.recorddata['gocdb_name'] = gocdb_name
        self.recorddata['voinfo'] = self.__vo_info()
        if self.recorddata['benchmark'] is None:
            self.recorddata['benchmark'] = conf_benchmark
        self.__benchmark()

    def __benchmark(self):
        """Use only APEL supported benchmarks of fallback to HEPSPEC:1.0"""
        __allowed_benchmark_types = ['Si2k', 'HEPSPEC']
        self.recorddata['benchmark_type'] = 'HEPSPEC'
        self.recorddata['benchmark_value'] = '1.0'
        bsplit = self.recorddata['benchmark'].split(':', 2)
        if len(bsplit) != 2:
            return
        (benchmark_type, benchmark_value) = bsplit
        if benchmark_type not in __allowed_benchmark_types:
            return
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
        return 'APEL-summary-job-message: v0.2'

    @staticmethod
    def join_str():
        return '\n%%\n'

    @staticmethod
    def footer():
        return '%%'


class APELSyncRecord(object):
    """Class representing APEL Sync Record"""
    __record_template = '''Site: {gocdb_name}
SubmitHost: {endpoint}
NumberOfJobs: {count}
Month: {month}
Year: {year}'''

    def __init__(self, recorddata, gocdb_name):
        self.recorddata = recorddata
        self.recorddata['gocdb_name'] = gocdb_name

    def get_record(self):
        return self.__record_template.format(**self.recorddata)

    @staticmethod
    def header():
        return 'APEL-sync-message: v0.1'

    @staticmethod
    def join_str():
        return '\n%%\n'

    @staticmethod
    def footer():
        return '%%'
