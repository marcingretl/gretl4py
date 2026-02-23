import gretl

# print accessor value
gretl.run_hansl("eval $sysinfo")

# estimate model (dataset loaded before)
d = gretl.get_data("bjg")
gretl.run_hansl("ols 1 0 2")
