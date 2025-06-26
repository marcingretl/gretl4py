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
gretl.new_matrix('theta = {1,5}')
gretl.uservar_print("theta")

# int NumPy
a2I = np.array([[1, 2], [3, 4]])
gretl.new_matrix(name='theta2i', nparray=a2I)
gretl.uservar_print("theta2i")

# double NumPy
a2D = np.array([[1.1, 2.2], [3.3, 4.4]])
gretl.new_matrix(name='theta2d', nparray=a2D)
gretl.uservar_print("theta2d")

# Fortran-contiguous (column-major) NumPy
a2DC = np.asfortranarray(a2D)
gretl.new_matrix(name='theta2dc', nparray=a2DC)
gretl.uservar_print("theta2dc")


# crash: 3D array
#a3D = np.array([[[1, 2], [3, 4]], [[5, 6], [7, 8]]])
#gretl.new_matrix(name='theta3', nparray=a3D)

gretl.uservars_list()
