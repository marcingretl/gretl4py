import gretl

A = gretl.Matrix(4, 4, 5)
print(type(A))
print(A)
print(A.rnameget())
print(A.cnameget())

A_np = A.numpy_view
print(type(A_np))
print(A_np)
A_np[1,2] = -1000
print(A_np)
print(A)

B = gretl.Matrix(1.0, 5.0)
print(B)

C = gretl.Matrix(10, 2, "Uniform")
print(C)

D = gretl.Matrix(2, 3)
D.rnameset(['first', 'second'])
D.cnameset(['one', 'two', 'three'])
print(D)
D.cnameset(list())
print(D)
