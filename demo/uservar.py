import importlib.resources as resources
import gretl
import numpy as np

np.set_printoptions(precision=4)

# force libgretl to use the decimal point character
gretl.set('force_decpoint', 'on')

# get path object for the data directory inside the gretl package
data_dir = resources.files('gretl').joinpath('data')

# load datasets
longley = gretl.get_data(str(data_dir.joinpath('longley.csv')))

#####################################
# lists                            #
#####################################
print('lists:')
longley.new_list(name='dX', value='*')
gretl.uservar_print("dX")

#####################################
# matrix                          #
#####################################
print('matrix-es:')
gretl.genr('matrix theta = {1,5}')
gretl.uservar_print("theta")

# int NumPy
a2I = np.array([[1, 2], [3, 4]])
gretl.genr(name='theta2i', value=a2I)
gretl.uservar_print("theta2i")

# double NumPy
a2D = np.array([[1.1, 2.2], [3.3, 4.4]])
gretl.genr(name='theta2d', value=a2D)
gretl.uservar_print("theta2d")

# Fortran-contiguous (column-major) NumPy
a2DC = np.asfortranarray(a2D)
gretl.genr(name='theta2dc', value=a2DC)
gretl.uservar_print("theta2dc")

# scalar
gretl.genr(name='b', value=3.7)
gretl.uservar_print("b")

# int
gretl.genr(name='a', value=4)
gretl.uservar_print("a")

gretl.genr(formula="a2 = 5")
gretl.uservar_print("a2")

# str
gretl.genr(name='c', value="hello world!!!")
gretl.uservar_print("c")

# list
#gretl.genr(name='d', value=[1,2])
#gretl.uservar_print("d")

# crash: 3D array
#a3D = np.array([[[1, 2], [3, 4]], [[5, 6], [7, 8]]])
#gretl.genr(name='theta3', value=a3D)

gretl.uservars_list()

#m1 = gretl.package(name="BMA", fname="BMA")
m2 = gretl.package(name="/home/marcin/.gretl/functions/BMA/BMA.gfn", fname="BMA")
m2.fit()
print(type(m2))
#m2.attrs()
