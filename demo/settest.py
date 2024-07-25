import sys
sys.path.append("../")
import gretl

gretl.set('csv_delim')
gretl.set('csv_delim', 'tab')
gretl.set('csv_delim')

print()
gretl.set('horizon')
gretl.set('horizon', 16)
gretl.set('horizon')

print()
gretl.set('svd')
gretl.set('svd', True)
gretl.set('svd')
