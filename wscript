#!/usr/bin/env python
#encoding: utf-8

# Bring in waf tools
import tools.waf.conf
import tools.waf.test

APPNAME = 'rlib'
VERSION = '0.0.1'
APIVERSION = '0.1'

top = '.'
out = '_build_'

DBGVAR = 'debug'
RELVAR = 'release'
DEFVAR = RELVAR
VARIANTS = [ DBGVAR, RELVAR ]

def options(opt):
    opt.load('compiler_c')
    opt.add_option('--variant',
            action='store', dest='variant', default=DEFVAR,
            help='use variant %r (defualt: %s)' % (VARIANTS, DEFVAR))
    opt.add_option('-d', '--debug',
            action='store_const', const=DBGVAR, dest='variant',
            help='use the debug variant')

def configure(cfg):
    cfg.load('compiler_c')

    ##################################
    # COMMON (for all variants)
    ##################################
    configure_os_arch(cfg)
    configure_headers(cfg)
    configure_string(cfg)
    configure_sizeof(cfg)
    configure_printf(cfg)
    configure_time(cfg)
    configure_threads(cfg)
    configure_signal(cfg)

    if cfg.env['CC_NAME'] == 'msvc':
        cfg.env.CPPFLAGS += ['/Zi', '/FS'] #, '/Wall']
    else:
        cfg.env.CPPFLAGS += ['-Wall', '-Werror', '-Wextra']
        cfg.env.CFLAGS += ['-fvisibility=hidden']

    # This is probably wrong, but works for now.
    # Read: There are probably elf based systmes which doesn't support dynamic linking
    if cfg.env.DEST_BINFMT == 'elf':
        cfg.env.LINKFLAGS_RTEST = ['-Wl,--export-dynamic']

    common_env = cfg.env

    # DEBUG
    cfg.setenv(DBGVAR, env=common_env)
    cfg.env.detach()
    if cfg.env['CC_NAME'] == 'msvc':
        cfg.env.CFLAGS += ['/MDd', '/Od']
        cfg.env.LINKFLAGS += ['/DEBUG']
    else:
        cfg.env.CFLAGS += ['-g']
    cfg.define('DEBUG', 1)
    cfg.write_config_header(DBGVAR+'/config.h')

    # RELEASE
    cfg.setenv(RELVAR, env=common_env)
    cfg.env.detach()
    if cfg.env['CC_NAME'] == 'msvc':
        cfg.env.CFLAGS += ['/MD', '/O2']
    else:
        cfg.env.CFLAGS += ['-O2']
    cfg.define('NDEBUG', 1)
    cfg.write_config_header(RELVAR+'/config.h')

def buildall(ctx):
    from waflib.Options import commands
    for var in VARIANTS:
        commands.insert(0, 'build_' + var)

def build(bld):
    if not bld.variant:
        bld.fatal('No variant specified')

    bld(    features    = 'subst',
            source      = 'rlib/rconfig.h.in',
            target      = 'rlib/rconfig.h')

    bld.shlib(
            source      = bld.path.ant_glob('rlib/*.c'),
            target      = APPNAME,
            vnum        = APIVERSION,
            includes    = [ '.' ],
            defines     = [ 'RLIB_COMPILATION', 'RLIB_SHLIB' ],
            use         = 'DL PTHREAD RT')

    bld.recurse('example test')

def init(ctx):
    from waflib.Build import BuildContext, CleanContext, ListContext, InstallContext, UninstallContext, StepContext
    from tools.waf.test import TestContext
    from waflib.Options import options

    for y in (BuildContext, CleanContext, ListContext, InstallContext, UninstallContext, StepContext, TestContext):
        name = y.__name__.replace('Context','').lower()
        class tmp(y):
            cmd = name
            variant = options.variant

        for var in VARIANTS:
            class tmp(y):
                cmd = name + '_' + var
                variant = var

