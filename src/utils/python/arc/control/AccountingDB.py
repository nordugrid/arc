import sys
import os
import logging
import time
import datetime
import sqlite3

from .ControlCommon import get_human_readable_size


class SQLFilter(object):
    """Helper object to aggregate SQL where statements"""
    def __init__(self):
        self.sql = ''
        self.params = ()
        self.empty_result = False

    def add(self, sql, params=None):
        if self.empty_result:
            return
        self.sql += sql
        self.sql += ' '
        if params is not None:
            self.params += tuple(params)

    def clean(self):
        self.sql = ''
        self.params = ()

    def noresult(self):
        self.empty_result = True

    def getsql(self):
        return self.sql

    def getparams(self):
        return self.params

    def isresult(self):
        return not self.empty_result


class AccountingDB(object):
    """A-REX Accounting Records database interface for python tools"""
    def __init__(self, db_file):
        """Init connection in constructor"""
        self.logger = logging.getLogger('ARC.AccountingDB')
        self.con = None
        # don't try to initialize database if not exists
        if not os.path.exists(db_file):
            self.logger.error('Accounting database file is not exists at %s', db_file)
            sys.exit(1)
        # try to make a connection
        try:
            self.con = sqlite3.connect(db_file)
        except sqlite3.Error as e:
            self.logger.error('Failed to initialize SQLite connection to %s. Error %s', db_file, str(e))
            sys.exit(1)

        if self.con is not None:
            self.logger.debug('Connection to accounting database (%s) has been established successfully', db_file)

        # data structures for memory fetching
        self.queues = {}
        self.users = {}
        self.wlcgvos = {}
        self.statuses = {}
        self.endpoints = {}
        # select aggregate filtering
        self.sqlfilter = SQLFilter()
        # publishing db
        self.attached_publishing_db = False

    def close(self):
        """Terminate database connection"""
        if self.con is not None:
            self.logger.debug('Closing connection to accounting database')
            self.con.close()
        self.con = None

    def __del__(self):
        self.close()

    # general value fetcher
    def __get_value(self, sql, params=(), errstr='value'):
        """General helper to get the one value from database"""
        try:
            for row in self.con.execute(sql, params):
                return row[0]
        except sqlite3.Error as e:
            if params:
                errstr = errstr.format(*params)
            self.logger.debug('Failed to get %s from the accounting database. Error: %s', errstr, str(e))
        return None

    # general list fetcher
    def __get_list(self, sql, params=(), errstr='values list'):
        """General helper to get list of values from the database"""
        values = []
        try:
            for row in self.con.execute(sql, params):
                values.append(row[0])
        except sqlite3.Error as e:
            if params:
                errstr = errstr.format(*params)
            self.logger.debug('Failed to get %s from the accounting database. Error: %s', errstr, str(e))
        return values

    # helpers to fetch accounting DB normalization databases to internal class structures
    def __fetch_idname_table(self, table):
        """General helper to fetch (id, name) tables content as a dict"""
        data = {
            'byid': {},
            'byname': {}
        }
        try:
            res = self.con.execute('SELECT ID, Name FROM {0}'.format(table))
            for row in res:
                data['byid'][row[0]] = row[1]
                data['byname'][row[1]] = row[0]
        except sqlite3.Error as e:
            self.logger.error('Failed to get %s table data from accounting database. Error: %s', table, str(e))
        return data

    def __fetch_queues(self, force=False):
        if not self.queues or force:
            self.queues = self.__fetch_idname_table('Queues')

    def __fetch_users(self, force=False):
        if not self.users or force:
            self.users = self.__fetch_idname_table('Users')

    def __fetch_wlcgvos(self, force=False):
        if not self.wlcgvos or force:
            self.wlcgvos = self.__fetch_idname_table('WLCGVOs')

    def __fetch_statuses(self, force=False):
        if not self.statuses or force:
            self.statuses = self.__fetch_idname_table('Status')

    def __fetch_endpoints(self, force=False):
        if not self.endpoints or force:
            self.endpoints = {
                'byid': {},
                'byname': {},
                'bytype': {}
            }
            try:
                res = self.con.execute('SELECT ID, Interface, URL FROM Endpoints')
                for row in res:
                    self.endpoints['byid'][row[0]] = (row[1], row[2])
                    self.endpoints['byname']['{0}:{1}'.format(row[1], row[2])] = row[0]
                    if row[1] not in self.endpoints['bytype']:
                        self.endpoints['bytype'][row[1]] = []
                    self.endpoints['bytype'][row[1]].append(row[0])
            except sqlite3.Error as e:
                self.logger.error('Failed to get Endpoints table data from accounting database. Error: %s', str(e))

    # retrieve list of names used for filtering stats
    def get_queues(self):
        self.__fetch_queues()
        return self.queues['byname'].keys()

    def get_users(self):
        self.__fetch_users()
        return self.users['byname'].keys()

    def get_wlcgvos(self):
        self.__fetch_wlcgvos()
        return self.wlcgvos['byname'].keys()

    def get_statuses(self):
        self.__fetch_statuses()
        return self.statuses['byname'].keys()

    def get_endpoint_types(self):
        self.__fetch_endpoints()
        return self.endpoints['bytype'].keys()

    def get_joblist(self, prefix=''):
        sql = 'SELECT JobID FROM AAR'
        params = ()
        errstr = 'Job IDs'
        if prefix:
            sql += ' WHERE JobId LIKE ?'
            params += (prefix + '%', )
            errstr = 'Job IDs starting from {0}'
        return self.__get_list(sql, params, errstr=errstr)

    # construct stats query filters
    def __filter_nameid(self, names, attrdict, attrname, dbfield):
        if not names:
            return
        filter_ids = []
        for n in names:
            if n not in attrdict['byname']:
                self.logger.error('There are no records matching %s %s in the database.', n, attrname)
            else:
                filter_ids.append(attrdict['byname'][n])
        if not filter_ids:
            self.sqlfilter.noresult()
        else:
            self.sqlfilter.add('AND {0} IN ({1})'.format(dbfield, ','.join(['?'] * len(filter_ids))),
                               tuple(filter_ids))

    def filter_queues(self, queues):
        """Add submission queue filtering to the select queries"""
        self.__fetch_queues()
        self.__filter_nameid(queues, self.queues, 'queues', 'QueueID')

    def filter_users(self, users):
        """Add user subject filtering to the select queries"""
        self.__fetch_users()
        self.__filter_nameid(users, self.users, 'users', 'UserID')

    def filter_wlcgvos(self, wlcgvos):
        """Add WLCG VOs filtering to the select queries"""
        self.__fetch_wlcgvos()
        self.__filter_nameid(wlcgvos, self.wlcgvos, 'WLCG VO', 'VOID')

    def filter_statuses(self, statuses):
        """Add job status filtering to the select queries"""
        self.__fetch_statuses()
        self.__filter_nameid(statuses, self.statuses, 'statuses', 'StatusID')

    def filter_endpoint_type(self, etypes):
        """Add endpoint type filtering to the select queries"""
        if not etypes:
            return
        self.__fetch_endpoints()
        filter_ids = []
        for e in etypes:
            if e not in self.endpoints['bytype']:
                self.logger.error('There are no records with %s endpoint type in the database.', e)
            else:
                filter_ids.extend(self.endpoints['bytype'][e])
        if not filter_ids:
            self.sqlfilter.noresult()
        else:
            self.sqlfilter.add('AND EndpointID IN ({0})'.format(','.join(['?'] * len(filter_ids))),
                               tuple(filter_ids))

    @staticmethod
    def unixtimestamp(t):
        """Return unix timestamp representation of date"""
        if isinstance(t, (datetime.datetime, datetime.date)):
            return time.mktime(t.timetuple())  # works in Python 2.6
        return t

    def filter_startfrom(self, stime):
        """Add job start time filtering to the select queries"""
        self.sqlfilter.add('AND SubmitTime > ?', (self.unixtimestamp(stime),))

    def filter_endtill(self, etime):
        """Add job end time filtering (end before) to the select queries"""
        self.sqlfilter.add('AND EndTime <= ?', (self.unixtimestamp(etime),))

    def filter_endfrom(self, etime):
        """Add job end time filtering (end after) to the select queries"""
        self.sqlfilter.add('AND EndTime > ?', (self.unixtimestamp(etime),))

    def filter_jobids(self, jobids):
        """Add jobid filtering to the select queries"""
        if not jobids:
            return
        if len(jobids) == 1:
            self.sqlfilter.add('AND JobID = ?', tuple(jobids))
        else:
            self.sqlfilter.add('AND JobID IN ({0})'.format(','.join(['?'] * len(jobids))),
                               tuple(jobids))

    def filter_extra_attributes(self, fdict):
        """Filter additional job information for custom queries"""
        # predefined filters
        filters = {
            'vomsfqan': "AND RecordID IN ( SELECT RecordID FROM AuthTokenAttributes "
                        "WHERE AttrKey IN ('vomsfqan', 'mainfqan') AND AttrValue IN ({0}))",
            'rte': "AND RecordID IN ( SELECT RecordID FROM RunTimeEnvironments WHERE RTEName IN ({0}) )",
            'dtrurl': "AND RecordID IN ( SELECT RecordID FROM DataTransfers WHERE URL IN ({0}) )"
        }
        for f in fdict.keys():
            values = fdict[f][:]
            if f in filters:
                sql = filters[f]
                self.sqlfilter.add(sql.format(','.join(['?'] * len(values))), tuple(values))
            else:
                # any job extra info (e.g. jobname, project, etc)
                sql = "AND RecordID IN ( SELECT RecordID FROM JobExtraInfo " \
                      "WHERE InfoKey = ? AND InfoValue IN ({0}))"
                sql = sql.format(','.join(['?'] * len(values)))
                values.insert(0, f)
                self.sqlfilter.add(sql, tuple(values))

    def filters_clean(self):
        """Remove all defined SQL query filters"""
        self.sqlfilter.clean()

    def __filtered_query(self, sql, params=(), errorstr=''):
        """Add defined filters to SQL query and execute it returning the results iterator"""
        if not self.sqlfilter.isresult():
            return []
        if '<FILTERS>' in sql:
            # substitute filters to <FILTERS> placeholder if defined
            sql = sql.replace('<FILTERS>', self.sqlfilter.getsql())
        else:
            # add to the end of query otherwise
            if 'WHERE' not in sql:
                sql += ' WHERE 1=1'  # verified that this does not affect performance
            sql += ' ' + self.sqlfilter.getsql()
        params += self.sqlfilter.getparams()
        try:
            res = self.con.execute(sql, params)
            return res
        except sqlite3.Error as e:
            params += (str(e),)
            self.logger.debug('Failed to execute query: {0}. Error: %s'.format(sql.replace('?', '%s')), *params)
            if errorstr:
                self.logger.error(errorstr + ' Something goes wrong during SQL query. '
                                             'Use DEBUG loglevel to troubleshoot.')
            return []

    #
    # Testing 10M jobs SQLite database from legacy jura archive conversion (4 year of records from Andrej) shows that:
    #   a) using SQL functions works a bit faster that python post-processing
    #   b) calculating several things at once in SQlite does not affect performance, so it is worth to do it in one shot
    #
    # ALSO NOTE: in where clause filters order is very important!
    #   Filtering time range first (using less/greater than comparision on column index) and value match filters later
    #   MUCH faster compared to opposite order of filters.
    #

    def get_stats(self):
        """Return jobs statistics counters for records that match applied filters"""
        stats = {
            'count': 0, 'walltime': 0, 'cpuusertime': 0, 'cpukerneltime': 0,
            'stagein': 0, 'stageout': 0, 'rangestart': 0, 'rangeend': 0
        }
        for res in self.__filtered_query('SELECT COUNT(RecordID), SUM(UsedWalltime), SUM(UsedCPUUserTime),'
                                         'SUM(UsedCPUKernelTime), SUM(StageInVolume), SUM(StageOutVolume),'
                                         'MIN(SubmitTime), MAX(EndTime) FROM AAR',
                                         errorstr='Failed to get accounting statistics'):
            if res[0] != 0:
                stats = {
                    'count': res[0],
                    'walltime': res[1],
                    'cpuusertime': res[2],
                    'cpukerneltime': res[3],
                    'stagein': res[4],
                    'stageout': res[5],
                    'rangestart': res[6],
                    'rangeend': res[7]
                }
        return stats

    def get_job_owners(self):
        """Return list of job owners distinguished names that match applied filters"""
        userids = []
        for res in self.__filtered_query('SELECT DISTINCT UserID FROM AAR',
                                         errorstr='Failed to get accounted job owners'):
            userids.append(res[0])
        if userids:
            self.__fetch_users()
            return [self.users['byid'][u] for u in userids]
        return []

    def get_job_wlcgvos(self):
        """Return list of WLCG VOs jobs matching applied filters"""
        voids = []
        for res in self.__filtered_query('SELECT DISTINCT VOID FROM AAR',
                                         errorstr='Failed to get accounted WLCG VOs for jobs'):
            voids.append(res[0])
        if voids:
            self.__fetch_wlcgvos()
            return [self.wlcgvos['byid'][v] for v in voids]
        return []

    def get_job_ids(self):
        """Return list of JobIDs matching applied filters"""
        jobids = []
        for res in self.__filtered_query('SELECT JobID FROM AAR',
                                         errorstr='Failed to get JobIDs'):
            jobids.append(res[0])
        return jobids

    # Querying AARs and their info
    def __fetch_authtokenattrs(self):
        """Return dict that holds list of auth token attributes tuples for every record id"""
        result = {}
        sql = 'SELECT RecordID, AttrKey, AttrValue FROM AuthTokenAttributes ' \
              'WHERE RecordID IN (SELECT RecordID FROM AAR WHERE 1=1 <FILTERS>)'
        for row in self.__filtered_query(sql, errorstr='Failed to get AuthTokenAttributes for AAR(s) from database'):
            if row[0] not in result:
                result[row[0]] = []
            result[row[0]].append((row[1], row[2]))
        return result

    def __fetch_jobevents(self):
        """Return list ordered job event tuples"""
        result = {}
        sql = 'SELECT RecordID, EventKey, EventTime FROM JobEvents ' \
              'WHERE RecordID IN (SELECT RecordID FROM AAR WHERE 1=1 <FILTERS>) ORDER BY EventTime ASC'
        for row in self.__filtered_query(sql, errorstr='Failed to get JobEvents for AAR(s) from database'):
            if row[0] not in result:
                result[row[0]] = []
            result[row[0]].append((row[1], row[2]))
        return result

    def __fetch_rtes(self):
        """Return list of job RTEs"""
        result = {}
        sql = 'SELECT RecordID, RTEName FROM RunTimeEnvironments ' \
              'WHERE RecordID IN (SELECT RecordID FROM AAR WHERE 1=1 <FILTERS>)'
        for row in self.__filtered_query(sql, errorstr='Failed to get RTEs for AAR(s) from database'):
            if row[0] not in result:
                result[row[0]] = []
            result[row[0]].append(row[1])
        return result

    def __fetch_datatransfers(self):
        """Return list of dicts representing individual job datatransfers"""
        result = {}
        sql = 'SELECT RecordID, URL, FileSize, TransferStart, TransferEnd, TransferType FROM DataTransfers ' \
              'WHERE RecordID IN (SELECT RecordID FROM AAR WHERE 1=1 <FILTERS>)'
        for row in self.__filtered_query(sql, errorstr='Failed to get DataTransfers for AAR(s) from database'):
            if row[0] not in result:
                result[row[0]] = []
            # in accordance to C++ enum values
            ttype = 'input'
            if row[5] == 11:
                ttype = 'cache_input'
            elif row[5] == 20:
                ttype = 'output'
            result[row[0]].append({
                'url': row[1],
                'size': row[2],
                'timestart': datetime.datetime.fromtimestamp(row[3]),
                'timeend': datetime.datetime.fromtimestamp(row[4]),
                'type':  ttype
            })
        return result

    def __fetch_extrainfo(self):
        """Return dict of extra job info"""
        result = {}
        sql = 'SELECT RecordID, InfoKey, InfoValue FROM JobExtraInfo ' \
              'WHERE RecordID IN (SELECT RecordID FROM AAR WHERE 1=1 <FILTERS>)'
        for row in self.__filtered_query(sql, errorstr='Failed to get DataTransfers for AAR(s) from database'):
            if row[0] not in result:
                result[row[0]] = {}
            result[row[0]][row[1]] = row[2]
        return result

    def get_aars(self, resolve_ids=False):
        """Return list of AARs corresponding to filtered query"""
        aars = []
        for res in self.__filtered_query('SELECT * FROM AAR', errorstr='Failed to get AAR(s) from database'):
            aar = AAR()
            aar.fromDB(res)
            aars.append(aar)
        if resolve_ids:
            self.__fetch_statuses()
            self.__fetch_users()
            self.__fetch_wlcgvos()
            self.__fetch_queues()
            self.__fetch_endpoints()
            for a in aars:
                a.aar['Status'] = self.statuses['byid'][a.aar['StatusID']]
                a.aar['UserSN'] = self.users['byid'][a.aar['UserID']]
                a.aar['WLCGVO'] = self.wlcgvos['byid'][a.aar['VOID']]
                a.aar['Queue'] = self.queues['byid'][a.aar['QueueID']]
                endpoint = self.endpoints['byid'][a.aar['EndpointID']]
                a.aar['Interface'] = endpoint[0]
                a.aar['EndpointURL'] = endpoint[1]
        return aars

    def enrich_aars(self, aars, authtokens=False, events=False, rtes=False, dtrs=False, extra=False):
        """Add info from extra tables to the list of AAR objects.
        This method assumes that the optional info will be queried just after basic info about AARs (when needed)
        The same query filtering object should be used! Do not clear filters between get_aars and enrich_aars"""
        recordids = [a.recordid() for a in aars]
        auth_data = None
        events_data = None
        rtes_data = None
        dtrs_data = None
        extra_data = None
        if authtokens:
            auth_data = self.__fetch_authtokenattrs()
        if events:
            events_data = self.__fetch_jobevents()
        if rtes:
            rtes_data = self.__fetch_rtes()
        if dtrs:
            dtrs_data = self.__fetch_datatransfers()
        if extra:
            extra_data = self.__fetch_extrainfo()
        for a in aars:
            rid = a.recordid()
            if auth_data is not None and rid in auth_data:
                a.aar['AuthTokenAttributes'] = auth_data[rid]
            if events_data is not None and rid in events_data:
                a.aar['JobEvents'] = events_data[rid]
            if rtes_data is not None and rid in rtes_data:
                a.aar['RunTimeEnvironments'] = rtes_data[rid]
            if dtrs_data is not None and rid in dtrs_data:
                a.aar['DataTransfers'] = dtrs_data[rid]
            if extra_data is not None and rid in extra_data:
                a.aar['JobExtraInfo'] = extra_data[rid]

    def get_apel_summaries(self):
        """Return info corresponding to APEL aggregated summary records"""
        sql = '''SELECT a.UserID, a.VOID, a.EndpointID, a.NodeCount, a.CPUCount,
              AuthTokenAttributes.AttrValue, JobExtraInfo.InfoValue,
              COUNT(a.RecordID), SUM(a.UsedWalltime), SUM(a.UsedCPUTime), MIN(a.EndTime), MAX(a.EndTime),
              strftime("%Y-%m", a.endTime, "unixepoch") AS YearMonth
              FROM ( SELECT RecordID, UserID, VOID, EndpointID, NodeCount, CPUCount, SubmitTime, EndTime,
                            UsedWalltime, UsedCPUUserTime + UsedCPUKernelTime AS UsedCPUTime FROM AAR
                     WHERE 1=1 <FILTERS>) a
              LEFT JOIN JobExtraInfo ON JobExtraInfo.RecordID = a.RecordID
                                    AND JobExtraInfo.InfoKey = "benchmark"
              LEFT JOIN AuthTokenAttributes ON AuthTokenAttributes.RecordID = a.RecordID
                                           AND AuthTokenAttributes.AttrKey = "mainfqan"
              GROUP BY YearMonth, a.UserID, a.VOID, a.EndpointID, a.NodeCount, a.CPUCount'''
        summaries = []
        self.__fetch_users()
        self.__fetch_wlcgvos()
        self.__fetch_endpoints()
        for res in self.__filtered_query(sql, errorstr='Failed to get APEL summary info from database'):
            (year, month) = res[12].split('-')
            summaries.append({
                'year': year,
                'month': month.lstrip('0'),
                'userdn': self.users['byid'][res[0]],
                'wlcgvo': self.wlcgvos['byid'][res[1]],
                'endpoint': self.endpoints['byid'][res[2]][1],
                'nodecount': res[3],
                'cpucount': res[4],
                'fqan': res[5],
                'benchmark': res[6],
                'count': res[7],
                'walltime': res[8],
                'cputime': res[9],
                'timestart': res[10],
                'timeend': res[11]
            })
        return summaries

    def get_apel_sync(self):
        """Return number of jobs per endpoint for APEL Sync messages"""
        sql = '''SELECT strftime("%Y-%m", endTime, "unixepoch") as YearMonth, COUNT(RecordID), EndpointID
              FROM AAR WHERE 1=1 <FILTERS> GROUP BY YearMonth, EndpointID'''
        syncs = []
        self.__fetch_endpoints()
        for res in self.__filtered_query(sql, errorstr='Failed to get APEL sync info from database'):
            (year, month) = res[0].split('-')
            syncs.append({
                'year': year,
                'month': month.lstrip('0'),
                'count': res[1],
                'endpoint': self.endpoints['byid'][res[2]][1]
            })
        return syncs

    def get_latest_endtime(self):
        """Return latest endtime for records matching defined filters"""
        sql = 'SELECT MAX(EndTime) FROM AAR'
        for res in self.__filtered_query(sql, errorstr='Failed to get latest endtime for records from database'):
            return res[0]
        return None

    # Publishing state recording
    def attach_publishing_db(self, dbfile):
        """Attach publishing database to current connection"""
        # already attached, nothing to do
        if self.attached_publishing_db:
            return
        # attach database
        sql = 'ATTACH DATABASE ? AS pub'
        dbinit_sql = '''CREATE TABLE IF NOT EXISTS pub.AccountingTargets (
              TargetID    TEXT UNIQUE NOT NULL,
              LastEndTime INTEGER NOT NULL,
              LastReport  INTEGER NOT NULL
            )'''
        if not os.path.exists(dbfile):
            self.logger.info('Cannot find accounting publishing database file, will going to init %s.', dbfile)
        try:
            self.con.execute(sql, (dbfile, ))
        except sqlite3.Error as e:
            self.logger.error('Failed to attach accounting publishing database at %s to the current connection. '
                              'Error: %s', dbfile, str(e))
            sys.exit(1)
        # create table if not exist
        try:
            self.con.execute(dbinit_sql)
        except sqlite3.Error as e:
            self.logger.error('Failed to initialize accounting publishing database schema. '
                              'Error: %s', str(e))
            sys.exit(1)
        else:
            self.con.commit()
        self.attached_publishing_db = True

    def set_last_published_endtime(self, target_id, endtimestamp):
        """Set latest records EndTime published to this target"""
        sql = 'INSERT OR REPLACE INTO pub.AccountingTargets(TargetID, LastEndTime, LastReport) ' \
              'VALUES(?, ?, ?)'
        publish_time = self.unixtimestamp(datetime.datetime.today())
        try:
            self.con.execute(sql, (target_id, endtimestamp, publish_time))
        except sqlite3.Error as e:
            self.logger.error('Failed to update latest published records EndTime for [%s] target. '
                              'Error: %s', target_id, str(e))
        else:
            self.con.commit()

    def get_last_published_endtime(self, target_id):
        """Get latest records EndTime published to this target"""
        sql = 'SELECT LastEndTime FROM pub.AccountingTargets WHERE TargetID = ?'
        updatetime = self.__get_value(sql, (target_id, ), errstr='Failed to get last published endtime for target [{0}]')
        # if we publish fist time to this target avoid all eternity of records publishing
        # for previous records pushing use republish functionality
        if not updatetime:
            self.logger.warn('There is no record of last published timestamp for target [%s] in the database. '
                             'Records starting from Today will be reported. '
                             'If you want to publish previous records to the new target, '
                             'please proceed with data republishing.', target_id)
            updatetime = self.unixtimestamp(datetime.date.today())
            self.set_last_published_endtime(target_id, updatetime)
        return updatetime

    def get_last_report_time(self, target_id):
        """Get time of the latest report to this target"""
        sql = 'SELECT LastReport FROM pub.AccountingTargets WHERE TargetID = ?'
        updatetime = self.__get_value(sql, (target_id, ), errstr='Failed to get last report time for target [{0}]')
        if not updatetime:
            updatetime = 0
        return updatetime


