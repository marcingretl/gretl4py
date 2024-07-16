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
            print(f'  {variable:s} \t {coeff:.6f}\t {sderr:.6f}\t {z:.3f}\t {pvalue:.2e}')
        print("\n")
        print(f'Log-likelihood = {MODEL["lnL"]:f}\n')
