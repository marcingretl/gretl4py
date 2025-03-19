import gretl

gretl.attach_gretl_data(
	gretl.gretl_data_path('misc/longley.gdt'))
gretl.print_data()

# if we want to print only names of variables, we should call:
gretl.print_data(namesonly=True)

# if we want to print provided dataset (not global), we the minimal
# code snipet would be:
longley = gretl.get_gretl_data(
    gretl.gretl_data_path('misc/longley.gdt'))    
gretl.print_data(data=longley, namesonly=True)
