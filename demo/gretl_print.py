def gretl_model_print(RES):
    '''Function prints libgretl model'''
    DASHEDLINE = "---------------------------------------------------------"
    if isinstance(RES, dict) is not True:
        print("I can print only dictionary!!!\n")
        return -1
    else:
        print("\t\t coefficient\t std. error\n  " + DASHEDLINE)
        coeff_names = RES["coeff"].keys()
        for variable in coeff_names:
            print(f'  {variable:s} \t {RES["coeff"][variable]:f}\t {RES["sderr"][variable]:f}')
        print("\n")
        print(f"Log-likelihood = {RES["lnL"]}\n")
