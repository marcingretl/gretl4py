import gretl

d1 = gretl.get_data(gretl.gretl_data_path('misc/longley.gdt'))
d1.sample(t1=5, t2=10)
d1.print()

# we restore the full sample
d1.sample(full=True)

# let's use only obs for which armfrc >= 2000
d1.sample(restriction='armfrc>=2000', replace=True)
