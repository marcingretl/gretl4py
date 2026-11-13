import gretl

d = gretl.get_data("denmark")
gretl.pkg("install", "mpitestpack.gfn", local=True)
f = gretl.include("mpitestpack")
b = gretl.mpitest()
print(b)
