import gretl

fname = gretl.gretl_data_path('misc/longley.gdt')
gretl.attach_gretl_data(fname)

res = gretl.mpols('1 0 2 3 4 5 6 7').fit()
print(res)
