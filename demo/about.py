import sysconfig
sysconfig.os.environ['GRETL4PY_VERBOSE'] = '1'
print('GRETL4PY_VERBOSE set to 1: all should be printed')
import gretl

# test the content of $sysinfo
d = gretl.about(0)

# mod_ver should be 0.1
assert d['mod_ver'] == '0.1',"module version should be 0.1!!!"
sysconfig.os.environ.pop('GRETL4PY_VERBOSE', None)
