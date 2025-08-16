import gretl
import numpy

a = gretl.Bundle()
b = gretl.Bundle()
print(a)
print(b)

a['hhh'] = 464645
a['ddd'] = "fhfg"
print(a)

l = [[1.5, 4.78], [5, -0.6]]
b['mat'] = numpy.asarray(l)
print(b)

a['b'] = b
print(a)

d = a.to_dict()
print(d)

