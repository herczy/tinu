import os, time
import tempfile

from TaskGen import *

VERSION = '0.1'
srcdir='.'
blddir='_build'
APPNAME='tinu'

def set_options(opt):
  pass

def configure(conf):
  from Options import options
  global VERSION

  conf.check_tool('gcc')
  conf.check_tool('misc')

  conf.check_cfg(package='glib-2.0', args='--libs --cflags', uselib_store='GLIB', mandatory=True)
  conf.check_cfg(package='applog', args='--libs --cflags', uselib_store='APPLOG', mandatory=True)

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

  bld.install_files('${PREFIX}/include/tinu', 'lib/tinu/*.h')
  bld.install_files('${PREFIX}/include', 'lib/tinu.h')
  bld.install_files('${PREFIX}/include/tinu', blddir + '/default/config.h')

# vim: syntax=python
