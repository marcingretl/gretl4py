import gretl

gretl.attach_gretl_data(
	gretl.gretl_data_path('misc/longley.gdt'))
gretl.printdata()

# if we want to print only names of variables, we should call:
gretl.printdata(namesonly=True)

# if we want to print provided dataset (not global), we the minimal
# code snipet would be:
longley = gretl.get_gretl_data(
    gretl.gretl_data_path('misc/longley.gdt'))    
gretl.printdata(data=longley, namesonly=True)
