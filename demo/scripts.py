import gretl

# print accessor value
gretl.run_hansl("eval $sysinfo")

# estimate model (dataset loaded before)
gretl.run_hansl("open bjg")
gretl.run_hansl("ols 1 0 2")
