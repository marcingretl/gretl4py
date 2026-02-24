import gretl

# print accessor value
gretl.run_hansl("eval $sysinfo")

gretl.run_hansl("open bjg")
gretl.run_hansl("ols 1 0 2")

gretl.run_hansl("open denmark")
gretl.run_hansl("list ylist = LRM LRY")
gretl.run_hansl("list xlist = IBO IDE")
gretl.run_hansl("vecm 4 1 ylist ; xlist")
