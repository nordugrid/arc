from __future__ import print_function
from __future__ import absolute_import

from .AccountingDB import AccountingDB, AAR

from arc.paths import ARC_VERSION

import sys
import datetime
import logging
import re

import random


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
    def __init__(self, arcconfig):
        self.logger = logging.getLogger('ARC.Accounting.Publisher')
        self.adb = None
        # arc config
        if arcconfig is None:
            self.logger.error('Failed to parse arc.conf. Accounting publihsing is not possible.')
            sys.exit(1)
        # get configured accounting targets and options
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

    def publish_sgas(self, aars):
        """Publish to all SGAS targets"""
        for sb in self.sgas_blocks:
            # prepare all-in-one config options dict
            self.confdict[sb].update(self.confdict['arex/jura'])
            # mandatory config options check
            if 'targeturl' not in self.confdict[sb]:
                self.logger.error('Target URL is mandatory for SGAS publishing but missing in [%s] block.'
                                  'Block processing skipped.', sb)
                continue
            # create URs
            urs = [UsageRecord(aar, self.confdict[sb]) for aar in aars]
            # TODO: real publishing
            # print('<?xml version="1.0" encoding="utf-8"?>')
            # print(random.choice(urs).get_xml())

    def publish_apel(self, aars):
        """Publish to all APEL targets"""
        for ab in self.apel_blocks:
            # prepare all-in-one config options dict
            self.confdict[ab].update(self.confdict['arex/jura'])
            # mandatory config options check
            if 'targeturl' not in self.confdict[ab]:
                self.logger.error('Target URL is mandatory for APEL publishing but missing in [%s] block.'
                                  'Block processing skipped.', ab)
                continue
            if 'gocdb_name' not in self.confdict[ab]:
                self.logger.error('GOCDB name is required for APEL publishing but missing in [%s] block.'
                                  'Records will not be reported to %s target.', ab, self.confdict[ab]['targeturl'])
                continue
            cars = [ComputeAccountingRecord(aar, self.confdict[ab]) for aar in aars]
            # TODO: real publishing
            print('<?xml version="1.0" encoding="utf-8"?>')
            print(random.choice(cars).get_xml())

    def publish(self):
        """Publish records"""
        # TODO: RECORDS SELECTION MOST PROBABLY PER TARGET OR INTEGRATED INTERVAL QUERY
        # TODO: interval from the command line for republishing
        # TODO: dedicated database to track what is published (for regular publishing, but not for republishing)

        # get records to publish
        aars = self.adb.get_aars(resolve_ids=True)
        fetch_dtrs = True if self.sgas_blocks else False  # dtrs are only published to SGAS
        self.adb.enrich_aars(aars, authtokens=True, rtes=True, dtrs=fetch_dtrs, extra=True)

        # publish to SGAS
        if self.sgas_blocks:
            self.publish_sgas(aars)

        # publish to APEL
        if self.apel_blocks:
            self.publish_apel(aars)


class AccountingRecord(object):
    """Base class for representing Accounting Records that implements common methots for all formats"""
    _voms_fqan_re = re.compile(
        r'(?P<group>/[-\w.]+(?:/[-\w.]+)*)(?P<role>/Role=[-\w.]+)?(?P<cap>/Capability=[-\w.]+)?'
    )

    def __init__(self, aar, conf):
        """Create XML representation of Accounting Record"""
        self.xml = ''
        self.confdict = conf
        self.aar = aar  # type: AAR
        self.filtered = False

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

    def is_filtered(self):
        """Indicate whether this record should be filtered according to configuration"""
        return self.filtered


