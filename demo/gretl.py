import _gretl
from gretl4py_addons import csv2dict
import gretl4py_classes

def __model_initializer(model_name, *args, **kwargs):
	print()
	return gretl4py_classes.Model(model_name, *args, **kwargs)

def __getattr__(model_name):
	def wrapper(*args, **kwargs):
		return __model_initializer(model_name, *args, **kwargs)

	return wrapper

def about(print=True):
    return _gretl.about(do_print=print)

def set(key, val=None):
    if val is None:
        _gretl.set(key)
    else:
        _gretl.set(key, val)

def get_gretl_data(fname):
	return _gretl.get_gretl_data(fname)

def attach_gretl_data(fname):
	_gretl.attach_gretl_data(fname)

def load_data(data_dict):
	_gretl.load_data(data_dict)

if __name__ == "__main__":
	pass


