from __future__ import print_function
from __future__ import absolute_import

from .AccountingDB import AccountingDB, AAR

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
            # create URs
            urs = [UsageRecord(aar, self.confdict[sb]) for aar in aars]
            # TODO: real publishing
            print('<?xml version="1.0" encoding="utf-8"?>')
            print(random.choice(urs).get_xml())

    def publish_apel(self, aars):
        """Publish to all SGAS targets"""
        pass

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


class UsageRecord(object):
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
        'user-local-id': '<LocalUserId>{localid}</LocalUserId>',
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
        self.xml = ''
        self.confdict = conf
        self.aar = aar  # type: AAR
        self.filtered = False
        self.__create_xml()

    # General helper methods
    def __add_aar_node(self, node, field):
        """Return XML tag for AAR attribute"""
        return '<{0}>{1}</{0}>'.format(node, self.aar.get()[field])

    def __add_extra_node(self, node, extra):
        """Return optional XML tag for AAR extra info attribute"""
        aar_extra = self.aar.extra()
        if extra in aar_extra:
            return '<{0}>{1}</{0}>'.format(node, extra)
        return ''

    # Methods to construct UR XML parts
    def __record_identity(self):
        """Construct RecordIdentity"""
        return self.__xml_templates['record-id'].format(**{
            'createtime': datetime_to_iso8601(datetime.datetime.today()),
            'recordid': 'ur-' + self.aar.submithost() + self.aar.get()['JobID']
        })

    def __job_identity(self):
        """Construct JobIdentity"""
        # GlobalJobId and LocalJobID properties are optional in UR but at least one must be present
        # we always have GlobalJobId
        # PorcessID is another optional value we don't have
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
        fqan_xml = ''
        if vomsfqans:
            fqan_re = re.compile(
                r'(?P<group>/[-\w.]+(?:/[-\w.]+)*)(?P<role>/Role=[-\w.]+)?(?P<cap>/Capability=[-\w.]+)?'
            )
            for fqan in vomsfqans:
                fqan_match = fqan_re.match(fqan)
                if fqan_match:
                    fqan_dict = fqan_match.groupdict()
                    fqan_xml += '<vo:Attribute>'
                    if 'group' in fqan_dict and fqan_dict['group']:
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
        localuser = ''
        aar_extra = self.aar.extra()
        if 'localuser' in aar_extra:
            localuser = self.__xml_templates['user-local-id'].format(**{
                'localid': aar_extra['localuser']
            })
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
        xml += self.__add_extra_node('JobName', 'jobname')
        # Status (mandatory)
        xml += self.__add_aar_node('Status', 'Status')
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
        xml += self.__add_aar_node('Queue', 'Queue')
        # ProjectName (optional)
        xml += self.__add_extra_node('ProjectName', 'projectname')
        # NodeCount, Processors
        xml += self.__add_aar_node('NodeCount', 'NodeCount')
        xml += self.__add_aar_node('Processors', 'CPUCount')
        # Network (optional, new in jura-ng)
        network = self.aar.get()['StageInVolume'] + self.aar.get()['StageOutVolume']
        xml += '<Network urf:storageUnit="B" urf:metric="total">{0}</Network>'.format(network)
        # Disk (optional, new in jura-ng)
        scratch = self.aar.get()['UsedScratch']
        if scratch:
            xml += '<Disk urf:storageUnit="B" urf:metric="total" urf:type="scratch">{0}</Disk>'.format(scratch)
        # ServiceLevel (optional, new in jura-ng)
        xml += self.__add_extra_node('ServiceLevel', 'wninstance')
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

    def get_xml(self):
        """Return UR XML"""
        return self.xml

    def is_filtered(self):
        """Indicate whether this UR should be filtered according to configuration"""
        return self.filtered
