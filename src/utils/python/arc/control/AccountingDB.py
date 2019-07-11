import sys
import os
import logging
import time
import sqlite3


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

    def get_jobids(self, prefix=''):
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

    def filter_startfrom(self, stime):
        """Add job start time filtering to the select queries"""
        unixtime = time.mktime(stime.timetuple())  # works in Python 2.6
        self.sqlfilter.add('AND SubmitTime > ?', (unixtime,))

    def filter_endtill(self, etime):
        """Add job end time filtering to the select queries"""
        unixtime = time.mktime(etime.timetuple())  # works in Python 2.6
        self.sqlfilter.add('AND EndTime < ?', (unixtime,))

    def __filtered_query(self, sql, params=(), errorstr=''):
        """Add defined filters to SQL query and execute it returning the results iterator"""
        if not self.sqlfilter.isresult():
            return []
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
        """Return list of WLCG VOs jobs matching applied filters belongs to"""
        voids = []
        for res in self.__filtered_query('SELECT DISTINCT VOID FROM AAR',
                                         errorstr='Failed to get accounted WLCG VOs for jobs'):
            voids.append(res[0])
        if voids:
            self.__fetch_wlcgvos()
            return [self.wlcgvos['byid'][v] for v in voids]
        return []
