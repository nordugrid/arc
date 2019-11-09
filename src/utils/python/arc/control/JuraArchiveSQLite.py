import sys
import logging
import datetime
import sqlite3
from arc.paths import *


class JuraArchiveSQLite(object):
    """SQLite implementation of legacy jura accounting archive database"""
    __dbinit_sql_script = ARC_DATA_DIR + '/sql-schema/legacy_jura_archivedb_schema.sql'

    def __init__(self, db_file):
        self.logger = logging.getLogger('ARC.JuraArchive.SQLiteDB')
        self.con = None
        try:
            self.con = sqlite3.connect(db_file, detect_types=sqlite3.PARSE_DECLTYPES)
        except sqlite3.Error as e:
            self.logger.error('Failed to initialize SQLite connection. Error %s', str(e))
            sys.exit(1)
        # try to get database version
        db_version = self.get_db_version()
        if db_version is None:
            try:
                self.logger.info('Initializing archive database structure.')
                with open(self.__dbinit_sql_script) as sql_f:
                    self.con.executescript(sql_f.read())
            except sqlite3.Error as e:
                self.logger.error('Failed to initialize SQLite database structure. Error %s', str(e))
                self.con.close()
                sys.exit(1)
        else:
            # In case we need to adjust database scheme, altering sql will be applied here depending on the version
            self.logger.debug('Loaded archive database version %s at %s', db_version, db_file)
        # init vars for records filtering methods
        self.filter_str = ''
        self.filter_params = ()
        self.return_empty_select = False

    def close(self):
        """Terminate database connection"""
        if self.con is not None:
            self.con.close()
        self.con = None

    def __del__(self):
        self.close()

    def _get_value(self, sql, params=(), errstr='value'):
        """General helper to get the one value from database"""
        try:
            for row in self.con.execute(sql, params):
                return row[0]
        except sqlite3.Error as e:
            if params:
                errstr = errstr.format(*params)
            self.logger.debug('Failed to get %s. Error: %s', errstr, str(e))
        return None

    def __get_kv_table(self, table, value, key, startswith=''):
        """General helper to fetch key=value tables content as a dict"""
        data = {}
        try:
            if startswith:
                res = self.con.execute(
                    "SELECT {0},{1} FROM {2} WHERE {0} LIKE ?".format(value, key, table), (startswith + '%',))
            else:
                res = self.con.execute("SELECT {0},{1} FROM {2}".format(value, key, table))
            # create owners dict
            for row in res:
                data[row[0]] = row[1]
        except sqlite3.Error as e:
            self.logger.error('Failed to get %s table data from accounting database. Error: %s', table, str(e))
        return data

    def __insert_kv_data(self, table, column, value):
        """General helper to insert key=value content to the database"""
        try:
            cursor = self.con.cursor()
            cursor.execute("INSERT INTO {0} ({1}) VALUES (?)".format(table, column), (value,))
        except sqlite3.Error as e:
            self.logger.error('Failed to insert "%s" into accounting database %s table. Error: %s',
                              value, table, str(e))
            return None
        else:
            self.con.commit()
            return cursor.lastrowid

    def get_db_version(self):
        """Get accounting databse schema version"""
        return self._get_value("SELECT VarValue FROM Vars WHERE VarName = 'VERSION'", errstr='archive database version')

    def get_owners(self, startswith=''):
        """Get job owners dictionary"""
        return self.__get_kv_table('Owners', 'OwnerDN', 'OwnerID', startswith)

    def get_vos(self, startswith=''):
        """Get job VOs dictionary"""
        return self.__get_kv_table('VOs', 'VOName', 'VOId', startswith)

    def get_ownerid(self, dn):
        """Get job owner ID by DN"""
        return self._get_value("SELECT OwnerId FROM Owners WHERE OwnerDN = ?", (dn,), errstr='user {0} database ID')

    def get_void(self, voname):
        """Get job VO ID by VO name"""
        return self._get_value("SELECT VOId FROM VOs WHERE VOName = ?", (voname,), errstr='VO {0} database ID')

    def add_vo(self, name):
        """Add new job VO record"""
        return self.__insert_kv_data('VOs', 'VOName', name)

    def add_owner(self, dn):
        """Add new job owner record"""
        return self.__insert_kv_data('Owners', 'OwnerDN', dn)

    def add_usagerecord(self, ur):
        """Add new accounting record"""
        try:
            cur = self.con.cursor()
            cur.execute("INSERT OR IGNORE INTO UsageRecords VALUES (?,?,?,?,?,?,?,?,?,?,?)",
                        (ur['RecordId'], ur['RecordType'], ur['StartTime'], ur['EndTime'],
                         ur['WallTime'], ur['CpuTime'], ur['Processors'],
                         ur['JobName'], ur['JobID'], ur['Owner'], ur['OwnerVO']))
        except sqlite3.Error as e:
            self.logger.error('Failed to insert "%s" record into accounting database. Error: %s',
                              ur['RecordId'], str(e))
            return False
        else:
            self.con.commit()
            if not cur.rowcount:
                self.logger.warning('Record "%s" is already exists in accounting database (insert ignored).',
                                    ur['RecordId'])
            return True

    def filters_clear(self):
        """Clear all filters"""
        self.filter_str = ''
        self.filter_params = ()

    def filter_type(self, typeid):
        """Add record type filtering to the select queries"""
        self.filter_str += 'AND RecordType = ? '
        self.filter_params += (typeid,)

    def filter_vos(self, vonames):
        """Add VO filtering to the select queries"""
        vos = self.get_vos()
        voids = []
        for vo in vonames:
            if vo not in vos:
                self.logger.error('There are no records with %s VO in the database.', vo)
            else:
                voids.append(vos[vo])
        if not voids:
            self.return_empty_select = True
        else:
            self.filter_str += 'AND VOId IN({0}) '.format(','.join(['?'] * len(voids)))
            self.filter_params += tuple(voids)

    def filter_owners(self, dns):
        """Add job owner DN filtering to the select queries"""
        owners = self.get_owners()
        ownerids = []
        for dn in dns:
            if dn not in owners:
                self.logger.error('There are no records with %s job owner in the database.', dn)
            else:
                ownerids.append(owners[dn])
        if not ownerids:
            self.return_empty_select = True
        else:
            self.filter_str += 'AND OwnerId IN({0}) '.format(','.join(['?'] * len(ownerids)))
            self.filter_params += tuple(ownerids)

    def filter_startfrom(self, stime):
        """Add job start time filtering to the select queries"""
        self.filter_str += 'AND StartTime > ? '
        self.filter_params += (stime,)

    def filter_endtill(self, etime):
        """Add job end time filtering to the select queries"""
        self.filter_str += 'AND EndTime < ? '
        self.filter_params += (etime,)

    def _filtered_query(self, sql, params=(), errorstr=''):
        """Add defined filters to SQL query and execute it returning the results iterator"""
        if self.return_empty_select:
            return []
        if self.filter_str:
            if 'WHERE' in sql:
                sql += ' ' + self.filter_str
            else:
                sql += ' WHERE' + self.filter_str[3:]
            params += self.filter_params
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

    def get_records_path_data(self):
        """Return records IDs and EndTime (necessary to find the file path)"""
        data = []
        for res in self._filtered_query("SELECT RecordId, EndTime FROM UsageRecords",
                                        errorstr='Failed to get accounting records.'):
            data.append((res[0], res[1]))
        return data

    def get_records_count(self):
        """Return records count"""
        for res in self._filtered_query("SELECT COUNT(*) FROM UsageRecords", errorstr='Failed to get records count.'):
            return res[0]
        return 0

    def get_records_walltime(self):
        """Return total records walltime"""
        wallt = datetime.timedelta(0)
        for res in self._filtered_query("SELECT WallTime FROM UsageRecords", errorstr='Failed to get walltime values'):
            wallt += datetime.timedelta(seconds=res[0])
        return wallt

    def get_records_cputime(self):
        """Return total records cputime"""
        cput = datetime.timedelta(0)
        for res in self._filtered_query("SELECT CpuTime FROM UsageRecords", errorstr='Failed to get cputime values'):
            cput += datetime.timedelta(seconds=res[0])
        return cput

    def get_records_owners(self):
        """Return list of owners for selected records"""
        owners = self.get_owners()
        ids = []
        for res in self._filtered_query("SELECT DISTINCT OwnerId FROM UsageRecords",
                                        errorstr='Failed to get job owners'):
            ids.append(res[0])
        return [dn for dn in owners.keys() if owners[dn] in ids]

    def get_records_vos(self):
        """Return list of VOs for selected records"""
        vos = self.get_vos()
        ids = []
        for res in self._filtered_query("SELECT DISTINCT VOId FROM UsageRecords",
                                        errorstr='Failed to get job VOs'):
            ids.append(res[0])
        return [v for v in vos.keys() if vos[v] in ids]

    def get_records_dates(self):
        """Return startdate and enddate interval for selected records"""
        mindate = None
        for res in self._filtered_query("SELECT MIN(StartTime) FROM UsageRecords",
                                        errorstr='Failed to get minimum records start date'):
            mindate = res[0]
        maxdate = None
        for res in self._filtered_query("SELECT MAX(EndTime) FROM UsageRecords",
                                        errorstr='Failed to get maximum records start date'):
            maxdate = res[0]
        return mindate, maxdate

    def delete_records(self):
        """Remove records from database"""
        if not self.filter_str:
            self.logger.error('Removing records without applying filters is not allowed')
            return False
        self._filtered_query("DELETE FROM UsageRecords")
        self.con.commit()
        return True
