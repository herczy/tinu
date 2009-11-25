import os, time
import tempfile

from TaskGen import *

def rdfile(fn):
  f = open(fn, 'r')
  res = f.read()
  f.close()

  return res.strip()

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
  opt.add_option('--context-stack', action='store', type='int', dest='stacksize', default=128)
  opt.add_option('--build-example', action='store_true', dest='example', default=False)
  opt.add_option('--disable-dwarf', action='store_true', dest='nodwarf', default=False)

def configure(conf):
  from Options import options
  global VERSION

  conf.check_tool('gcc')
  conf.check_tool('misc')

  conf.check_cfg(package='glib-2.0', args='--libs --cflags', uselib_store='GLIB', mandatory=True)
  conf.check(lib='dl', mandatory=True)

  if not options.nodwarf:
    dwarf = conf.check(lib='dwarf')
    if dwarf:
      conf.check(lib='elf', mandatory=True)

    conf.env['HAVE_DWARF'] = dwarf;
    conf.define('HAVE_DWARF', int(dwarf))
  else:
    conf.env['HAVE_DWARF'] = False;
    conf.define('HAVE_DWARF', 0)

  conf.env['PREFIX'] = options.prefix
  conf.env['APPNAME'] = APPNAME

  conf.env['CXXFLAGS'] += ['-g', '-Wall']
  conf.env['LDFLAGS'] += ['-Wl,-E', '-rdynamic']

  conf.check_message_custom('current version', '', VERSION)

  conf.define('APPNAME', APPNAME)
  conf.define('VERSION', str(VERSION))
  conf.define('BUILDTIME', TIME)

  conf.check_message_custom('build time', '', TIME)

  conf.check_message_custom('test context stack size', '', "%d kb" % Options.options.stacksize)
  conf.define('TEST_CTX_STACK_SIZE', Options.options.stacksize * 1024)

  conf.env['EXAMPLE'] = options.example
  if options.example:
    conf.check_message_custom('building example', '', 'true')

  branch, rev = git_revision()
  if branch:
    conf.define('GIT_BRANCH', branch)
    conf.define('GIT_COMMIT', rev)
    conf.check_message_custom('git branch', '', branch)
    conf.check_message_custom('git commit id', '', rev)

  conf.write_config_header('config.h')

def build(bld):
  from Options import options
  bld.add_subdirs('lib test')

  if bld.env['EXAMPLE']:
    bld.add_subdirs('example')

  bld.install_files('${PREFIX}/include/tinu', 'lib/tinu/*.h')
  bld.install_files('${PREFIX}/include', 'lib/tinu.h')
  bld.install_files('${PREFIX}/include/tinu', blddir + '/default/config.h')

  bld.install_files('${PREFIX}/share/tinu', 'example/example.c')

# vim: syntax=python