class UsageRecord(AccountingRecord):
    """Class representing SGAS (OFG.98 based) Usage Record format of accounting records"""
    __xml_templates = {
        'ur-base': '<JobUsageRecord xmlns:arc="http://www.nordugrid.org/ws/schemas/ur-arc" '
                   'xmlns:ds="http://www.w3.org/2000/09/xmldsig#" '
                   'xmlns:tr="http://www.sgas.se/namespaces/2010/10/filetransfer" '
                   'xmlns:urf="http://schema.ogf.org/urf/2003/09/urf" '
                   'xmlns:vo="http://www.sgas.se/namespaces/2009/05/ur/vo" '
                   'xmlns:xsd="http://www.w3.org/2001/XMLSchema" '
                   'xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">{jobrecord}</JobUsageRecord>',
        'record-id': '<RecordIdentity urf:createTime="{createtime}" urf:recordId="{recordid}"/>',
        'job-local-id': '<LocalJobId>{localid}</LocalJobId>',
        'job-id': '<JobIdentity><GlobalJobId>{globalid}</GlobalJobId>{localid}</JobIdentity>',
        'user-id': '<UserIdentity><GlobalUserName>{globalid}</GlobalUserName>{localid}{wlcgvos}</UserIdentity>',
        'vo-info': '<vo:VO vo:type="voms"><vo:Name>{voname}</vo:Name>{issuer}{attributes}</vo:VO>',
        'dtr-input': '<tr:FileDownload><tr:URL>{url}</tr:URL><tr:Size>{size}</tr:Size>'
                     '<tr:StartTime>{tstart}</tr:StartTime><tr:EndTime>{tend}</tr:EndTime>'
                     '<tr:RetrievedFromCache>{cache}</tr:RetrievedFromCache></tr:FileDownload>',
        'dtr-output': '<tr:FileUpload><tr:URL>{url}</tr:URL><tr:Size>{size}</tr:Size>'
                      '<tr:StartTime>{tstart}</tr:StartTime><tr:EndTime>{tend}</tr:EndTime></tr:FileUpload>'
    }

    def __init__(self, aar, conf):
        """Create XML representation of UsageRecord"""
        AccountingRecord.__init__(self, aar, conf)
        self.logger = logging.getLogger('ARC.Accounting.UR')
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
            if 'localid_prefix' in self.confdict:
                localid = self.confdict['localid_prefix'] + localid
            localid = self.__xml_templates['job-local-id'].format(**{
                'localid': localid
            })
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
            if 'vomsless_vo' in self.confdict:
                vomsless_data = self.confdict['vomsless_vo'].split('#')
                wlcgvo = vomsless_data[0]
                if len(vomsless_data) > 1:
                    voissuer = vomsless_data[1]
        # arc.conf: VO filtering
        if 'vofilter' in self.confdict:
            if not isinstance(self.confdict['vofilter'], list):
                self.confdict['vofilter'] = [self.confdict['vofilter']]
            if wlcgvo not in self.confdict['vofilter']:
                self.filtered = True
        if not wlcgvo:
            return vomsxml
        # VOMS FQAN info
        vomsfqans = [t[1] for t in self.aar.authtokens() if t[0] == 'vomsfqan']
        fqangroups = []
        fqan_xml = ''
        if vomsfqans:
            for fqan in vomsfqans:
                fqan_match = self._voms_fqan_re.match(fqan)
                if fqan_match:
                    fqan_dict = fqan_match.groupdict()
                    fqan_xml += '<vo:Attribute>'
                    if 'group' in fqan_dict and fqan_dict['group']:
                        fqangroups.append(fqan_dict['group'])
                        fqan_xml += '<vo:Group>{group}</vo:Group>'.format(**fqan_dict)
                    if 'role' in fqan_dict and fqan_dict['role']:
                        fqan_xml += '<vo:Role>{role}</vo:Role>'.format(**fqan_dict)
                    if 'cap' in fqan_dict and fqan_dict['cap']:
                        fqan_xml += '<vo:Capability>{cap}</vo:Capability>'.format(**fqan_dict)
                    fqan_xml += '</vo:Attribute>'
        # arc.conf: additional vo_group attributes
        if 'vo_group' in self.confdict:
            if not isinstance(self.confdict['vo_group'], list):
                self.confdict['vo_group'] = [self.confdict['vo_group']]
            for vg in self.confdict['vo_group']:
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