def configure_os_arch(cfg):
    cfg.start_msg('Checking dest/host OS')
    cfg.env.RLIB_OS = 'R_OS_WIN32' if cfg.env.DEST_OS == 'win32' else 'R_OS_UNIX'
    if cfg.env.DEST_OS == 'linux':
        cfg.env.RLIB_OS_EXTRA = '#define R_OS_LINUX              1'
    elif cfg.env.DEST_OS == 'darwin':
        cfg.env.RLIB_OS_EXTRA = '#define R_OS_DARWIN             1'
    elif cfg.env.DEST_OS == 'freebsd':
        cfg.env.RLIB_OS_EXTRA = '#define R_OS_FREEBSD            1'
    elif cfg.env.DEST_OS == 'netbsd':
        cfg.env.RLIB_OS_EXTRA = '#define R_OS_NETBSD             1'
    elif cfg.env.DEST_OS == 'openbsd':
        cfg.env.RLIB_OS_EXTRA = '#define R_OS_OPENBSD            1'
    cfg.end_msg(cfg.env.DEST_OS, 'CYAN')
    cfg.start_msg('Checking dest/host CPU/ARCH')
    archcolor = 'CYAN'
    if cfg.env.DEST_CPU in ['x86_64', 'amd64', 'x64']:
        cfg.env.RLIB_ARCH = 'R_ARCH_X86_64'
    elif cfg.env.DEST_CPU == 'x86':
        cfg.env.RLIB_ARCH = 'R_ARCH_X86'
    elif cfg.env.DEST_CPU == 'ia':
        cfg.env.RLIB_ARCH = 'R_ARCH_IA64'
    elif cfg.env.DEST_CPU == 'arm':
        cfg.env.RLIB_ARCH = 'R_ARCH_ARM'
    elif cfg.env.DEST_CPU == 'thumb':
        cfg.env.RLIB_ARCH = 'R_ARCH_THUMB'
    elif cfg.env.DEST_CPU == 'aarch64':
        cfg.env.RLIB_ARCH = 'R_ARCH_AARCH64'
    elif cfg.env.DEST_CPU == 'mips':
        cfg.env.RLIB_ARCH = 'R_ARCH_MIPS'
    elif cfg.env.DEST_CPU == 'sparc':
        cfg.env.RLIB_ARCH = 'R_ARCH_SPARC'
    elif cfg.env.DEST_CPU == 'alpha':
        cfg.env.RLIB_ARCH = 'R_ARCH_ALPHA'
    elif cfg.env.DEST_CPU == 'powerpc':
        cfg.env.RLIB_ARCH = 'R_ARCH_POWERPC'
    else:
        cfg.env.RLIB_ARCH = 'R_ARCH_UNKNOWN'
        archcolor = 'RED'
    cfg.end_msg(cfg.env.DEST_CPU, archcolor)

def configure_headers(cfg):
    cfg.multicheck(
            {'header_name':'stdio.h'},
            {'header_name':'stdlib.h'},
            {'header_name':'stdarg.h'},
            {'header_name':'limits.h'},
            {'header_name':'float.h'},
            {'header_name':'string.h'},
            msg = 'Checking for standard headers')

    if cfg.check(header_name='alloca.h', mandatory=False):
        cfg.env.RLIB_DEFINE_HAVE_ALLOCA_H = '#define RLIB_HAVE_ALLOCA_H      1'
    else:
        cfg.env.RLIB_DEFINE_HAVE_ALLOCA_H = '/* #undef RLIB_HAVE_ALLOCA_H */'

    if not cfg.env.DEST_OS == 'win32':
        cfg.check(header_name='dlfcn.h')
        cfg.check(lib='dl')

    cfg.check(header_name='sched.h', mandatory=False)
    cfg.check(header_name='sys/sysctl.h', mandatory=False)
    if cfg.env.DEST_OS == 'linux':
        cfg.check(header_name='sys/prctl.h')
        cfg.check(header_name='sys/sysinfo.h')

    cfg.check(header_name='sys/time.h', mandatory=False)
    if cfg.env.DEST_OS == 'darwin':
        cfg.check(header_name='mach/clock.h')
        cfg.check(header_name='mach/mach_time.h')

def configure_string(cfg):
    cfg.check_cc(function_name='stpcpy',
            header_name="string.h", mandatory=False)

def configure_printf(cfg):
    funcs = ['printf', 'fprintf', 'sprintf',
             'vprintf', 'vfprintf', 'vsprintf',
             'snprintf', 'vsnprintf',
             'asprintf', 'vasprintf',
             '_vscprintf']
    for f in funcs:
        cfg.check_cc(function_name=f,header_name="stdio.h", mandatory=False)

def configure_time(cfg):
    if cfg.env.DEST_OS not in [ 'darwin', 'win32']:
        cfg.check_cc(function_name='clock_gettime',
                header_name="time.h", lib='rt', mandatory=False)


