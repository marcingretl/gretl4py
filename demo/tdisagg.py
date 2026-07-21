### Traditional Chow-Lin: y is a series with repetition
### and X is a list of series. This corresponds to case 4(a)
### as described in section 9.3 of the documentation above.
###
import gretl

#Note: 'tdisagg.gdt' was created in gretl as:
#open fedstl.bin
#data indpro payems
#data gdpc1

d = gretl.get_data("tdisagg.gdt")
d.sample(no_missing=True)

x = 'indpro payems'
RES = gretl.Bundle()

d.add_series(d.tdisagg(Y="gdpc1", X=x, s=3, opts=gretl.Bundle(verbose=1, aggtype="sum"), results=RES), "gdpm")
print(RES)
print(d)