class ComputeAccountingRecord(AccountingRecord):
    """Class representing APEL CAR format of accounting records"""
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

    def __init__(self, aar, conf):
        """Create XML representation of Compute Accounting Record"""
        AccountingRecord.__init__(self, aar, conf)
        self.logger = logging.getLogger('ARC.Accounting.CAR')
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
            if 'vomsless_vo' in self.confdict:
                vomsless_data = self.confdict['vomsless_vo'].split('#')
                wlcgvo = vomsless_data[0]

        # arc.conf: VO filtering
        if 'vofilter' in self.confdict:
            if not isinstance(self.confdict['vofilter'], list):
                self.confdict['vofilter'] = [self.confdict['vofilter']]
            if wlcgvo not in self.confdict['vofilter']:
                self.filtered = True
        if wlcgvo:
            xml += '<Group>{0}</Group>'.format(wlcgvo)
        else:
            return xml
        # VOMS FQAN info
        # APEL CAR has a concept of effective FQAN (WTF?):
        #   "if multiple FQAN are available, the effective one MUST be reported"
        # In ARC during auth phase all FQANs are evaluated and who knows which one is "effective"
        # In typical case proxy comes with only one FQAN, but if many, let's assume that
        # the longest one is effective (e.g. group path is longer and /Role= is also added that makes FQAN longer)
        vomsfqans = [t[1] for t in self.aar.authtokens() if t[0] == 'vomsfqan']
        if vomsfqans:
            fqan = max(vomsfqans, key=len)
            fqan_match = self._voms_fqan_re.match(fqan)
            fqangroups = []
            if fqan_match:
                xml += '<GroupAttribute urf:type="FQAN">{0}</GroupAttribute>'.format(fqan)
                fqan_dict = fqan_match.groupdict()
                if 'group' in fqan_dict and fqan_dict['group']:
                    fqangroups.append(fqan_dict['group'])
                    xml += '<GroupAttribute urf:type="vo-group">{group}</GroupAttribute>'.format(**fqan_dict)
                if 'role' in fqan_dict and fqan_dict['role']:
                    xml += '<GroupAttribute urf:type="vo-role">{role}</GroupAttribute>'.format(**fqan_dict)
        # arc.conf: vo_group attributes
        # goes contradictory to APEL "effective" FQAN concept: if we already have FQAN defined "additinal" does not
        # make sense, but to ensure bug-to-bug compatibility (this works previously) attributes will be added if defined
        if 'vo_group' in self.confdict:
            if not isinstance(self.confdict['vo_group'], list):
                self.confdict['vo_group'] = [self.confdict['vo_group']]
            for vg in self.confdict['vo_group']:
                if vg not in fqangroups:  # eliminate duplicates
                    xml += '<GroupAttribute urf:type="vo-group">{0}</GroupAttribute>'.format(vg)
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
        xml += '<SubmitHost urf:type="ARC CE">{0}</SubmitHost>'.format(self.aar.submithost())
        # Queue (mandatory)
        xml += '<Queue urf:description="execution">{0}</Queue>'.format(self.aar.get()['Queue'])
        # Site (mandatory)
        xml += '<Site urf:type="gocdb">{gocdb_name}</Site>'.format(**self.confdict)
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
        allowed_benchmark_types = ['Si2k', 'Sf2k', 'HEPSPEC']
        bechmark_type = ''
        bechmark_value = ''
        if 'benchmark_type' in aar_extra:
            # value from AAR first
            if aar_extra['benchmark_type'] in allowed_benchmark_types:
                bechmark_type = aar_extra['benchmark_type']
                bechmark_value = aar_extra['benchmark_value'] if 'benchmark_value' in aar_extra else '1.0'
            else:
                self.logger.error('Unsupported benchmark type %s in the usage record for job %s.',
                                  aar_extra['benchmark_type'], self.aar.get()['JobID'])
        if not bechmark_type:
            # config value if not in AAR
            if 'benchmark_type' in self.confdict:
                if self.confdict['benchmark_type'] in allowed_benchmark_types:
                    bechmark_type = self.confdict['benchmark_type']
                    bechmark_value = self.confdict['benchmark_value'] if 'benchmark_value' in self.confdict else '1.0'
                else:
                    self.logger.error('Unsupported benchmark type %s in the arc.conf APEL configuration.',
                                      self.confdict['benchmark_type'])
        if not bechmark_type:
            # just put defaults if still nothing
            bechmark_type = 'Si2k'
            bechmark_value = '1.0'
        xml += '<ServiceLevel urf:type="{0}">{1}</ServiceLevel>'.format(bechmark_type, bechmark_value)
        return xml

    def __create_xml(self):
        """Create CAR XML concatenating parts of the information"""
        xml = self.__record_identity()
        xml += self.__job_identity()
        xml += self.__user_identity()
        xml += self.__job_data()
        self.xml += self.__xml_templates['ur-base'].format(**{'jobrecord': xml})
