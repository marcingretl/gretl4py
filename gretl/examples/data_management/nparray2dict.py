import gretl
from numpy import genfromtxt

# Load CSV data as numpy array, skipping header and first column (assuming index)
np = genfromtxt('longley.csv', delimiter=',')[1:, 1:]

# Convert numpy array to dict of series with gretl helper
d = gretl.nparray2dict(np)

# Print keys of resulting dictionary
print(d.keys())
