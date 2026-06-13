import gretl

A = gretl.Matrix(4, 4, gretl.GretlMatrixType.Real, gretl.GretlMatrixFill.Ones)

print(type(A))
A.print()

print(type(A.numpy_view))
print(A.numpy_view)