class AAR(object):
    """AAR representation in Python"""
    def __init__(self):
        # define AAR dict structure
        self.aar = {}

    def fromDB(self, res):
        self.aar = {
            'RecordID': res[0],
            'JobID': res[1],
            'LocalJobID': res[2],
            'EndpointID': res[3],
            'Interface': None,
            'EndpointURL': None,
            'QueueID': res[4],
            'Queue': None,
            'UserID': res[5],
            'UserSN': None,
            'VOID': res[6],
            'WLCGVO': None,
            'StatusID': res[7],
            'Status': None,
            'ExitCode': res[8],
            'SubmitTime': datetime.datetime.fromtimestamp(res[9]),
            'EndTime': datetime.datetime.fromtimestamp(res[10]),
            'NodeCount': res[11],
            'CPUCount': res[12],
            'UsedMemory': res[13],
            'UsedVirtMem': res[14],
            'UsedWalltime': res[15],
            'UsedCPUUserTime': res[16],
            'UsedCPUKernelTime': res[17],
            'UsedCPUTime': res[16] + res[17],
            'UsedScratch': res[18],
            'StageInVolume': res[19],
            'StageOutVolume': res[20],
            'AuthTokenAttributes': [],
            'JobEvents': [],
            'RunTimeEnvironments': [],
            'DataTransfers': [],
            'JobExtraInfo': {}
        }

    def add_human_readable(self):
        for hrattr in ['UsedMemory', 'UsedVirtMem', 'UsedScratch', 'StageInVolume', 'StageOutVolume']:
            self.aar[hrattr + 'HR'] = get_human_readable_size(self.aar[hrattr])

    # common attributes
    def recordid(self):
        return self.aar['RecordID']

    def status(self):
        return self.aar['Status']

    def wlcgvo(self):
        return self.aar['WLCGVO']

    # dedicated lists with extra info
    def events(self):
        return self.aar['JobEvents']

    def extra(self):
        return self.aar['JobExtraInfo']

    def rtes(self):
        return self.aar['RunTimeEnvironments']

    def authtokens(self):
        return self.aar['AuthTokenAttributes']

    def datatransfers(self):
        return self.aar['DataTransfers']

    # return all dict
    def get(self):
        return self.aar

    # info that requires processing
    def submithost(self):
        submithost = self.aar['EndpointURL']
        submithost = submithost.split(':')[1][2:]  # http://[example.org]:443/jobs..
        submithost = submithost.split('/')[0]  # [exmple.org]/jobs...
        return submithost

