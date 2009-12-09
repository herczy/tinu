import os, time, sys
import tempfile
import optparse
import hashlib

from TaskGen import *
import Options

def rdfile(fn):
  f = open(fn, 'r')
  res = f.read()
  f.close()

  return res.strip()

def calcChecksum(filename):
  sha1 = hashlib.sha1()
  md5 = hashlib.md5()

  f = file(filename, 'r')
  while True:
    data = f.read(4096)
    if len(data) == 0:
      break

    sha1.update(data)
    md5.update(data)

  f.close()
  return sha1.hexdigest(), md5.hexdigest()

VERSION = '0.1'
if os.access('VERSION', os.R_OK):
  VERSION = rdfile('VERSION')
srcdir='.'
blddir='_build_'
APPNAME='tinu'
TIME=time.strftime("%a, %d %b %Y %H:%M:%S +0000", time.gmtime())

def git_revision():
  if not os.access('.git/HEAD', os.R_OK):
    return None, None

  head = rdfile('.git/HEAD')
  branch = head[5:]

  rev = rdfile('.git/' + branch)
  branch = branch.split('/')[-1]

  return branch, rev

def set_options(opt):
  confopt = optparse.OptionGroup(opt.parser, "Configure options")
  confopt.add_option('--context-stack', action='store', type='int', dest='stacksize', default=128,
    help="Set allocated stack size for test cases")
  confopt.add_option('--build-example', action='store_true', dest='example', default=False,
    help="Build example unittest provided")
  confopt.add_option('--disable-dwarf', action='store_true', dest='nodwarf', default=False,
    help="Disable DWARF support")
  confopt.add_option('--disable-coredumper', action='store_true', dest='nocoredumper', default=False,
    help="Disable Google coredumper support")
  confopt.add_option('--enable-profiling', action='store_true', dest='profiling', default=False,
    help="Enable profiling info (usable with gprof)")
  confopt.add_option('--enable-coverage', action='store_true', dest='coverage', default=False,
    help="Enable coverage info (usable with gcov)")

  distopt = optparse.OptionGroup(opt.parser, "Distmake options")
  distopt.add_option('--snapshot', action='store_true', dest='snapshot', default=False,
    help="Make a snapshot")

  if 'dist' in sys.argv:
    opt.add_option_group(distopt)

  if 'configure' in sys.argv:
    opt.add_option_group(confopt)

def configure(conf):
  global VERSION

  conf.check_tool('gcc')
  conf.check_tool('misc')

  conf.check_cfg(package='glib-2.0', args='--libs --cflags', uselib_store='GLIB', mandatory=True)
  conf.check(lib='dl', mandatory=True)

  if not Options.options.nodwarf:
    dwarf = conf.check(lib='dwarf')
    needelf = True

    conf.env['HAVE_DWARF'] = dwarf;
    conf.define('HAVE_DWARF', int(dwarf))
  else:
    needelf = False

    conf.env['HAVE_DWARF'] = False;
    conf.define('HAVE_DWARF', 0)

  conf.env['PREFIX'] = Options.options.prefix
  conf.env['APPNAME'] = APPNAME

  conf.env['CCFLAGS'] += ['-g', '-Wall']
  conf.env['LINKFLAGS'] += ['-Wl,-E', '-rdynamic']

  if not Options.options.nocoredumper:
    needelf = True

    conf.env['HAVE_COREDUMPER'] = True
    conf.define('HAVE_COREDUMPER', 1)

  if needelf:
    conf.check(lib='elf', mandatory=True)

  if Options.options.profiling:
    conf.env['CCFLAGS'].append('-pg')
    conf.env['LINKFLAGS'].append('-pg')
    conf.check_message_custom('profiling info enabled', '', 'true', color='GREEN')
  else:
    conf.check_message_custom('profiling info enabled', '', 'false', color='YELLOW')

  if Options.options.coverage:
    conf.env['CCFLAGS'].extend(['-fprofile-arcs', '-ftest-coverage'])
    conf.env['LINKFLAGS'].extend(['-fprofile-arcs', '-ftest-coverage'])
    conf.check_message_custom('coverage info enabled', '', 'true', color='GREEN')
  else:
    conf.check_message_custom('coverage info enabled', '', 'false', color='YELLOW')

  conf.check_message_custom('current version', '', VERSION)

  conf.define('APPNAME', APPNAME)
  conf.define('VERSION', str(VERSION))
  conf.define('BUILDTIME', TIME)

  conf.check_message_custom('build time', '', TIME)

  conf.check_message_custom('test context stack size', '', "%d kb" % Options.options.stacksize)
  conf.define('TEST_CTX_STACK_SIZE', Options.options.stacksize * 1024)

  conf.env['EXAMPLE'] = Options.options.example
  if Options.options.example:
    conf.check_message_custom('building example', '', 'true')

  branch, rev = git_revision()
  if branch:
    conf.define('GIT_BRANCH', branch)
    conf.define('GIT_COMMIT', rev)
    conf.check_message_custom('git branch', '', branch)
    conf.check_message_custom('git commit id', '', rev)

  conf.write_config_header('config.h')

def build(bld):
  if bld.env['HAVE_COREDUMPER']:
    bld.add_subdirs('coredumper')

  bld.add_subdirs('lib test')

  if bld.env['EXAMPLE']:
    bld.add_subdirs('example')

  bld.install_files('${PREFIX}/include/tinu', 'lib/tinu/*.h')
  bld.install_files('${PREFIX}/include', 'lib/tinu.h')
  bld.install_files('${PREFIX}/include/tinu', blddir + '/default/config.h')

  bld.install_files('${PREFIX}/share/tinu', 'example/example.c')

def dist():
  import Scripting, Utils
  global VERSION
  version = VERSION
  dir = ''

  if Options.options.snapshot:
    branch, rev = git_revision()
    if rev:
      version = version + '+' + branch + '#' + rev
    else:
      version = version + time.strftime('+%Y%m%d')

    dir = blddir + '/snapshot-' + branch
  else:
    dir = blddir + '/release'

  if not os.path.exists(dir):
    os.mkdir(dir)
  elif not os.path.isdir(dir):
    raise Exception, dir + ' is not a directory'

  Scripting.g_gz = 'gz'
  arch_name = Scripting.dist(APPNAME, version)

  sha1, md5 = calcChecksum(arch_name)
  f = file('%s/%s-%s.md5' % (dir, APPNAME, version), 'w')
  f.write('%s %s' % (md5, arch_name))
  f.close()

  f = file('%s/%s-%s.sha1' % (dir, APPNAME, version), 'w')
  f.write('%s %s' % (sha1, arch_name))
  f.close()

  os.system('mv %s %s/%s' % (arch_name, dir, arch_name))

  Utils.pprint('GREEN', 'md5: %s; sha1: %s' % (md5, sha1))

# vim: syntax=python
