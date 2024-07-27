class Model:
	
	def __init__(self, dictModel):
		if dictModel:
			for _ in dictModel: setattr(self, _, dictModel[_])
			
	def __repr__(self):
		out = '\n'
		'''Function prints libgretl model'''
		DASHEDLINE = "---------------------------------------------------------------\n"
		ratio_type = 'z-score' if self._zscore == True else 't-ratio'
		out += f'\t\tcoefficient\t std. error\t{ratio_type:s}\t p-value\n'
		out += ('  ' + DASHEDLINE)
		coeff_names = self.coeff.keys()
		for variable in coeff_names:
			coeff = self.coeff[variable]
			sderr = self.sderr[variable]
			ratio = coeff/sderr
			pvalue = self.pvalue[variable]
			out += (f'  {variable:s} \t {coeff:9.4f}\t {sderr:9.4f}\t {ratio:5.3f}\t {pvalue:4.2e}\n')
		out += ('\n')
		out += (f'Mean dependent var  {self.ybar:9.4f}\t S.D. dependent var  {self.sdy:10.4f}\n')
		out += (f'Log-likelihood      {self.lnL:9.4f}\t Akaike criterion     {self.criterion["aic"]:9.4f}\n')
		out += (f'Schwarz criterion   {self.criterion["bic"]:9.4f}\t Hannan-Quinn\t      {self.criterion["hqc"]:9.4f}\n')
		out += ('\nvcv matrix\n')
		out += (str(self.vcv) + '\n')
		out += ('\n')
		
		return out
		
		
if __name__ == "__main__":
	pass
