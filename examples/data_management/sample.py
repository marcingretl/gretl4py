import gretl

gretl.attach_gretl_data(gretl.gretl_data_path('misc/longley.gdt'))
gretl.sample(t1=5, t2=10)
gretl.printdata()
