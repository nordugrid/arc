from __future__ import print_function
from __future__ import absolute_import

import sys
import os
import shutil
import isodate  # extra dependency on python-isodate package
import xml.etree.ElementTree as ElementTree
import logging
import errno
from .AccountingDBSQLite import AccountingDBSQLite


class JuraArchive(object):
    """Class for managing Jura Archive files and database"""
    # define type of records in the code to avoid unnecessary db lookups or joins for 2 values
    __db_records_type_map = {
        'sgas': 1,
        'apel': 2
    }

    def __init__(self, archive_dir, db_dir=None):
        self.archive_dir = archive_dir.rstrip('/') + '/'
        if db_dir is None:
            self.db_dir = self.archive_dir
        else:
            self.db_dir = db_dir.rstrip('/') + '/'
        self.logger = logging.getLogger('ARC.JuraArchive.Manager')
        self.db_file = self.db_dir + 'accounting.db'
        self.db = None  # type: AccountingDBSQLite

    def db_exists(self):
        """Check if accounting database was initialized"""
        return os.path.exists(self.db_file)

    def db_connection_init(self):
        """Initialize database connection"""
        if self.db is None:
            self.db = AccountingDBSQLite(self.db_file)

    def __del__(self):
        if self.db is not None:
            self.db.close()

    @staticmethod
    def __xml_to_dict(t):
        """Helper to transform necessary usagerecord XML data to Python dictionary"""
        childs = list(t)
        if childs:
            d = {t.tag: {}}
            for cd in map(JuraArchive.__xml_to_dict, list(t)):
                for k, v in cd.items():
                    if k in d[t.tag]:
                        d[t.tag][k].append(v)
                    else:
                        d[t.tag][k] = [v]
        else:
            d = {t.tag: t.text}
            # add extra tags with attribute values in name
            for a, av in t.attrib.items():
                a_tag = t.tag + '_' + a.split('}')[-1] + '_' + av
                d[a_tag] = t.text
        return d

    @staticmethod
    def __usagerecord_to_dict(xml_str):
        """Process usagerecord XML"""
        xml = ElementTree.fromstring(xml_str)
        return JuraArchive.__xml_to_dict(xml)

    @staticmethod
    def _timedelta_to_seconds(td):
        """Return duration in seconds"""
        return td.seconds + td.days * 24 * 3600

    @staticmethod
    def __parse_ur_common(ardict):
        """Extract common info from accounting record XML"""
        # extract common info
        arinfo = {
            'JobID': ardict['JobIdentity'][0]['GlobalJobId'][0],
            'JobName': ardict['JobName'][0],
            'Owner': 'N/A',
            'OwnerVO': 'N/A',
            'StartTime': isodate.parse_datetime(ardict['StartTime'][0]).replace(tzinfo=None),
            'EndTime': isodate.parse_datetime(ardict['EndTime'][0]).replace(tzinfo=None),
            'WallTime': 0,
            'CpuTime': 0,
            'Processors': 0,
        }
        # extract optional common info (possibly missing)
        if 'Processors' in ardict:
            arinfo['Processors'] = ardict['Processors'][0]
        if 'WallDuration' in ardict:
            arinfo['WallTime'] = JuraArchive._timedelta_to_seconds(isodate.parse_duration(ardict['WallDuration'][0]))
        if 'CpuDuration_usageType_all' in ardict:
            arinfo['CpuTime'] = JuraArchive._timedelta_to_seconds(
                isodate.parse_duration(ardict['CpuDuration_usageType_all'][0])
            )
        elif 'CpuDuration' in ardict:
            for ct in ardict['CpuDuration']:
                arinfo['CpuTime'] += JuraArchive._timedelta_to_seconds(isodate.parse_duration(ct))
        return arinfo

    def __record_date_path(self, rid, rtime):
        """Construct date-defined path for record file"""
        if rid.startswith('usagerecordCAR.'):
            rid = rid[15:] + '.CAR'
        else:
            rid = rid[12:] + '.UR'
        rpath = '{0}{1}/{2}/'.format(self.db_dir, rtime.strftime('%Y-%m/%d'), rid[:2])
        return rpath, rid[2:]

    def record2subdir(self, record):
        """Move accounting record to date-defined subdir"""
        (rpath, rname) = self.__record_date_path(record['RecordId'], record['EndTime'])
        self.logger.debug('Moving %s record to %s%s', record['RecordId'], rpath, rname)
        # create directory
        try:
            os.makedirs(rpath)
        except OSError as e:
            if e.errno != errno.EEXIST:
                self.logger.error('Failed to create per-date archive directory %s. Error: %s', rpath, str(e))
                sys.exit(1)
        # move the records file to per-date location
        try:
            shutil.move(self.archive_dir + record['RecordId'], rpath + rname)
        except OSError as e:
            self.logger.error('Failed to move %s record to per-date archive directory %s. Error: %s',
                              record['RecordId'], rpath, str(e))
            sys.exit(1)

    def records2db(self, records):
        """Add accounting record to database"""
        self.db_connection_init()
        # there are not so many VOs and Owners, just prefetch the data from database
        owners = self.db.get_owners()
        vos = self.db.get_vos()
        # process records one by one
        for ar in records:
            # find and substitute Owner ID
            if ar['Owner'] not in owners:
                ownerid = self.db.add_owner(ar['Owner'])
                if ownerid is None:
                    self.logger.debug('Skipping the "%s" record. Error: cannot add job owner record to database',
                                      ar['RecordId'])
                    continue
                else:
                    owners[ar['Owner']] = ownerid
            # replace owner value by ID
            ar['Owner'] = owners[ar['Owner']]
            # find and substitute VO ID
            if ar['OwnerVO'] not in vos:
                void = self.db.add_vo(ar['OwnerVO'])
                if void is None:
                    self.logger.debug('Skipping the "%s" record. Error: cannot add job VO record to database',
                                      ar['RecordId'])
                    continue
                else:
                    vos[ar['OwnerVO']] = void
            # replace VO value by ID
            ar['OwnerVO'] = vos[ar['OwnerVO']]
            # insert the usage records to database
            self.logger.debug('Adding "%s" record to accounting database.', ar['RecordId'])
            if self.db.add_usagerecord(ar):
                self.record2subdir(ar)

    def process_records(self, batch_size=100):
        """Walk through Jura archive, parse the records, put the information info database and sort files"""
        __ns_sgas_vo = '{http://www.sgas.se/namespaces/2009/05/ur/vo}VO'
        __ns_sgas_voname = '{http://www.sgas.se/namespaces/2009/05/ur/vo}Name'
        records = []
        for ar in os.listdir(self.archive_dir):
            # only parse accounting record files
            if not ar.startswith('usagerecord'):
                continue
            # read data
            try:
                with open(self.archive_dir + ar) as ar_f:
                    record_xml = ar_f.read()
            except OSError as e:
                self.logger.error('Failed to read archived usagerecord file %s. Error: %s', ar, str(e))
                continue
            # parse data (APEL CAR)
            if ar.startswith('usagerecordCAR.'):
                self.logger.debug('Processing the %s archived accounting record (APEL CAR)', ar)
                # extract info (common)
                ardict = self.__usagerecord_to_dict(record_xml)['UsageRecord']
                # extract info (APEL-specific)
                try:
                    arinfo = self.__parse_ur_common(ardict)
                    arinfo['RecordId'] = ar
                    arinfo['RecordType'] = self.__db_records_type_map['apel']
                    if 'UserIdentity' in ardict:
                        if 'GlobalUserName' in ardict['UserIdentity'][0]:
                            arinfo['Owner'] = ardict['UserIdentity'][0]['GlobalUserName'][0]
                        if 'Group' in ardict['UserIdentity'][0]:
                            arinfo['OwnerVO'] = ardict['UserIdentity'][0]['Group'][0]
                except KeyError as err:
                    self.logger.error('Malformed APEL record %s found in accounting archive. Cannot find %s key.',
                                      ar, str(err))
                except IndexError as err:
                    self.logger.error('Malformed APEL record %s found in accounting archive. Error: %s.',
                                      ar, str(err))
                else:
                    records.append(arinfo)
            # parse data (SGAS UR)
            elif ar.startswith('usagerecord.'):
                # SGAS accounting records
                self.logger.debug('Processing the %s archived accounting record (SGAS UR)', ar)
                ardict = self.__usagerecord_to_dict(record_xml)['JobUsageRecord']
                # extract info
                try:
                    arinfo = self.__parse_ur_common(ardict)
                    arinfo['RecordId'] = ar
                    arinfo['RecordType'] = self.__db_records_type_map['sgas']
                    if 'UserIdentity' in ardict:
                        if 'GlobalUserName' in ardict['UserIdentity'][0]:
                            arinfo['Owner'] = ardict['UserIdentity'][0]['GlobalUserName'][0]
                        if __ns_sgas_vo in ardict['UserIdentity'][0]:
                            arinfo['OwnerVO'] = ardict['UserIdentity'][0][__ns_sgas_vo][0][__ns_sgas_voname][0]
                except KeyError as err:
                    self.logger.error('Malformed SGAS record %s found in accounting archive. Cannot find %s key.',
                                      ar, str(err))
                except IndexError as err:
                    self.logger.error('Malformed SGAS record %s found in accounting archive. Error: %s.',
                                      ar, str(err))
                else:
                    records.append(arinfo)
            # process if already a batch size
            if len(arinfo) >= batch_size:
                self.records2db(records)
                records = []
                continue
        # process records
        self.records2db(records)

    def _filterwrap(func):
        """Decorator to parse and apply database filters and perform infornation query"""
        def wrap(self, filters=None):
            self.db_connection_init()
            if filters:
                if 'type' in filters:
                    self.db.filter_type(self.__db_records_type_map[filters['type']])
                if 'vos' in filters:
                    self.db.filter_vos(filters['vos'])
                if 'owners' in filters:
                    self.db.filter_owners(filters['owners'])
                if 'startfrom' in filters:
                    self.db.filter_startfrom(filters['startfrom'])
                if 'endtill' in filters:
                    self.db.filter_endtill(filters['endtill'])
            result = func(self, filters)
            self.db.filters_clear()
            return result
        return wrap

    @_filterwrap
    def get_records_count(self, filters=None):
        return self.db.get_records_count()

    @_filterwrap
    def get_records_walltime(self, filters=None):
        return self.db.get_records_walltime()

    @_filterwrap
    def get_records_cputime(self, filters=None):
        return self.db.get_records_cputime()

    @_filterwrap
    def get_records_owners(self, filters=None):
        return self.db.get_records_owners()

    @_filterwrap
    def get_records_vos(self, filters=None):
        return self.db.get_records_vos()

    @_filterwrap
    def get_records_path_data(self, filters=None):
        return self.db.get_records_path_data()

    @_filterwrap
    def get_records_dates(self, filters=None):
        return self.db.get_records_dates()

    def get_all_vos(self, startswith):
        return self.db.get_vos(startswith)

    def get_all_owners(self, startswith):
        return self.db.get_owners(startswith)

    def export_records(self, subdir='republish', filters=None):
        """Export all usage record files that match filtering criteria to the common directory"""
        export_dir = self.db_dir + subdir.rstrip('/') + '/'
        self.logger.info('Exporting accounting records to %s', export_dir)
        os.makedirs(export_dir)
        self.db_connection_init()
        for (rid, rtime) in self.get_records_path_data(filters):
            ur_path = self.__record_date_path(rid, rtime)
            if os.path.exists(ur_path):
                shutil.copy(ur_path, export_dir + rid)
            else:
                self.logger.error('Usage records in %s no longer exists. Republishing is not possible.', ur_path)
        return export_dir

    def export_remove(self, subdir='republish'):
        """Remove exported directory"""
        export_dir = self.db_dir + subdir.rstrip('/') + '/'
        self.logger.info('Cleaning exported records in %s', export_dir)
        shutil.rmtree(export_dir)
