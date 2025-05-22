import gretl

# to printdefault directory for writing and reading	files
gretl.set('workdir')

# to force libgretl to use the decimal point character
gretl.set('force_decpoint')
gretl.set('force_decpoint', 'on')
gretl.set('force_decpoint')

# to set the column delimiter used when saving data to file in CSV format as tab
gretl.set('csv_delim')
gretl.set('csv_delim', 'tab')
gretl.set('csv_delim')

# to set the horizon for impulse responses and forecast variance decompositions
gretl.set('horizon')
gretl.set('horizon', 16)
gretl.set('horizon')

# to use SVD rather than Cholesky or QR decomposition in least squares calculations
gretl.set('svd')
gretl.set('svd', True)
gretl.set('svd')
