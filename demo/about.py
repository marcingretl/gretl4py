import sysconfig
sysconfig.os.environ['GRETL4PY_VERBOSE'] = '1'
print('GRETL4PY_VERBOSE set to 1: all should be printed')
import gretl

# test the content of $sysinfo
gretl.about(0)
sysconfig.os.environ.pop('GRETL4PY_VERBOSE', None)
