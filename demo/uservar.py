import importlib.resources as resources

import gretl
import numpy as np

np.set_printoptions(precision=4)

# Use a period as the decimal-point character in libgretl.
gretl.set("force_decpoint", "on")

# Locate the data directory included in the gretl package.
data_dir = resources.files("gretl").joinpath("data")

# Load a dataset.
longley = gretl.get_data(str(data_dir.joinpath("longley.csv")))

# ------------------------------------------------------------
# Lists
# ------------------------------------------------------------
print("Lists:")

# Create a list containing all variables in the dataset.
longley.new_list(name="dX", value="*")
gretl.print("dX", list=True)

# ------------------------------------------------------------
# Matrices
# ------------------------------------------------------------
print("Matrices:")

# Create a matrix using a hansl formula.
gretl.genr("matrix theta = {1,5}")
gretl.print("theta")

# Create a matrix from a two-dimensional integer NumPy array.
a2I = np.array([[1, 2], [3, 4]])
gretl.genr(name="theta2i", value=a2I)
gretl.print("theta2i")

# Create a matrix from a two-dimensional floating-point NumPy array.
a2D = np.array([[1.1, 2.2], [3.3, 4.4]])
gretl.genr(name="theta2d", value=a2D)
gretl.print("theta2d")

# Create a matrix from a Fortran-contiguous (column-major) array.
a2DF = np.asfortranarray(a2D)
gretl.genr(name="theta2df", value=a2DF)
gretl.print("theta2df")

# ------------------------------------------------------------
# Scalars
# ------------------------------------------------------------
print("Scalars:")

# Create scalar variables from Python values.
gretl.genr(name="b", value=3.7)
gretl.print("b")

gretl.genr(name="a", value=4)
gretl.print("a")

# A scalar can also be created using a hansl formula.
gretl.genr(formula="a2 = 5")
gretl.print("a2")

# ------------------------------------------------------------
# Strings
# ------------------------------------------------------------
print("Strings:")

gretl.genr(name="c", value="hello world!")
gretl.print("c")

# ------------------------------------------------------------
# Bundles
# ------------------------------------------------------------
print("Bundles:")

# Create a bundle using a hansl expression.
gretl.genr("bundle B = _(Nrep=10^6)")
gretl.print("B")

# Retrieve the user-defined variable back into Python.
B = gretl.get_uservar("B")
print(f"type(B): {type(B)}")

# ------------------------------------------------------------
# Inspect user-defined variables
# ------------------------------------------------------------
print("User-defined variables:")

uservar_types = ["scalar", "matrix", "list", "string", "bundle", "array"]

for var_type in uservar_types:
    gretl.varlist(type=var_type)

# ------------------------------------------------------------
# Delete user-defined variables
# ------------------------------------------------------------
print("Deleting variables:")

gretl.delete("a")
gretl.delete("dX")
gretl.delete(type="matrix")

print("Remaining user-defined variables:")
for var_type in uservar_types:
    gretl.varlist(type=var_type)
