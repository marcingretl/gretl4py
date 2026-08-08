import gretl

# ------------------------------------------------------------
# Built-in accessors
# ------------------------------------------------------------
print("Built-in accessor:")
gretl.run_hansl("eval $sysinfo")

# ------------------------------------------------------------
# Defining and calling a hansl function
# ------------------------------------------------------------
print("User-defined hansl function:")

script = """
function void foo (void)
    print "result from function"
    eval $sysinfo
end function

foo()
"""
gretl.run_hansl(script)

# ------------------------------------------------------------
# Running a hansl script
# ------------------------------------------------------------
print("OLS example:")

script = """
open bjg
ols 1 0 2
eval $vcv
"""
gretl.run_hansl(script)

# ------------------------------------------------------------
# Running an existing .inp file
# ------------------------------------------------------------
script_path = gretl.gretldir + "/scripts/mle-advanced.inp"

print("Running an .inp file:")
gretl.run_script(script_path)

# Alternatively, the same file can be included from hansl.
print("Including the .inp file from hansl:")
gretl.run_hansl(f"include {script_path}")
