''' OBSOLETE: to be deleted sysinfo() '''

import sys
sys.path.append('./')
sys.path.append('../')
import json

sysinfo = json.loads(gretl.sysinfo())
print("cpuid: " + sysinfo["cpuid"])
print("blas: " + sysinfo["blas"])
