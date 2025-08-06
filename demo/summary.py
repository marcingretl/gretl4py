import gretl
d1 = gretl.get_data("greene19_1.gdt")

# for selected variables
gretl.summary("GPA TUCE", by="PSI")

# for pre-definced list
d1.new_list(name='FOO', value='GPA GRADE PSI')
gretl.summary("FOO")
res = d1.get_accessor("$result")
print(res)

# for whole dataset
gretl.summary()

# for matrix
gretl.genr('matrix M = mnormal(100,10)')
gretl.summary(matrix="M")
