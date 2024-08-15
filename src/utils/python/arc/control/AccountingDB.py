import sys
import os
import logging
import calendar
import datetime
import sqlite3

from .ControlCommon import get_human_readable_size

ACCOUNTING_DB_FILE = "accounting_v2.db"

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
    def __init__(self, db_file, must_exists=True):
        """Try to init connection in constructor to ensure database is accessible"""
        self.logger = logging.getLogger('ARC.AccountingDB')
        self.db_file = db_file
        self.db_version = 0
        self.con = None
        self.pub_con = None
        # don't try to initialize database if not exists
        if must_exists and not os.path.exists(db_file):
            self.logger.error('Accounting database file does not exists at %s', db_file)
            sys.exit(1)
        # try to make a connection
        self.adb_connect()
        # fetch database version
        self.adb_version()
        # ensure Write-Ahead Logging mode
        jmode = self.__get_value('PRAGMA journal_mode=WAL', errstr='SQLite journal mode')
        if jmode != 'wal':
            self.logger.error('Failed to switch SQLite database journal mode to Write-Ahead Logging (WAL). '
                              'Default "%s" mode continued to be used. '
                              'This can negatively affect A-REX jobs processing during publishing. '
                              'It is STRONGLY ADVISED to update SQLite to 3.7.0+!', jmode)
        else:
            self.logger.debug('Using SQLite Write-Ahead Logging mode that is non-blocking for read-only queries')
        #
        # NOTE: to minimize possibility of locking in any journal mode, AccountingDB object uses connection on-demand
        #       instead of keeping it open
        #
        self.adb_close()

        # data structures for in-memory fetching
        self.queues = {}
        self.users = {}
        self.wlcgvos = {}
        self.fqans = {}  # v2
        self.statuses = {}
        self.benchmarks = {}  # v2
        self.endpoints = {}

        # select aggregate filtering
        self.sqlfilter = SQLFilter()

    def adb_connect(self):
        """Initialize database connection"""
        if self.con is not None:
            self.logger.debug('Using already established SQLite connection to accounting database %s', self.db_file)
            return self.con
        try:
            self.con = sqlite3.connect(self.db_file)
        except sqlite3.Error as e:
            self.logger.error('Failed to initialize SQLite connection to %s. Error %s', self.db_file, str(e))
            sys.exit(1)
        if self.con is not None:
            self.logger.debug('Connection to accounting database (%s) has been established successfully', self.db_file)
            return self.con

    def adb_close(self):
        """Terminate database connection"""
        if self.con is not None:
            self.logger.debug('Closing connection to accounting database')
            self.con.close()
            self.con = None

    def adb_version(self):
        """Return accounting database version"""
        if not self.db_version:
            self.db_version = int(self.__get_value('SELECT KeyValue FROM DBConfig WHERE KeyName = "DBSCHEMA_VERSION"'))
            self.logger.info('Accounting database schema version for %s is %s', self.db_file, self.db_version)
        return self.db_version

    def _backup_progress(self, status, remaining, total):
        """Log database backup progress"""
        progress = ( total - remaining ) * 100 // total
        if progress >= self._backup_progress_log_threshold:
            self.logger.info('Backup progress: %s%% database pages copied...', progress)
            self._backup_progress_log_threshold += 10
        else:
            self.logger.debug('Backup progress: %s%% database pages copied...', progress)

    def backup(self, backup_adb):
        """Backup database to another database defined as AccountingDB object"""
        self.adb_connect()
        backup_con = backup_adb.adb_connect()
        try:
            self._backup_progress_log_threshold = 0
            self.con.backup(backup_con, pages=1, progress=self._backup_progress)
        except sqlite3.Error as e:
            self.logger.error('Failed to backup accounting database to %s. Error %s', backup_adb.db_file, str(e))
            sys.exit(1)
        backup_adb.adb_close()
        self.adb_close()
        del self._backup_progress_log_threshold

    def __del__(self):
        self.adb_close()
        self.publishing_db_close()

    # general value fetcher
    def __get_value(self, sql, params=(), errstr='value', pubcon=False):
        """General helper to get the one value from database"""
        db_con = self.con
        if pubcon:
            db_con = self.pub_con
        try:
            for row in db_con.execute(sql, params):
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
        self.adb_connect()
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
        self.adb_close()
        return data

    def __fetch_queues(self, force=False):
        if not self.queues or force:
            self.logger.debug('Fetching queues from accounting database')
            self.queues = self.__fetch_idname_table('Queues')

    def __fetch_users(self, force=False):
        if not self.users or force:
            self.logger.debug('Fetching users from accounting database')
            self.users = self.__fetch_idname_table('Users')

    def __fetch_wlcgvos(self, force=False):
        if not self.wlcgvos or force:
            self.logger.debug('Fetching WLCG VOs from accounting database')
            self.wlcgvos = self.__fetch_idname_table('WLCGVOs')

    def __fetch_fqans(self, force=False):
        if self.db_version < 2:
            return
        if not self.fqans or force:
            self.logger.debug('Fetching FQANs from accounting database')
            self.fqans = self.__fetch_idname_table('FQANs')

    def __fetch_statuses(self, force=False):
        if not self.statuses or force:
            self.logger.debug('Fetching available job statuses from accounting database')
            self.statuses = self.__fetch_idname_table('Status')

    def __fetch_benchmarks(self, force=False):
        if self.db_version < 2:
            return
        if not self.benchmarks or force:
            self.logger.debug('Fetching available node benchmarks from accounting database')
            self.benchmarks = self.__fetch_idname_table('Benchmarks')

    def __fetch_endpoints(self, force=False):
        if not self.endpoints or force:
            self.logger.debug('Fetching service endpoints from accounting database')
            self.adb_connect()
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
            self.adb_close()

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

    def get_fqans(self):
        if self.db_version < 2:
            return []
        self.__fetch_fqans()
        return self.fqans['byname'].keys()

    def get_statuses(self):
        self.__fetch_statuses()
        return self.statuses['byname'].keys()

    def get_benchmarks(self):
        if self.db_version < 2:
            return []
        self.__fetch_benchmarks()
        return self.benchmarks['byname'].keys()

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
        self.adb_connect()
        joblist = self.__get_list(sql, params, errstr=errstr)
        self.adb_close()
        return joblist

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

    def filter_fqans(self, fqans):
        """Add FQANs filtering to the select queries"""
        if self.db_version < 2:
            self.logger.warning('FQAN filtering is only available for v2 accounting database. Use filter-extra instead on legacy data.')
            return
        self.__fetch_fqans()
        self.__filter_nameid(fqans, self.fqans, 'FQAN', 'FQANID')

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
            return calendar.timegm(t.timetuple())  # works in Python 2.6
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
            # TODO: add OR
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
        # NOTE: this function does not close DB connection and return sqlite3.Cursor object on successful query
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
        self.adb_connect()
        try:
            res = self.con.execute(sql, params)
            return res
        except sqlite3.Error as e:
            params += (str(e),)
            self.logger.debug('Failed to execute query: {0}. Error: %s'.format(
                sql.replace('%', '%%').replace('?', '%s')), *params)
            if errorstr:
                self.logger.error(errorstr + ' Something goes wrong during SQL query. '
                                             'Use DEBUG loglevel to troubleshoot.')
            self.adb_close()
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
        self.adb_close()
        return stats

    def get_job_owners(self):
        """Return list of job owners distinguished names that match applied filters"""
        userids = []
        for res in self.__filtered_query('SELECT DISTINCT UserID FROM AAR',
                                         errorstr='Failed to get accounted job owners'):
            userids.append(res[0])
        self.adb_close()
        if userids:
            self.__fetch_users()
            return [self.users['byid'][u] for u in userids]
        return []

    def get_job_wlcgvos(self):
        """Return list of job owners WLCG VOs matching applied filters"""
        voids = []
        for res in self.__filtered_query('SELECT DISTINCT VOID FROM AAR',
                                         errorstr='Failed to get accounted WLCG VOs for jobs'):
            voids.append(res[0])
        self.adb_close()
        if voids:
            self.__fetch_wlcgvos()
            return [self.wlcgvos['byid'][v] for v in voids]
        return []

    def get_job_fqan(self):
        """Return list of job owners FQANs matching applied filters"""
        if self.db_version < 2:
            return []
        fqanids = []
        for res in self.__filtered_query('SELECT DISTINCT FQANID FROM AAR',
                                         errorstr='Failed to get accounted WLCG VOs for jobs'):
            fqanids.append(res[0])
        self.adb_close()
        if fqanids:
            self.__fetch_fqans()
            return [self.fqans['byid'][v] for v in fqanids]
        return []

    def get_job_ids(self):
        """Return list of JobIDs matching applied filters"""
        jobids = []
        for res in self.__filtered_query('SELECT JobID FROM AAR',
                                         errorstr='Failed to get JobIDs'):
            jobids.append(res[0])
        self.adb_close()
        return jobids

    # Querying AARs and their info
    def __fetch_authtokenattrs(self):
        """Return dict that holds list of auth token attributes tuples for every record id"""
        result = {}
        self.logger.debug('Fetching auth token attributes for AARs from database')
        sql = 'SELECT RecordID, AttrKey, AttrValue FROM AuthTokenAttributes ' \
              'WHERE RecordID IN (SELECT RecordID FROM AAR WHERE 1=1 <FILTERS>)'
        for row in self.__filtered_query(sql, errorstr='Failed to get AuthTokenAttributes for AAR(s) from database'):
            if row[0] not in result:
                result[row[0]] = []
            result[row[0]].append((row[1], row[2]))
        self.adb_close()
        return result

    def __fetch_jobevents(self):
        """Return list ordered job event tuples"""
        result = {}
        self.logger.debug('Fetching job events for AARs from database')
        sql = 'SELECT RecordID, EventKey, EventTime FROM JobEvents ' \
              'WHERE RecordID IN (SELECT RecordID FROM AAR WHERE 1=1 <FILTERS>) ORDER BY EventTime ASC'
        for row in self.__filtered_query(sql, errorstr='Failed to get JobEvents for AAR(s) from database'):
            if row[0] not in result:
                result[row[0]] = []
            result[row[0]].append((row[1], row[2]))
        self.adb_close()
        return result

    def __fetch_rtes(self):
        """Return list of job RTEs"""
        result = {}
        self.logger.debug('Fetching used RTEs for AARs from database')
        sql = 'SELECT RecordID, RTEName FROM RunTimeEnvironments ' \
              'WHERE RecordID IN (SELECT RecordID FROM AAR WHERE 1=1 <FILTERS>)'
        for row in self.__filtered_query(sql, errorstr='Failed to get RTEs for AAR(s) from database'):
            if row[0] not in result:
                result[row[0]] = []
            result[row[0]].append(row[1])
        self.adb_close()
        return result

    def __fetch_datatransfers(self):
        """Return list of dicts representing individual job datatransfers"""
        result = {}
        self.logger.debug('Fetching jobs datatransfer records for AARs from database')
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
                'timestart': datetime.datetime.utcfromtimestamp(row[3]),
                'timeend': datetime.datetime.utcfromtimestamp(row[4]),
                'type':  ttype
            })
        self.adb_close()
        return result

    def __fetch_extrainfo(self):
        """Return dict of extra job info"""
        self.logger.debug('Fetching extra job info for AARs from database')
        result = {}
        sql = 'SELECT RecordID, InfoKey, InfoValue FROM JobExtraInfo ' \
              'WHERE RecordID IN (SELECT RecordID FROM AAR WHERE 1=1 <FILTERS>)'
        for row in self.__filtered_query(sql, errorstr='Failed to get DataTransfers for AAR(s) from database'):
            if row[0] not in result:
                result[row[0]] = {}
            result[row[0]][row[1]] = row[2]
        self.adb_close()
        return result

    def get_aars(self, resolve_ids=False):
        """Return list of AARs corresponding to filtered query"""
        self.logger.debug('Fetching AARs main data from database')
        aars = []
        for res in self.__filtered_query('SELECT * FROM AAR', errorstr='Failed to get AAR(s) from database'):
            aar = AAR(version=self.db_version)
            aar.fromDB(res)
            aars.append(aar)
        if resolve_ids:
            self.__fetch_statuses()
            self.__fetch_users()
            self.__fetch_wlcgvos()
            self.__fetch_fqans()
            self.__fetch_queues()
            self.__fetch_endpoints()
            self.__fetch_benchmarks()
            for a in aars:
                a.aar['Status'] = self.statuses['byid'][a.aar['StatusID']]
                a.aar['UserSN'] = self.users['byid'][a.aar['UserID']]
                a.aar['WLCGVO'] = self.wlcgvos['byid'][a.aar['VOID']]
                a.aar['Queue'] = self.queues['byid'][a.aar['QueueID']]
                endpoint = self.endpoints['byid'][a.aar['EndpointID']]
                a.aar['Interface'] = endpoint[0]
                a.aar['EndpointURL'] = endpoint[1]
                if self.db_version >= 2:
                    a.aar['Benchmark'] = self.benchmarks['byid'][a.aar['BenchmarkID']]
                    a.aar['FQAN'] = self.fqans['byid'][a.aar['FQANID']]
        self.adb_close()
        return aars

    def enrich_aars(self, aars, authtokens=False, events=False, rtes=False, dtrs=False, extra=False):
        """Add info from extra tables to the list of AAR objects.
        This method assumes that the optional info will be queried just after basic info about AARs (when needed)
        The same query filtering object should be used! Do not clear filters between get_aars and enrich_aars"""
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
        """Return data for APEL aggregated summary records"""
        if self.db_version < 2:
            return self.__get_apel_summaries_v1()
        else:
            return self.__get_apel_summaries_v2()

    def __get_apel_summaries_v1(self):
        """Query data for APEL aggregated summary records from v1 database"""
        summaries = []
        # get RecordID range to limit further filtering
        record_start = None
        record_end = None
        for res in self.__filtered_query('SELECT min(RecordID), max(RecordID) FROM AAR',
                                         errorstr='Failed to get records range from database'):
            record_start = res[0]
            record_end = res[1]
        if record_start is None or record_end is None:
            self.logger.error('Database query error. Failed to proceed with APEL summary generation')
            self.adb_close()
            return summaries
        self.logger.debug('Will query extra table for the records in range [%s, %s]', record_start, record_end)
        # invoke heavy summary query with extra table pre-filtering
        sql = '''SELECT a.UserID, a.VOID, a.EndpointID, a.NodeCount, a.CPUCount,
              t.AttrValue as AuthToken, e.InfoValue as RBenchmark,
              COUNT(a.RecordID), SUM(a.UsedWalltime), SUM(a.UsedCPUTime), MIN(a.EndTime), MAX(a.EndTime),
              strftime("%Y-%m", a.endTime, "unixepoch") AS YearMonth
              FROM ( SELECT RecordID, UserID, VOID, EndpointID, NodeCount, CPUCount, SubmitTime, EndTime,
                            UsedWalltime, UsedCPUUserTime + UsedCPUKernelTime AS UsedCPUTime FROM AAR
                     WHERE 1=1 <FILTERS>) a
              LEFT JOIN ( SELECT * FROM JobExtraInfo
                          WHERE JobExtraInfo.RecordID >= {0} AND JobExtraInfo.RecordID <= {1}
                          AND JobExtraInfo.InfoKey = "benchmark" ) e ON e.RecordID = a.RecordID
              LEFT JOIN ( SELECT * FROM AuthTokenAttributes
                          WHERE AuthTokenAttributes.RecordID >= {0} AND AuthTokenAttributes.RecordID <= {1}
                          AND AuthTokenAttributes.AttrKey = "mainfqan" ) t ON t.RecordID = a.RecordID
              GROUP BY YearMonth, a.UserID, a.VOID, a.EndpointID,
                       a.NodeCount, a.CPUCount, RBenchmark'''.format(record_start, record_end)
        self.__fetch_users()
        self.__fetch_wlcgvos()
        self.__fetch_endpoints()
        self.logger.debug('Invoking query to fetch APEL summaries data from database')
        for res in self.__filtered_query(sql, errorstr='Failed to get APEL summary info from v1 database'):
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
        self.adb_close()
        return summaries

    def __get_apel_summaries_v2(self):
        """Query data for APEL aggregated summary records from v2 database"""
        summaries = []
        # UserID(0), VOID(1), EndpointID(2), NodeCount(3), CPUCount(4), FQANID(5), BenchmarkID(6),
        # COUNT(RecordID)(7), SUM(UsedWalltime)(8), SUM(UsedCPUUserTime)(9), SUM(UsedCPUKernelTime)(10),
        # MIN(EndTime)(11), MAX(EndTime)(12), YearMonth(13)
        sql = '''SELECT UserID, VOID, EndpointID, NodeCount, CPUCount, FQANID, BenchmarkID,
              COUNT(RecordID), SUM(UsedWalltime), SUM(UsedCPUUserTime), SUM(UsedCPUKernelTime),
              MIN(EndTime), MAX(EndTime), strftime("%Y-%m", endTime, "unixepoch") AS YearMonth
              FROM AAR WHERE 1=1 <FILTERS>
              GROUP BY YearMonth, UserID, VOID, EndpointID, NodeCount, CPUCount, FQANID, BenchmarkID'''
        self.__fetch_users()
        self.__fetch_wlcgvos()
        self.__fetch_fqans()
        self.__fetch_benchmarks()
        self.__fetch_endpoints()
        self.logger.debug('Invoking query to fetch APEL summaries data from database')
        for res in self.__filtered_query(sql, errorstr='Failed to get APEL summary info from database'):
            (year, month) = res[13].split('-')
            summaries.append({
                'year': year,
                'month': month.lstrip('0'),
                'userdn': self.users['byid'][res[0]],
                'wlcgvo': self.wlcgvos['byid'][res[1]],
                'endpoint': self.endpoints['byid'][res[2]][1],
                'nodecount': res[3],
                'cpucount': res[4],
                'fqan': self.fqans['byid'][res[5]],
                'benchmark': self.benchmarks['byid'][res[6]],
                'count': res[7],
                'walltime': res[8],
                'cputime': res[9] + res[10],
                'timestart': res[11],
                'timeend': res[12]
            })
        self.adb_close()
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
        self.adb_close()
        return syncs

    def get_latest_endtime(self):
        """Return latest endtime for records matching defined filters"""
        endtime = None
        sql = 'SELECT MAX(EndTime) FROM AAR'
        for res in self.__filtered_query(sql, errorstr='Failed to get latest endtime for records from database'):
            endtime = res[0]
            break
        self.adb_close()
        return endtime

    # Publishing state recording
    def publishing_db_connect(self, dbfile):
        """Establish connection to publishing state database"""
        self.logger.debug('Establishing connection to accounting publishing state database at %s', dbfile)

        if self.pub_con is not None:
            self.logger.debug('Publishing state database is already connected')
            return
        # attach database
        dbinit_sql = '''CREATE TABLE IF NOT EXISTS AccountingTargets (
              TargetID    TEXT UNIQUE NOT NULL,
              LastEndTime INTEGER NOT NULL,
              LastReport  INTEGER NOT NULL
            )'''
        if not os.path.exists(dbfile):
            self.logger.info('Cannot find accounting publishing database file, will going to init %s.', dbfile)
        try:
            self.pub_con = sqlite3.connect(dbfile)
        except sqlite3.Error as e:
            self.logger.error('Failed to connect to accounting publishing state database at %s. '
                              'Error: %s', dbfile, str(e))
            sys.exit(1)
        # create table if not exist
        try:
            self.pub_con.execute(dbinit_sql)
        except sqlite3.Error as e:
            self.logger.error('Failed to initialize accounting publishing state database schema. '
                              'Error: %s', str(e))
            sys.exit(1)
        else:
            self.pub_con.commit()
        self.logger.info('Established connection to accounting publishing state database')

    def publishing_db_close(self):
        """Terminate publishing database connection"""
        if self.pub_con is not None:
            self.logger.debug('Closing connection to accounting database')
            self.pub_con.close()
            self.pub_con = None

    def set_last_published_endtime(self, target_id, endtimestamp):
        """Set latest records EndTime published to this target"""
        sql = 'INSERT OR REPLACE INTO AccountingTargets(TargetID, LastEndTime, LastReport) ' \
              'VALUES(?, ?, ?)'
        publish_time = self.unixtimestamp(datetime.datetime.today())
        try:
            self.pub_con.execute(sql, (target_id, endtimestamp, publish_time))
        except sqlite3.Error as e:
            self.logger.error('Failed to update latest published records EndTime for [%s] target. '
                              'Error: %s', target_id, str(e))
        else:
            self.pub_con.commit()

    def get_last_published_endtime(self, target_id):
        """Get latest records EndTime published to this target"""
        sql = 'SELECT LastEndTime FROM AccountingTargets WHERE TargetID = ?'
        updatetime = self.__get_value(sql, (target_id, ), pubcon=True,
                                      errstr='Failed to get last published endtime for target [{0}]')
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
        sql = 'SELECT LastReport FROM AccountingTargets WHERE TargetID = ?'
        updatetime = self.__get_value(sql, (target_id, ), pubcon=True,
                                      errstr='Failed to get last report time for target [{0}]')
        if not updatetime:
            updatetime = 0
        return updatetime

