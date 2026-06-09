import gretl

# non-available commands
scpt = '''function void foo (void)
    eval $sysinfo
end function

foo()'''

scpt = '''open bjg
ols 1 0 2
eval $vcv'''

#scpt = '''include /home/marcin/cvs/Gretl/gretl-gretl4py/demo/test.inp'''
gretl.run_hansl(scpt)
#exit()

file = '/home/marcin/cvs/Gretl/gretl-gretl4py/demo/test.inp'
#file = '/home/marcin/cvs/Gretl/gretl-gretl4py/demo/func.inp'
gretl.run(file)

# print accessor value
gretl.run_hansl("eval $sysinfo")

# open dataset and run OLS
#gretl.run_hansl("open bjg")
#gretl.run_hansl("ols 1 0 2")
#exit()

# run regular gretl script ('mle-advanced.inp')
file = gretl.gretldir + '/scripts/mle-advanced.inp'
print(file)
gretl.run(file)
