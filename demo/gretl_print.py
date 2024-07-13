def gretl_model_print(MODEL):
    '''Function prints libgretl model'''
    DASHEDLINE = "---------------------------------------------------------"
    if isinstance(MODEL, dict) is not True:
        print("I can print only dictionary!!!\n")
        return -1
    else:
        print("\t\t coefficient\t std. error\n  " + DASHEDLINE)
        coeff_names = MODEL["coeff"].keys()
        for variable in coeff_names:
            print(f'  {variable:s} \t {MODEL["coeff"][variable]:f}\t {MODEL["sderr"][variable]:f}')
        print("\n")
        print(f'Log-likelihood = {MODEL["lnL"]:f}\n')
