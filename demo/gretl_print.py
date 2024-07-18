def gretl_model_print(MODEL):
    '''Function prints libgretl model'''
    DASHEDLINE = "---------------------------------------------------------------"
    if isinstance(MODEL, dict) is not True:
        print("I can print only dictionary!!!\n")
        return -1
    else:
        print("\t\t coefficient\t std. error\t z\t p-value\n  " + DASHEDLINE)
        coeff_names = MODEL["coeff"].keys()
        for variable in coeff_names:
            coeff = MODEL["coeff"][variable]
            sderr = MODEL["sderr"][variable]
            z = coeff/sderr
            pvalue = MODEL["pvalue"][variable]
            print(f'  {variable:s} \t {coeff:9.4f}\t {sderr:9.4f}\t {z:5.3f}\t {pvalue:4.2e}')
        print('')
        print(f'Mean dependent var  {MODEL["ybar"]:9.4f}\t S.D. dependent var  {MODEL["sdy"]:10.4f}')
        print(f'Log-likelihood      {MODEL["lnL"]:9.4f}\t Akaike criterion     {MODEL["criterion"]["aic"]:9.4f}')
        print(f'Schwarz criterion   {MODEL["criterion"]["bic"]:9.4f}\t Hannan-Quinn\t      {MODEL["criterion"]["hqc"]:9.4f}')
        print(MODEL["vcv"])