class AAR(object):
    """AAR representation in Python"""
    def __init__(self, version=2):
        self.logger = logging.getLogger('ARC.AccountingDB.AAR')
        self.aar = {}
        self.version = 2

    def fromDB(self, res):
        if self.version == 2:
            return self.__fromDB_v2(res)
        elif self.version == 1:
            return self.__fromDB_v1(res)
        else:
            self.logger.fatal('AAR version %s is not supported.', self.version)
            sys.exit(1)

    def __fromDB_v2(self, res):
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
            'FQANID': res[7],
            'FQAN': None,
            'StatusID': res[8],
            'Status': None,
            'ExitCode': res[9],
            'BenchmarkID': res[10],
            'Benchmark': None,
            'SubmitTime': datetime.datetime.utcfromtimestamp(res[11]),
            'EndTime': datetime.datetime.utcfromtimestamp(res[12]),
            'NodeCount': res[13],
            'CPUCount': res[14],
            'UsedMemory': res[15],
            'UsedVirtMem': res[16],
            'UsedWalltime': res[17],
            'UsedCPUUserTime': res[18],
            'UsedCPUKernelTime': res[19],
            'UsedCPUTime': res[18] + res[19],
            'UsedScratch': res[20],
            'StageInVolume': res[21],
            'StageOutVolume': res[22],
            'AuthTokenAttributes': [],
            'JobEvents': [],
            'RunTimeEnvironments': [],
            'DataTransfers': [],
            'JobExtraInfo': {}
        }

    def __fromDB_v1(self, res):
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
            'SubmitTime': datetime.datetime.utcfromtimestamp(res[9]),
            'EndTime': datetime.datetime.utcfromtimestamp(res[10]),
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
        split_url = self.aar['EndpointURL'].split(':')
        if len(split_url) < 2:
            self.logger.error('Endpoint URL "%s" does not comply with URL syntax for AAR %s. Returning empty string.',
                              self.aar['EndpointURL'], self.aar['RecordID'])
            return ''
        submithost = split_url[1][2:]  # http://[example.org]:443/jobs..
        submithost = submithost.split('/')[0]  # [exmple.org]/jobs...
        return submithost