SNIP_PTHREAD_CALL = '''
#include <pthread.h>

int main(int argc, char **argv) {
  (void)argc; (void)argv;
  %s;
  return 0;
}
'''
def configure_threads(cfg):
    cfg.check_cc(function_name='gettid',
            header_name="sys/types.h", mandatory=False)
    if cfg.check(header_name='pthread.h', mandatory=False):
        cfg.check_cc(function_name='pthread_getname_np', defines=['_GNU_SOURCE=1'],
                header_name="pthread.h", lib='pthread', mandatory=False)
        cfg.check_cc(
                fragment=SNIP_PTHREAD_CALL % 'pthread_setname_np (0, "test")',
                define_name="HAVE_PTHREAD_SETNAME_NP_WITH_TID",
                msg='Checking for pthread_setname_np (pthread_t, const char*)',
                defines=['_GNU_SOURCE=1'], lib='pthread', mandatory=False)
        cfg.check_cc(
                fragment=SNIP_PTHREAD_CALL % 'pthread_setname_np ("test")',
                msg='Checking for pthread_setname_np (const char*)',
                define_name="HAVE_PTHREAD_SETNAME_NP_WITHOUT_TID",
                defines=['_GNU_SOURCE=1'], lib='pthread', mandatory=False)
        cfg.check_cc(function_name='pthread_getthreadid_np', defines=['_GNU_SOURCE=1'],
                header_name="pthread.h", lib='pthread', mandatory=False)
        cfg.check_cc(function_name='pthread_threadid_np', defines=['_GNU_SOURCE=1'],
                header_name="pthread.h", lib='pthread', mandatory=False)

def configure_signal(cfg):
    if not cfg.env.DEST_OS == 'win32':
        cfg.check(lib='rt', mandatory=False)
        cfg.check_cc(function_name='timer_create', lib='rt',
                header_name="time.h", mandatory=False)
        cfg.check_cc(function_name='setitimer',
                header_name="sys/time.h", mandatory=False)
        cfg.check_cc(function_name='alarm',
                header_name="unistd.h", mandatory=False)

