import gretl

# non-available commands
#src = ['x = mnormal(2,2)', 'print x', 'delete x', 'print x']
src = ['delete x']
gretl.run_hansl(src)
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
