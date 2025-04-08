import gretl

d1 = gretl.get_data('tobit.gdt')
m1 = gretl.tobit('apt ~ const + read + math + proggeneral + progvocational', llimit=400, rlimit=600.7).fit()
print(m1)
