from __future__ import print_function
from __future__ import absolute_import

from .AccountingDB import AccountingDB, AAR

from arc.paths import ARC_VERSION

import sys
import logging
import re
import datetime

import random

__voms_fqan_re = re.compile(r'(?P<group>/[-\w.]+(?:/[-\w.]+)*)(?P<role>/Role=[-\w.]+)?(?P<cap>/Capability=[-\w.]+)?')

__allowed_benchmark_types = ['Si2k', 'Sf2k', 'HEPSPEC']


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
    return (group, role, capability)


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


def __next_month_start(t):
    """Helper function to return datetime for the next calendar month start"""
    nextmonth = t.month + 1
    nextyear = t.year
    if nextmonth == 13:
        nextmonth = 1
        nextyear += 1
    return datetime.datetime(nextyear, nextmonth, 1, 0, 0, 0)


def split_timeframe(starttime, endtime):
    """Return list of timeframe tuples that split requested range on monthly basis"""
    timeframes = []
    framestart = starttime
    nextmonth = __next_month_start(starttime)
    while endtime > nextmonth:
        timeframes.append((framestart, nextmonth))
        framestart = nextmonth
        nextmonth = __next_month_start(framestart)
    timeframes.append((framestart, endtime))
    return timeframes


class RecordsPublisher(object):
    """Class for publishing accounting records to central network services"""
    def __init__(self, arcconfig):
        self.logger = logging.getLogger('ARC.Accounting.Publisher')
        self.adb = None
        # arc config
        if arcconfig is None:
            self.logger.error('Failed to parse arc.conf. Accounting publihsing is not possible.')
            sys.exit(1)
        # get configured accounting targets and options
        self.vomsless_vo = arcconfig.get_value('vomsless_vo', ['arex/jura'])
        self.extra_vogroups = arcconfig.get_value('vo_group', ['arex/jura'], force_list=True)
        self.sgas_blocks = arcconfig.get_subblocks('arex/jura/sgas')
        self.apel_blocks = arcconfig.get_subblocks('arex/jura/apel')
        self.confdict = arcconfig.get_config_dict(['arex/jura'] + self.sgas_blocks + self.apel_blocks)
        # accounting database connection
        adb_file = arcconfig.get_value('controldir', 'arex').rstrip('/') + '/accounting.db'
        self.adb = AccountingDB(adb_file)

    def __del__(self):
        if self.adb is not None:
            del self.adb
            self.adb = None

    def __vo_filter(self, target_conf):
        # add WLCG VO filtering if defined in arc.conf
        if 'vofilter' in target_conf:
            vos = []
            if not isinstance(target_conf['vofilter'], list):
                vos.append(target_conf['vofilter'])
            else:
                vos += target_conf['vofilter']
            self.logger.info('VO filtering is configured. Quierying jobs owned by this WLCG VO(s): %s', ','.join(vos))
            self.adb.filter_wlcgvos(vos)

    def publish_sgas(self, target_conf):
        """Publish to SGAS target"""
        self.adb.filters_clean()
        self.adb.filter_statuses(['completed', 'failed'])
        self.__vo_filter(target_conf)
        # TODO: Apply timerange filters interval in target_conf or query the database for latest
        # get AARs from database
        self.logger.debug('Querying AARS from accounting database')
        aars = self.adb.get_aars(resolve_ids=True)
        self.logger.info('Accounting database query returned %s AARs', len(aars))
        self.logger.debug('Querying additional job information about AARs')
        self.adb.enrich_aars(aars, authtokens=True, rtes=True, dtrs=True, extra=True)
        # get UR parameters
        localid_prefix = target_conf['localid_prefix'] if 'localid_prefix' in target_conf else None
        # create URs
        self.logger.debug('Going to create OGF.98 Usage Records based on the AARs')
        urs = [UsageRecord(aar, localid_prefix=localid_prefix,
                           vomsless_vo=self.vomsless_vo, extra_vogroups=self.extra_vogroups) for aar in aars]
        # TODO: real publishing
        print('<?xml version="1.0" encoding="utf-8"?>')
        print(random.choice(urs).get_xml())

    def __config_benchmark_value(self, confdict):
        if 'benchmark_type' in confdict and 'benchmark_value' in confdict:
            if confdict['benchmark_type'] in __allowed_benchmark_types:
                return '{benchmark_type}:{benchmark_value}'.format(**confdict)
            self.logger.error('Unsupported benchmark type %s in the arc.conf configuration. '
                              'Falling back to default "Si2k:1.0"')
            return 'Si2k:1.0'
        self.logger.warning('There is no benchmark value defined in the accounting target configuration. '
                            'All records without acounted benchmark will be reported with "Si2k:1.0"')
        return 'Si2k:1.0'

    def publish_apel(self, target_conf):
        """Publish to all APEL targets"""
        self.adb.filters_clean()
        self.adb.filter_statuses(['completed', 'failed'])
        self.__vo_filter(target_conf)
        # TODO: Apply timerange filters interval in target_conf or query the database for latest
        # get AARs from database
        self.logger.debug('Querying AARS from accounting database')
        aars = self.adb.get_aars(resolve_ids=True)
        self.logger.info('Accounting database query returned %s AARs', len(aars))
        self.logger.debug('Querying additional job information about AARs')
        self.adb.enrich_aars(aars, authtokens=True, rtes=True, extra=True)
        # get UR parameters
        gocdb_name = target_conf['gocdb_name']
        conf_benchmark = self.__config_benchmark_value(target_conf)
        # create URs
        self.logger.debug('Going to create EMI CAR v1.2 Compute Accounting Records based on the AARs')
        cars = [ComputeAccountingRecord(aar, gocdb_name=gocdb_name, benchmark=conf_benchmark,
                           vomsless_vo=self.vomsless_vo, extra_vogroups=self.extra_vogroups) for aar in aars]
        # summary records
        summaries = self.adb.get_apel_summaries()
        apelsummaries = [
            APELSummaryRecord(rec, datetime.datetime.today(), gocdb_name, conf_benchmark).get_record()
            for rec in summaries
        ]
        print('\n%%\n'.join(apelsummaries))
        # TODO: real publishing
        # print('<?xml version="1.0" encoding="utf-8"?>')
        # print(random.choice(cars).get_xml())

    def publish(self):
        """Publish records"""
        # TODO: RECORDS SELECTION MOST PROBABLY PER TARGET OR INTEGRATED INTERVAL QUERY
        # TODO: interval from the command line for republishing
        # TODO: dedicated database to track what is published (for regular publishing, but not for republishing)
        for sb in self.sgas_blocks:
            if 'targeturl' not in self.confdict[sb]:
                self.logger.error('Target URL is mandatory for SGAS publishing but missing in [%s] block.'
                                  'Block processing skipped.')
                continue
            self.publish_sgas(self.confdict[sb])

        for ab in self.apel_blocks:
            # mandatory config options check
            if 'targeturl' not in self.confdict[ab]:
                self.logger.error('Target URL is mandatory for APEL publishing but missing in [%s] block.'
                                  'Block processing skipped.', ab)
                continue
            if 'gocdb_name' not in self.confdict[ab]:
                self.logger.error('GOCDB name is required for APEL publishing but missing in [%s] block.'
                                  'Records will not be reported to %s target.', ab, self.confdict[ab]['targeturl'])
                continue
            self.publish_apel(self.confdict[ab])


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
        #   and looks like it is more theoretical rather than for real analyses
        # ServiceLevel (mandatory) represents benchmarking normalization
        bechmark_type = 'Si2k'
        bechmark_value = '1.0'
        if self.benchmark is not None:
            (bechmark_type, bechmark_value) = self.benchmark.split(':', 2)
        if 'benchmark' in aar_extra:
            (b_type, b_value) = aar_extra['benchmark'].split(':', 2)
            # value from AAR first
            if aar_extra['benchmark_type'] in __allowed_benchmark_types:
                bechmark_type = b_type
                bechmark_value = b_value if b_value else '1.0'
            else:
                self.logger.error('Unsupported benchmark type %s in the usage record for job %s.',
                                  b_type, self.aar.get()['JobID'])
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

    def __init__(self, recorddata, timeframe, gocdb_name, conf_benchmark):
        self.recorddata = recorddata
        self.recorddata['month'] = timeframe.month
        self.recorddata['year'] = timeframe.year
        self.recorddata['gocdb_name'] = gocdb_name
        self.recorddata['voinfo'] = self.__vo_info()
        if self.recorddata['benchmark'] is None:
            self.recorddata['benchmark'] = conf_benchmark
        self.__benchmark()

    def __benchmark(self):
        (type, value) = self.recorddata['benchmark'].split(':', 2)
        self.recorddata['benchmark_type'] = type
        self.recorddata['benchmark_value'] = value

    def __vo_info(self):
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
