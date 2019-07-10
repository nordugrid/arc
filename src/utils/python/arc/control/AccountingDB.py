import sys
import os
import logging
import datetime
import sqlite3


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

    def close(self):
        """Terminate database connection"""
        if self.con is not None:
            self.con.close()
        self.con = None

    def __del__(self):
        self.close()

