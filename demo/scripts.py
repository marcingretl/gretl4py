import gretl

# non-available commands
scpt = '''function void foo (void)
print "result from function"
eval $sysinfo
end function

foo()'''
gretl.run_hansl(scpt)

scpt = '''open bjg
ols 1 0 2
eval $vcv'''
gretl.run_hansl(scpt)


# print accessor value
gretl.run_hansl("eval $sysinfo")

# run regular gretl script ('mle-advanced.inp')
file = gretl.gretldir + '/scripts/mle-advanced.inp'
gretl.run_script(file)

# as include
line = f'include {file}'
gretl.run_hansl(line)
