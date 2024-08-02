import sys
sys.path.append("../")
import gretl
import json

sysinfo = json.loads(gretl.sysinfo())
print("cpuid: " + sysinfo["cpuid"])
print("blas: " + sysinfo["blas"])
