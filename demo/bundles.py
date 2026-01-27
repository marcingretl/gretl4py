import gretl
import numpy

a = gretl.Bundle()
b = gretl.Bundle()
print(a)
print(b)

a['hhh'] = 464645
a['ddd'] = "fhfg"
a['xxx'] = "xxx"
print(a)
a.print()

# delete element
a.delete_element('xxx')
a.print()

l = [[1.5, 4.78], [5, -0.6]]
b['mat'] = numpy.asarray(l)
print(b)
b.print()

a['b'] = b
a.print(True) # tree

d = a.to_dict()
print(d)

