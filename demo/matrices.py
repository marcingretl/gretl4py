import gretl
import numpy as np
import pandas as pd


# ============================================================
# Matrix constructors
# ============================================================

# Empty matrix
A = gretl.Matrix()
print(A)


# Zero-initialized matrices (real / complex)
A = gretl.Matrix(4, 4)
B = gretl.Matrix(4, 4, type="complex")

print(A)
print(B)


# Constant-filled matrices (real / complex)
A = gretl.Matrix(4, 4, 1.5)
B = gretl.Matrix(4, 4, 1.5, "complex")

print(A)
print(B)


# Random matrices
# "normal" = Gaussian distribution
# "u"      = uniform distribution (shortcut)
A = gretl.Matrix(4, 4, "normal")
B = gretl.Matrix(4, 4, "u")

print(A)
print(B)


# ============================================================
# Special constructors
# ============================================================

# Row vector with arithmetic progression: start, end, step
A = gretl.Matrix(1.0, 10.0, 1.5)

# Descending sequence example
B = gretl.Matrix(10.0, 1.0, 2)

print(A)
print(B)


# Identity matrix
A = gretl.Matrix(5)
print(A)


# ============================================================
# Construction from Python list
# ============================================================

lst1 = [1,1]
lst2 = [[1,1],
        [2,2],
        [3,3]]

A = gretl.Matrix(lst1)
B = gretl.Matrix(lst2)

print(A)
print(B)

# ============================================================
# Construction from NumPy
# ============================================================

np1 = np.full((2, 3), 2.4)
np2 = np.full((2, 3), 2.4 + 1j, dtype=np.complex128)

A = gretl.Matrix(np1)
B = gretl.Matrix(np2)

print(np1)
print(A)
print(np2)
print(B)


# ============================================================
# Row/column names
# ============================================================

print("\nRow/column naming")

A.rnameset(["first", "second"])
A.cnameset(["one", "two", "three"])

print(A)

print(f"Row names: {A.rnameget()}")
print(f"Column names: {A.cnameget()}")


# ============================================================
# NumPy / Pandas views (shared memory)
# ============================================================

print("\nViews (shared memory demonstration)")

np1 = A.numpy_view
pd1 = A.pandas_view

np2 = A.to_numpy()      # independent copy
pd2 = A.to_pandas()     # independent copy

print(A)
print(np1)
print(pd1)

# Modify shared views
A[0, 0] = 0
np1[1, 1] = 0
pd1.iloc[0, 2] = 0

print(A)
print(np1)
print(pd1)

# Independent copies (unchanged)
print(np2)
print(pd2)


# ============================================================
# matrices multiplication via '@' operator
# ============================================================

A = gretl.Matrix(4, 4, "normal")
B = gretl.Matrix(4, 4, "normal")
C = gretl.Matrix(4, 4, 2.99, "complex")
print(f'\nproduct of A @ B:\n{A @ B}')
print(f'\nproduct of A @ B.numpy_view:\n{A @ B.numpy_view}')
print(f'\nproduct of A @ C:\n{A @ C}')
