import gretl
km = gretl.include("kmeans")
iris = gretl.get_data("iris")

b1 = iris.kmeans([1,2,3,4], 3, gretl.Bundle(init="hw", verbosity=2))
iris.add_series(b1['cluster_id'], "sphat1")
iris.xtab("species sphat1")

print(f"\nexperiment using species as initial clusters")
X = iris.to_matrix([1,2,3,4])
C0 = gretl.Matrix(3, 4)
C0.numpy_view[0, :] = X.numpy_view[0:50, :].mean(axis=0)
C0.numpy_view[1, :] = X.numpy_view[49:100, :].mean(axis=0)
C0.numpy_view[2, :] = X.numpy_view[50:150, :].mean(axis=0)
b2 = gretl.kmeans(X, C0, gretl.Bundle(verbosity=2))
iris.add_series(b2['cluster_id'], "sphat2")
iris.xtab("species sphat2")
