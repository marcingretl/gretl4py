import gretl

# Print the value of a built-in accessor.
gretl.run_hansl("eval $sysinfo")

# Run a simple hansl script defining and calling a function.
script = """
function void foo (void)
    print "result from function"
    eval $sysinfo
end function

foo()
"""
gretl.run_hansl(script)

# Open a dataset, estimate an OLS model, and print the covariance matrix.
script = """
open bjg
ols 1 0 2
eval $vcv
"""
gretl.run_hansl(script)

# Run an existing gretl script.
script_path = gretl.gretldir + "/scripts/mle-advanced.inp"
gretl.run_script(script_path)

# Include the same script from within hansl.
gretl.run_hansl(f"include {script_path}")
