import sys

from sqlite3 import connect
from tinu import load_using_args, Backend

from time import time

scripts = {
    'tinu_run_versions' : '''CREATE TABLE tinu_run_versions (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    create_time INTEGER
)''',
    'tinu_summary' : '''CREATE TABLE tinu_summary (
    run_id INTEGER REFERENCES tinu_run_versions(id) ON DELETE CASCADE,
    passed INTEGER,
    failed INTEGER,
    segfault INTEGER
)''',
    'tinu_messages' : '''CREATE TABLE tinu_messages (
    run_id INTEGER REFERENCES tinu_run_versions(id) ON DELETE CASCADE,
    critical INTEGER,
    error INTEGER,
    warning INTEGER,
    notice INTEGER,
    info INTEGER,
    debug INTEGER
)''',
    'tinu_suites' : '''CREATE TABLE tinu_suites (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    run_id INTEGER REFERENCES tinu_run_versions(id) ON DELETE CASCADE,
    name TEXT,
    result INTEGER,
    assert_passed INTEGER,
    assert_total INTEGER
)''',
    'tinu_tests' : '''CREATE TABLE tinu_tests (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    suite_id INTEGER REFERENCES tinu_suites(id) ON DELETE CASCADE,
    name TEXT,
    result INTEGER,
    assert_passed INTEGER,
    assert_total INTEGER,
    run_time FLOAT
)'''
}

class SqliteBackend(Backend):
    def __init__(self, db_name):
        self.db = None
        self.__check_database(db_name)

        self.run_id = self.__get_last_id('tinu_run_versions')

    def __check_database(self, db_name):
        global scripts

        self.db = connect(db_name)

        master = self.db.cursor()
        master.execute('SELECT * FROM sqlite_master')

        found = []
        missing = scripts.keys()
        for type, name, tbl_name, rootpage, sql in master:
            if not scripts.has_key(name):
                continue

            del missing[missing.index(name)]

            if sql != scripts[name]:
                raise Exception("Invalid sql file")

        for name in missing:
            self.db.executescript(scripts[name] + ';')

    def __get_last_id(self, seq):
        cur = self.db.cursor()
        cur.execute('SELECT seq - 1 AS id FROM sqlite_sequence WHERE name = ?', (seq, ))
        res = cur.fetchone()
        if res is None:
            return None
        return res[0]

    def __create_run_id(self):
        self.db.execute('INSERT INTO tinu_run_versions (create_time) VALUES (?)', (int(time()), ))
        self.run_id = self.__get_last_id('tinu_run_versions')

    def set_id(self, id):
        self.run_id = id

    def __find_suite(self, name):
        cur = self.db.cursor()
        cur.execute('SELECT id FROM tinu_suites WHERE name = ? AND run_id = ?', (name, self.run_id))
        return cur.fetchone()['id']

    def __delete_last_id(self):
        id = self.__get_last_id('tinu_run_versions')
        with self.db:
            self.db.execute("DELETE FROM tinu_run_versions WHERE id = ?", (id, ))
            self.db.execute("UPDATE sqlite_sequence SET seq = seq - 1 WHERE name = 'tinu_run_versions'")


    ##
    ## Interface
    ##
    def load_summary(self):
        cur = self.db.cursor()
        cur.execute('SELECT * FROM tinu_summary WHERE run_id = ? LIMIT 1', (self.run_id, ))
        line = cur.fetchone()
        return line['passed'], line['failed'], line['segfault']

    def load_message_counts(self):
        cur = self.db.cursor()
        cur.execute('SELECT * FROM tinu_messages WHERE run_id = ? LIMIT 1', (self.run_id, ))
        line = cur.fetchone()
        return line

    def load_tests(self, suite):
        suite_id = self.__find_suite(suite)
        cur = self.db.cursor()
        cur.execute('SELECT name, result, assert_passed, assert_total, time FROM tinu_suites WHERE suite_id = ?', (suite_id, ))
        for value in cur:
            yield value

    def load_suites(self, suite):
        cur = self.db.cursor()
        cur.execute('SELECT name, result, assert_passed, assert_total FROM tinu_suites WHERE run_id = ?', (self.run_id, ))
        for value in cur:
            yield value

    def save_results(self, results):
        self.__create_run_id()
        try:
            with self.db:
                self.db.execute("INSERT INTO tinu_summary (run_id, passed, failed, segfault) VALUES (?, ?, ?, ?)",
                    (self.run_id, results.summary.passed, results.summary.failed, results.summary.segfault))
                self.db.execute("INSERT INTO tinu_messages (critical, error, warning, notice, info, debug) VALUES (?, ?, ?, ?, ?, ?)",
                    (results.messages['critical'], results.messages['error'], results.messages['warning'], results.messages['notice'], results.messages['info'], results.messages['debug']))

            for name, suite in results.suites.iteritems():
                self.db.execute("INSERT INTO tinu_suites (run_id, name, result, assert_passed, assert_total) VALUES (?, ?, ?, ?, ?)",
                    (self.run_id, name, suite.result, suite.asserts.passed, suite.asserts.total))
                suite_id = self.__get_last_id('tinu_suites')

                with self.db:
                    for test, case in suite.tests.iteritems():
                        self.db.execute('INSERT INTO tinu_tests (suite_id, name, result, assert_passed, assert_total, run_time) VALUES (?, ?, ?, ?, ?, ?)',
                            (suite_id, test, case.result, case.asserts.passed, case.asserts.total, case.time))

        except:
            self.__delete_last_id()
            raise

try:
    backend = SqliteBackend(sys.argv[2])

except IndexError:
    sys.stderr.write('usage: %s <file> <db>\n', sys.argv[0])
    sys.exit(-1)

load_using_args().save_backend(backend)
