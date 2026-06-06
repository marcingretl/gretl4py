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
lines = scpt.splitlines()
gretl.run_hansl(scpt)
exit()


file = '/home/marcin/cvs/Gretl/gretl-gretl4py/demo/test.inp'
file = '/home/marcin/cvs/Gretl/gretl-gretl4py/demo/func.inp'
gretl.run(file)


exit()

# print accessor value
gretl.run_hansl("eval $sysinfo")

# open dataset and run OLS
gretl.run_hansl("open bjg")
gretl.run_hansl("ols 1 0 2")

# define script as list of strings
src = ['open denmark', 'list ylist = LRM LRY', 'list xlist = IBO IDE', 'vecm 4 1 ylist ; xlist']
gretl.run_hansl(src)

# run regular gretl script ('mle-advanced.inp')
file = gretl.gretldir + '/scripts/mle-advanced.inp'
print(file)
gretl.run(file)
