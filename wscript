import os, time
import tempfile

from TaskGen import *

VERSION = '0.1'
srcdir='.'
blddir='_build'
APPNAME='tinu'

def set_options(opt):
  opt.add_option('--enable-threads', dest='threads', action='store_true', default=False, help='Enable threads')

def configure(conf):
  from Options import options
  global VERSION

  conf.check_tool('gcc')
  conf.check_tool('misc')

  conf.check_cfg(package='glib-2.0', args='--libs --cflags', uselib_store='GLIB', mandatory=True)

  if options.threads:
    conf.define('ENABLE_THREADS', 1)
    conf.check_cfg(package='gthread-2.0', args='--libs --cflags', uselib_store='GTHREAD', mandatory=True)

  conf.env['PREFIX'] = options.prefix
  conf.env['APPNAME'] = APPNAME

  conf.env['CXXFLAGS'] += ['-g', '-Wall']
  conf.env['LDFLAGS'] += ['-Wl,-E', '-rdynamic']

  conf.check_message_custom('current version', '', VERSION)

  conf.define('APPNAME', APPNAME)
  conf.define('VERSION', str(VERSION))

  conf.write_config_header('config.h')

def build(bld):
  bld.add_subdirs('lib test')

  #bld.install_files('${PREFIX}/include/unittest', 'lib/unittest/*.h')
  #bld.install_files('${PREFIX}/include/unittest', blddir + '/default/lib/unittest/*.h')
  #bld.install_files('${PREFIX}/include/unittest', blddir + '/default/config.h')
  #bld.install_files('${PREFIX}/include', 'lib/unittest/unittest.h')

# vim: syntax=python
