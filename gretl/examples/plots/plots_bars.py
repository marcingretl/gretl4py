import gretl
import matplotlib.pyplot as plt


##################################################
#   dataset
##################################################
d = gretl.get_data("denmark")

# First band: narrower, filled.
b1 = gretl.Bundle()
b1["center"] = "LRY"
b1["width"] = 0.05
b1["factor"] = 1.0
b1["style"] = "fill"
b1["title"] = "Narrow band"

# Second band: wider, dashed boundaries.
b2 = gretl.Bundle()
b2["center"] = "LRY"
b2["width"] = 0.10
b2["factor"] = 1.0
b2["style"] = "dash"
b2["title"] = "Wide band"

fig1 = d.plot("LRY", time_series=True, with_lines=True, band=b1)
fig2 = d.plot("LRY", time_series=True, with_lines=True, bands=[b1, b2])

##################################################
#   matrix
##################################################

m = gretl.Matrix(100, 1, "normal")

# Band: center = 0, half-width = 1.
bandmat = gretl.Matrix.zeros(100, 2)
bandmat.numpy_view[:, 1] = 1.0 

b3 = gretl.Bundle()
b3["bandmat"] = bandmat
b3["factor"] = 1.0
b3["style"] = "fill"
b3["title"] = "±1 sigma"

# Second band: center = 0, half-width = 2.
bandmat.numpy_view[:, 1] = 2.0
b4 = gretl.Bundle()
b4["bandmat"] = bandmat
b4["factor"] = 1.0
b4["style"] = "dash"
b4["title"] = "±2 sigma"

fig3 = m.plot("1", time_series=True, with_lp=True, band=b3)
fig4 = m.plot([1], time_series=True, with_lp=True, bands=[b3, b4])

plt.show()
