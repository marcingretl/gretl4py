import gretl
import numpy as np
import pandas as pd


# ============================================================
# Matrix constructors
# ============================================================

A = gretl.Matrix()
print(A)

A = gretl.Matrix(4, 4)
B = gretl.Matrix(4, 4, type="complex")

print(A)
print(B)

A = gretl.Matrix(4, 4, 1.5)
B = gretl.Matrix(4, 4, 1.5, "complex")

print(A)
print(B)

A = gretl.Matrix(4, 4, "normal")
B = gretl.Matrix(4, 4, "u")

print(A)
print(B)


# ============================================================
# Special constructors
# ============================================================

A = gretl.Matrix(1.0, 10.0, 1.5)
B = gretl.Matrix(10.0, 1.0, 2)

print(A)
print(B)

A = gretl.Matrix(5)
print(A)


# ============================================================
# From Python list
# ============================================================

A = gretl.Matrix([1, 1])
B = gretl.Matrix([[1, 1], [2, 2], [3, 3]])

print(A)
print(B)


# ============================================================
# From NumPy
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
# Names
# ============================================================

A.rnameset(["first", "second"])
A.cnameset(["one", "two", "three"])

print(A)
print(A.rnameget())
print(A.cnameget())


# ============================================================
# NumPy / Pandas views
# ============================================================

print("\nViews (shared memory)")

np_view = A.numpy_view
pd_view = A.pandas_view

np_copy = A.to_numpy()
pd_copy = A.to_pandas()

print(A)
print(np_view)
print(pd_view)

# mutate shared views
A[0, 0] = 0
np_view[1, 1] = 0
pd_view.iloc[0, 2] = 0

print(A)
print(np_view)
print(pd_view)

print(np_copy)
print(pd_copy)

# WARNING:
# pandas view may silently copy depending on dtype/layout
print("\nWARNING: pandas view may not always be fully writable (implementation-dependent)")


# ============================================================
# Matrix multiplication
# ============================================================

A = gretl.Matrix(4, 4, "normal")
B = gretl.Matrix(4, 4, "normal")

C = gretl.Matrix(4, 4, 2.99, "complex")
C[0, 0] = 7j
C[1, 1] = 10 + 1j

print(A @ B)

# WARNING: NumPy view uses shared memory assumptions
print(A @ B.numpy_view)

print(A @ C)


# ============================================================
# Element-wise operations
# ============================================================

print(A / B)
print(A / C)

print(A + B)
print(A + B.numpy_view)

# NOTE: consider adding explicit NumPy scalar tests
print(A + np.float64(2.0))             # TODO: verify scalar dispatch coverage
print(A + np.complex128(1 + 1j))       # TODO: verify complex scalar dispatch

print(A - B)
print(A - C)


# ============================================================
# Element-wise multiplication
# ============================================================

A = gretl.Matrix(4, 4, 4.5)
B = gretl.Matrix(4, 4, 2.3)

print(A * B)
print(A * B.numpy_view)
print(A * C)


# ============================================================
# Element-wise power
# ============================================================

print(A ** B)
print(A ** B.numpy_view)
print(A ** C)

# NOTE:
# Python ** corresponds to Gretl ".^"
# NOT matrix power (^)

# WARNING:
# Negative powers may produce inf/nan if zeros exist
A_tmp = gretl.Matrix(4, 4, 1.0)
print(A_tmp ** -2)   # TODO: ensure intended semantics


# ============================================================
# IN-PLACE OPERATIONS
# ============================================================

D = gretl.Matrix(4, 4, -2, "complex")
np3 = np.full((4, 4), 2.4 + 1j, dtype=np.complex128)

A += B
A += B.numpy_view
A -= B
A -= B.numpy_view

C += D
C += np3
C -= D
C -= np3

A **= B
A **= B.numpy_view
C **= D
C **= np3


# ============================================================
# Scalar operations
# ============================================================

A = gretl.Matrix(4, 4, 1.0)
B = gretl.Matrix(4, 4, 1.0, "complex")

print(A + 1)
print(1 + A)

print(A - 2)
print(2 - A)

print(A * 4)
print(4 * A)

print(A / 5)
print(5 / A)

print(B + (1 + 1j))
print((1 + 1j) + B)

print(B - (2 + 1j))
print((2 + 1j) - B)

print(B * (1 + 1j))
print((1 + 1j) * B)

print(B / (2 + 1j))
print((2 + 1j) / B)

A += 1
A -= 3
A *= 4
A /= -5

# WARNING: not standard in NumPy semantics for matrices
A **= -2

B += (1 + 0.5j)
B -= (1 + 0.5j)
B *= (2 + 0.5j)
B /= (4 + 1.5j)
B **= (2 + 0.5j)

# ============================================================
# Unary operators
# ============================================================

A = gretl.Matrix(2, 2, 1.5)
C = gretl.Matrix(2, 2, 1.5, "complex")
C[0, 0] = 1 + 2j

print(+A)
print(-A)

print(+C)
print(-C)


# ============================================================
# Comparison operators
# ============================================================

A = gretl.Matrix([1, 2, 3, 4])
B = gretl.Matrix([2, 2, 2, 2])

print(A == B)
print(A != B)

print(A > B)
print(A >= B)

print(A < B)
print(A <= B)

# scalar comparisons

print(A == 2)
print(A != 2)

print(A > 2)
print(A >= 2)

print(A < 2)
print(A <= 2)

# reflected scalar comparisons

print(2 == A)
print(2 != A)

print(2 > A)
print(2 >= A)

print(2 < A)
print(2 <= A)


# NumPy interoperability

npv = np.array([2, 2, 2, 2], dtype=np.float64)

print(A == npv)
print(A != npv)

print(A > npv)
print(A >= npv)

print(A < npv)
print(A <= npv)
