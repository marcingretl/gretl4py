import gretl

# for matrix
M = gretl.Matrix(10, 10, 'normal')
gretl.summary(matrix=M)
M.summary()

# for variables, we need a dataset in place
d1 = gretl.get_data("greene19_1.gdt")

# for selected variables
gretl.summary([1, 2], by="PSI")

# for pre-definced list
d1.new_list(name='FOO', value='GPA GRADE PSI')
gretl.summary("FOO")
res = d1.get_accessor("$result")
print(res)

# for whole dataset
gretl.summary()
