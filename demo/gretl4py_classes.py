import _gretl # by I'd like to have 'import gretl' here

class Model:

	def __init__(self, name, *args, **kwargs):
		self.name = name
		try:
			self.cython_function = getattr(_gretl, name)
		except Exception as err:
			print(f"Unexpected {err=}, {type(err)=}")
			raise

		self.model_args = args
		self.model_kwargs = kwargs
		self.computed = False

	def __repr__(self):
		if self.computed:
			out = self._gretlprn
		else:
			out = '\n'
			out += "Model " + self.name.upper()
		return out

	def __load_dict_model(self, dict_model):
		if dict_model:
			for _ in dict_model: setattr(self, _, dict_model[_])

	def fit(self):
		if self.cython_function:
			model_result = self.cython_function(*self.model_args, **self.model_kwargs)
			self.__load_dict_model(model_result)

		self.computed = True


if __name__ == "__main__":
	pass
