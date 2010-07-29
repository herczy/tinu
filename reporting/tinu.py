def virtual(func):
    def __wrapper(*args, **kwargs):
        raise NotImplementedError("Function %s is virtual" % func.__name__)

    return __wrapper

def is_prefix(str, prefix, sep = '.'):
    if len(str) <= len(prefix):
        return False

    return str[len(prefix)] == sep and str[:len(prefix)] == prefix

class Summary(object):
    def __init__(self):
        self.passed = 0
        self.failed = 0
        self.segfault = 0

    def parse_item(self, key, value):
        if key == 'passed':
            self.passed = int(value)

        elif key == 'failed':
            self.failed = int(value)

        else:
            self.segfault = int(value)

    def load_backend(self, backend):
        self.passed, self.failed, self.segfault = backend.load_summary()

class Message(object):
    def __init__(self):
        self.msgcount = {}

    def parse_item(self, key, value):
        self.msgcount[key] = int(value)

    def __getitem__(self, key):
        return self.msgcount[key]

    def load_backend(self, backend):
        self.msgcount = backend.load_message_counts()

class Asserts(object):
    def __init__(self):
        self.total = 0
        self.passed = 0

    def parse_item(self, key, value):
        if key == 'passed':
            self.passed = int(value)

        else:
            self.total = int(value)

class Test(object):
    def __init__(self, suite, name):
        self.suite = suite
        self.name = name

        self.result = ''
        self.asserts = Asserts()
        self.time = 0

    def parse_item(self, key, value):
        if key == 'result':
            self.result = value

        if key == 'time':
            self.time = float(value)

        elif is_prefix(key, 'asserts'):
            _, rest = key.split('.', 1)
            self.asserts.parse_item(rest, value)

    def load_backend(self, backend, result, assert_total, assert_passed, time):
        self.result = result
        self.asserts.total = assert_total
        self.asserts.passed = assert_passed
        self.time = time

class Suite(object):
    def __init__(self, name):
        self.name = name
        self.tests = {}
        self.result = None
        self.asserts = Asserts()

    def parse_item(self, key, value):
        if is_prefix(key, 'test'):
            _, name, rest = key.split('.', 2)

            if not self.tests.has_key(name):
                self.tests[name] = Test(self, name)

            self.tests[name].parse_item(rest, value)

        elif key == 'result':
            self.result = int(value)

        elif is_prefix(key, 'asserts'):
            _, rest = key.split('.', 1)
            self.asserts.parse_item(rest, value)

    def load_backend(self, backend, result, apass, atotal):
        self.result = result
        self.asserts.total = atotal
        self.asserts.passed = apass

        self.tests = {}
        for test_name, result, assert_total, assert_passed, time in backend.load_tests(self.name):
            self.tests[test] = Test(self, test)
            self.tests[test].load_backend(backend, result, assert_total, assert_passed, time)

class TinuResult(object):
    def __init__(self):
        self.summary = Summary()
        self.messages = Message()
        self.suites = {}

    def __parse(self, line_iterator):
        for line in line_iterator:
            key, value = line.split('=', 1)
            cls, rest = key.split('.', 1)

            if cls == 'summary':
                self.summary.parse_item(rest, value)

            elif cls == 'message':
                self.messages.parse_item(rest, value)

            elif cls == 'suite':
                name, rest = rest.split('.', 1)
                if not self.suites.has_key(name):
                    self.suites[name] = Suite(name)

                self.suites[name].parse_item(rest, value)

    def load_iterator(self, iter):
        self.__parse(iter)

    def load_file(self, file_name):
        def __load_iterator(file):
            for line in file:
                if line[-2:] == '\r\n':
                    yield line[:-2]

                elif line[-1] == '\n':
                    yield line[:-1]

                else:
                    yield line

        fd = open(file_name, 'r')
        self.__parse(__load_iterator(fd))
        fd.close()

    def load_backend(self, backend):
        self.summary.load_backend(backend)
        self.messages.load_backend(backend)

        self.suites = {}
        for name, result, apass, atotal in backend.load_suites():
            self.suites[name] = Suite(name)
            self.suites[name].load_backend(backend, result, apass, atotal)

    def save_backend(self, backend):
        backend.save_results(self)

class Backend(object):
    @virtual
    def load_summary(self):
        pass

    @virtual
    def load_message_counts(self):
        pass

    @virtual
    def load_suites(self):
        pass

    @virtual
    def load_tests(self, suite):
        pass

    @virtual
    def save_results(self, results):
        pass

class ExampleBackend(Backend):
    def __init__(self, state):
        self.saved = state

    def load_summary(self):
        return self.saved.summary.passed, self.saved.summary.failed, self.saved.summary.segfault

    def load_message_counts(self):
        return self.saved.messages.msgcount

    def load_suites(self, suite):
        for name, obj in self.saved.suites.iteritems():
            yield name, obj.result, obj.asserts.total, obj.asserts.passed

    def load_tests(self, suite):
        for name, obj in self.saved.suites[suite].tests.iteritems():
            yield name, obj.result, obj.asserts.total, obj.asserts.passed, obj.time

    def save_results(self, results):
        self.saved = results

    def dump(self):
        res = []
        def _(*args):
            res.append(' '.join(map(str, args)))

        _('Summary')
        _('  Passed : %s' % self.saved.summary.passed)
        _('  Failed : %s' % self.saved.summary.failed)
        _('  SIGSEGV: %s' % self.saved.summary.segfault)

        _('Message counts')
        for key, value in self.saved.messages.msgcount.iteritems():
            _('  %s = %s' % (key, value))

        for name, suite in self.saved.suites.iteritems():
            _('Suite %s' % name)
            _('  Result: %s' % suite.result)
            _('  Assert passes: %d/%d' % (suite.asserts.passed, suite.asserts.total))

            for tname, case in suite.tests.iteritems():
                _('  Test case %s' % tname)
                _('    Result       : %s' % case.result)
                _('    Assert passes: %d/%d' % (case.asserts.passed, case.asserts.total))
                _('    Time         : %.3lf' % case.time)

        return res

def load_using_args():
    import sys

    cfg = TinuResult()
    try:
        cfg.load_file(sys.argv[1])

    except IndexError:
        sys.stderr.write("Usage: %s <filename>\n" % sys.argv[0])
        sys.exit(-1)

    return cfg
