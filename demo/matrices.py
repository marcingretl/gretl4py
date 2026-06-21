import gretl
import numpy as np
import pandas as pd


# ============================================================
# Matrix constructors
# ============================================================

# Empty matrix (uninitialized / 0x0)
A = gretl.Matrix()
print(A)


# Zero-initialized matrices (real / complex)
# NOTE: complex matrices still store zero + 0j
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
# "normal" = Gaussian distribution (mean 0, variance 1)
# "u"      = uniform distribution
A = gretl.Matrix(4, 4, "normal")
B = gretl.Matrix(4, 4, "u")

print(A)
print(B)


# ============================================================
# Special constructors
# ============================================================

# Arithmetic progression vector:
# start, end, step
A = gretl.Matrix(1.0, 10.0, 1.5)

# Descending sequence example
B = gretl.Matrix(10.0, 1.0, 2)

print(A)
print(B)


# Identity matrix (NxN)
A = gretl.Matrix(5)
print(A)


# ============================================================
# Construction from Python list
# ============================================================

lst1 = [1, 1]
lst2 = [[1, 1],
        [2, 2],
        [3, 3]]

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

# Shared memory views (modifications affect A)
np1 = A.numpy_view
pd1 = A.pandas_view

# Independent copies (safe snapshots)
np2 = A.to_numpy()
pd2 = A.to_pandas()

print(A)
print(np1)
print(pd1)

# Modify shared views → A is affected
A[0, 0] = 0
np1[1, 1] = 0
pd1.iloc[0, 2] = 0

print(A)
print(np1)
print(pd1)

# Independent copies remain unchanged
print(np2)
print(pd2)


# ============================================================
# Matrix multiplication (@ = linear algebra product)
# ============================================================

A = gretl.Matrix(4, 4, "normal")
B = gretl.Matrix(4, 4, "normal")

C = gretl.Matrix(4, 4, 2.99, "complex")
C[0, 0] = 0 + 7j
C[1, 1] = 10 + 1j

print(f'\nMatrix multiplication (A @ B):\n{A @ B}')
print(f'\nMatrix multiplication with NumPy view (A @ B.numpy_view):\n{A @ B.numpy_view}')
print(f'\nMatrix multiplication with complex matrix (A @ C):\n{A @ C}')


# ============================================================
# Element-wise arithmetic (libgretl dot semantics)
# ============================================================

print(f'\nElement-wise division (A / B):\n{A / B}')
print(f'\nElement-wise division with complex (A / C):\n{A / C}')

print(f'\nElement-wise addition (A + B):\n{A + B}')
print(f'\nElement-wise addition with NumPy view:\n{A + B.numpy_view}')
print(f'\nElement-wise addition with complex matrix:\n{A + C}')

print(f'\nElement-wise subtraction (A - B):\n{A - B}')
print(f'\nElement-wise subtraction with NumPy view:\n{A - B.numpy_view}')
print(f'\nElement-wise subtraction with complex matrix:\n{A - C}')


# ============================================================
# In-place operations
# ============================================================

A += B
A += B.numpy_view
print(f'\nIn-place addition (A += B + view):\n{A}')

A -= B
A -= B.numpy_view
print(f'\nIn-place subtraction (A -= B + view):\n{A}')


# ============================================================
# Element-wise multiplication (Hadamard product)
# ============================================================
A = gretl.Matrix(4, 4, 3.3)
B = gretl.Matrix(4, 4, 1.1)

print(f'\nElement-wise multiplication (*)')
print(f'\nA * B:\n{A * B}')
print(f'\nA * B.numpy_view:\n{A * B.numpy_view}')
print(f'\nA * C:\n{A * C}')


# ============================================================
# Element-wise power (Hadamard power, NOT matrix power)
# ============================================================

print(f'\nElement-wise power (A ** B):\n{A ** B}')
print(f'\nElement-wise power (A ** B.numpy_view):\n{A ** B.numpy_view}')
print(f'\nElement-wise power (A ** C):\n{A ** C}')


# ============================================================
# Scalar operations
# ============================================================

A = gretl.Matrix(4, 4, 1.0)
B = gretl.Matrix(4, 4, 1.0, "complex")

print(f'\nA + 1:\n{A + 1}')
print(f'\nA - 2:\n{A - 2}')
print(f'\nA * 4:\n{A * 4}')
print(f'\nA / 5:\n{A / 5}')

print(f'\nB + (1 + 1j):\n{B + (1 + 1j)}')
print(f'\nB - (2 + 1j):\n{B - (2 + 1j)}')
print(f'\nB * (1 + 1j):\n{B * (1 + 1j)}')
print(f'\nB / (2 + 1j):\n{B / (2 + 1j)}')

A += 1
print(f'\nA += 1:\n{A}')

A -= 3
print(f'\nA -= 3:\n{A}')

A *= 4
print(f'\nA *= 4:\n{A}')

A /= -5
print(f'\nA /= -5:\n{A}')
