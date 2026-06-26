import gretl
km = gretl.include("kmeans")
iris = gretl.get_data("iris")

#X = iris.to_matrix([1,2,3,4])
#b = gretl.kmeans(X, 3, gretl.Bundle(init="hw", verbosity=2))
b = iris.kmeans([1,2,3,4], 3, gretl.Bundle(init="hw", verbosity=2))
iris.add_series(b['clustid'], "sphat")
iris.xtab("species sphat")

# experiment using species as initial clusters
#printf "\n"
#matrix C0 = zeros(3, 4)
#C0[1,] = meanc(X[1:50,])
#C0[2,] = meanc(X[50:100,])
#C0[3,] = meanc(X[51:150,])
#bundle b = kmeans(X, C0, _(verbosity=2))
