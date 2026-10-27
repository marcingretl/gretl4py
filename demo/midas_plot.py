import gretl

# Load dataset.
d = gretl.get_data("gdp_midas.gdt")

# Build MIDAS regressors.
dX = "ld_payems_m3 ld_payems_m2 ld_payems_m1"
dXL = d.hflags(3, 11, dX)

d.hfplot([dXL, dX], with_lines=True, time_series=True)