def configure_sizeof(cfg):
    sizeof_short = cfg.check_sizeof('short', guess=2)
    sizeof_int = cfg.check_sizeof('int', guess=4)
    sizeof_long = cfg.check_sizeof('long', guess=8)
    sizeof_longlong = cfg.check_sizeof('long long', guess=8)
    sizeof_size_t = cfg.check_sizeof('size_t', guess=sizeof_longlong)
    sizeof_void_p = cfg.check_sizeof('void*', guess=sizeof_size_t)

    if sizeof_short == 2:
        cfg.env.RINT16_TYPE = 'short'
        cfg.env.RINT16_MODIFIER = '"h"'
        cfg.env.RINT16_FMT = '"hi"'
        cfg.env.RUINT16_FMT = '"hu"'
    elif sizeof_int == 2:
        cfg.env.RINT16_TYPE = 'int'
        cfg.env.RINT16_MODIFIER = '""'
        cfg.env.RINT16_FMT = '"i"'
        cfg.env.RUINT16_FMT = '"u"'
    if sizeof_short == 4:
        cfg.env.RINT32_TYPE = 'short'
        cfg.env.RINT32_MODIFIER = '"h"'
        cfg.env.RINT32_FMT = '"hi"'
        cfg.env.RUINT32_FMT = '"hu"'
    elif sizeof_int == 4:
        cfg.env.RINT32_TYPE = 'int'
        cfg.env.RINT32_MODIFIER = '""'
        cfg.env.RINT32_FMT = '"i"'
        cfg.env.RUINT32_FMT = '"u"'
    elif sizeof_long == 4:
        cfg.env.RINT32_TYPE = 'long'
        cfg.env.RINT32_MODIFIER = '"l"'
        cfg.env.RINT32_FMT = '"li"'
        cfg.env.RUINT32_FMT = '"lu"'
    if sizeof_int == 8:
        cfg.env.RINT64_TYPE = 'int'
        cfg.env.RINT64_MODIFIER = '""'
        cfg.env.RINT64_FMT = '"i"'
        cfg.env.RUINT64_FMT = '"u"'
        cfg.env.RINT64_CONST = 'val'
        cfg.env.RUINT64_CONST = 'val'
    elif sizeof_long == 8:
        cfg.env.RINT64_TYPE = 'long'
        cfg.env.RINT64_MODIFIER = '"l"'
        cfg.env.RINT64_FMT = '"li"'
        cfg.env.RUINT64_FMT = '"lu"'
        cfg.env.RINT64_CONST = 'val##L'
        cfg.env.RUINT64_CONST = 'val##UL'
    elif sizeof_longlong == 8:
        cfg.env.RINT64_TYPE = 'long long'
        cfg.env.RINT64_MODIFIER = '"ll"'
        cfg.env.RINT64_FMT = '"lli"'
        cfg.env.RUINT64_FMT = '"llu"'
        cfg.env.RINT64_CONST = 'val##LL'
        cfg.env.RUINT64_CONST = 'val##ULL'

    if sizeof_size_t == sizeof_short:
        cfg.env.RSIZE_TYPE = 'short'
        cfg.env.RSIZE_MODIFIER = '"h"'
        cfg.env.RSIZE_FMT = '"hu"'
        cfg.env.RSSIZE_FMT = '"hi"'
        cfg.env.RLIB_SIZE_TYPE = 'RUSHORT'
        cfg.env.RLIB_SSIZE_TYPE = 'RSHORT'
    elif sizeof_size_t == sizeof_int:
        cfg.env.RSIZE_TYPE = 'int'
        cfg.env.RSIZE_MODIFIER = '""'
        cfg.env.RSIZE_FMT = '"u"'
        cfg.env.RSSIZE_FMT = '"i"'
        cfg.env.RLIB_SIZE_TYPE = 'RUINT'
        cfg.env.RLIB_SSIZE_TYPE = 'RINT'
    elif sizeof_size_t == sizeof_long:
        cfg.env.RSIZE_TYPE = 'long'
        cfg.env.RSIZE_MODIFIER = '"l"'
        cfg.env.RSIZE_FMT = '"lu"'
        cfg.env.RSSIZE_FMT = '"li"'
        cfg.env.RLIB_SIZE_TYPE = 'RULONG'
        cfg.env.RLIB_SSIZE_TYPE = 'RLONG'
    elif sizeof_size_t == sizeof_longlong:
        cfg.env.RSIZE_TYPE = 'long long'
        cfg.env.RSIZE_MODIFIER = '"I64"'
        cfg.env.RSIZE_FMT = '"I64u"'
        cfg.env.RSSIZE_FMT = '"I64i"'
        cfg.env.RLIB_SIZE_TYPE = 'RUINT64'
        cfg.env.RLIB_SSIZE_TYPE = 'RINT64'

    if sizeof_void_p == sizeof_int:
        cfg.env.RINTPTR_TYPE = 'int'
        cfg.env.RINTPTR_MODIFIER = '""'
        cfg.env.RINTPTR_FMT = '"i"'
        cfg.env.RUINTPTR_FMT = '"u"'
        cfg.env.RINTMAX_TYPE = 'int'
        cfg.env.RINTMAX_MODIFIER = '""'
        cfg.env.RINTMAX_FMT = '"i"'
        cfg.env.RUINTMAX_FMT = '"u"'
    elif sizeof_void_p == sizeof_long:
        cfg.env.RINTPTR_TYPE = 'long'
        cfg.env.RINTPTR_MODIFIER = '"l"'
        cfg.env.RINTPTR_FMT = '"li"'
        cfg.env.RUINTPTR_FMT = '"lu"'
        cfg.env.RINTMAX_TYPE = 'long'
        cfg.env.RINTMAX_MODIFIER = '"l"'
        cfg.env.RINTMAX_FMT = '"li"'
        cfg.env.RUINTMAX_FMT = '"lu"'
    elif sizeof_void_p == sizeof_longlong:
        cfg.env.RINTPTR_TYPE = 'long long'
        cfg.env.RINTPTR_MODIFIER = '"I64"'
        cfg.env.RINTPTR_FMT = '"I64i"'
        cfg.env.RUINTPTR_FMT = '"I64u"'
        cfg.env.RINTMAX_TYPE = 'long long'
        cfg.env.RINTMAX_MODIFIER = '"I64"'
        cfg.env.RINTMAX_FMT = '"I64i"'
        cfg.env.RUINTMAX_FMT = '"I64u"'

    cfg.env.RLIB_SIZEOF_VOID_P = sizeof_void_p
    cfg.env.RLIB_SIZEOF_LONG = sizeof_long
    cfg.env.RLIB_SIZEOF_SIZE_T = sizeof_size_t

