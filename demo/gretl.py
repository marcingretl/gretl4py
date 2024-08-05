from gretl4py_addons import csv2dict
import gretl4py_classes

def __model_initializer(model_name, *args, **kwargs):
	print()
	return gretl4py_classes.Model(model_name, *args, **kwargs)

def __getattr__(model_name):
	def wrapper(*args, **kwargs):
		return __model_initializer(model_name, *args, **kwargs)

	return wrapper

if __name__ == "__main__":
	pass


