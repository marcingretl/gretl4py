import gretl

d1 = gretl.get_data('tobit.gdt')
m1 = gretl.tobit('apt ~ const + read + math + proggeneral + progvocational').fit()
print(m1)
