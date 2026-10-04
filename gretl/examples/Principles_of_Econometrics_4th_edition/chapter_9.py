import math
import gretl

def run_example():
    # check gretl4py version
    assert float(gretl.version) >= 0.41, "required gretl4py 0.41 or higher"

    # switch of verbosity
    gretl.set('verbose', 'off')

    # force libgretl to use the decimal point character
    gretl.set('force_decpoint', 'on')

    # datasets
    DATA_PATH = gretl.gretldir + '/examples/Principles_of_Econometrics_4th_edition/data/'
    okun            = gretl.get_data(DATA_PATH + 'okun.gdt')
    phillips_aus    = gretl.get_data(DATA_PATH + 'phillips_aus.gdt')

    ############################################
    # 9.2.2. Okun's Law
    ############################################
    print("\n9.2.2. Okun\'s Law")
    okun.sample('1985:2', '2009:3')
    okun.diff('u')                                                              # (9.9)
    okun.lags('u', 1)
    okun.lags('g', 3)
    okun.new_series(formula='g_rate = (g - g_1) / g_1 * 100')                   # (9.11)
    print("Table 9.1 Spreadsheet of Observations for Distributed Lag Model")
    okun.print(varlist="u u_1 d_u g g_1 g_2 g_3", range="1:5")
    okun.print(varlist="u u_1 d_u g g_1 g_2 g_3", range="96:98")
    ardl1 = gretl.ols(formula='d_u const g g_1 g_2 g_3', simple=True).fit()
    ardl2 = gretl.ols(formula='d_u const g g_1 g_2', simple=True).fit()
    print(ardl1)
    print(ardl2)

    ############################################
    # 9.3.2. Serial Correlation in Output Growth
    ############################################
    print("\n9.3.2. Serially Correlated Errors")
    okun.corrgm(y='g', order=12, acf_only=True)

    ############################################
    # 9.3.2a. A Phillips Curve
    ############################################
    print("\n9.3.2a. A Phillips Curve")
    phillips_aus.set_as_default()
    phillips_aus.diff('u')
    phillips_aus.summary()
    PC = gretl.ols(formula='inf const d_u', simple=True).fit()  # (9.22)
    print(PC)
    PC.save_uhat(name='PC_e')
    phillips_aus.corrgm(y='PC_e', order=12, acf_only=True)

    ############################################
    # 9.4. Other Tests For Serially Correlated Errors
    ############################################
    print("\n9.4. Other Tests For Serially Correlated Errors")
    print("\n9.4.1. A Lagrange Multiplier")
    PC_e_2 = phillips_aus.get_series("PC_e")
    PC_e_2[0] = 0
    phillips_aus.add_series(name='PC_e_2', data=PC_e_2)
    PC_aux_1 = gretl.ols('PC_e const d_u PC_e(-1)').fit()
    PC_aux_2 = gretl.ols('PC_e_2 const d_u PC_e_2(-1)').fit()
    t_i  = PC_aux_1.coeff[2,0]/PC_aux_1.stderr[2,0]
    t_ii = PC_aux_2.coeff[2,0]/PC_aux_2.stderr[2,0]
    print(f"(i)\tt = {t_i:.3f}, p-value: {gretl.pvalue(c='t', x=t_i, params=PC_aux_1.df):.3f}")
    print(f"(ii)\tt = {t_ii:.3f}, p-value: {gretl.pvalue(c='t', x=t_ii, params=PC_aux_2.df):.3f}")
    print(f"(iii)\tLM = {PC_aux_1.trsq:.2f}")
    print(f"(iv)\tLM = {PC_aux_2.trsq:.2f}")
    print(f"chi^2(1) critical value: {gretl.critical(c='X', p=0.05, params=1):.2f}")

    ############################################
    # 9.5. Estimation With Serially Correlated Errors
    ############################################
    print("\n9.5. Estimation With Serially Correlated Errors")
    print("\n9.5.1. Least Squares Estimation")
    PC_hac = gretl.ols(formula='inf const d_u',
        robust=True, simple=True).fit()                         # (9.29)
    print(PC_hac)
    gretl.modeltab([PC, PC_hac])

    print("\n9.5.3. Estimating a More General Model")
    PC_ar1_1 = gretl.ols(formula='inf const inf(-1) d_u(0 to -1)',
        simple=True).fit()                                      # (9.49)
    print(PC_ar1_1)

    PC_ar1_2 = gretl.ols(formula='inf const inf(-1) d_u',
        simple=True).fit()                                      # (9.51)
    print(PC_ar1_2)

    ############################################
    # 9.6. Autoregressive Distributed Lag Models
    ############################################
    print("\n9.6. Autoregressive Distributed Lag Models")
    PC_ARDL_1_1 = gretl.ols(formula='inf const inf(-1) d_u(0 to -1)').fit()
    PC_ARDL_1_0 = gretl.ols(formula='inf const inf(-1) d_u').fit()
    gretl.modeltab([PC_ARDL_1_1, PC_ARDL_1_0])

    print("\n9.6.1. Phillips Curve")
    PC_ARDL_1_0 = gretl.ols(formula='inf const inf(-1) d_u', simple=True).fit()         # (9.56)
    print(PC_ARDL_1_0)
    PC_ARDL_1_0.save_uhat(name='PC_ARDL_1_0_e')
    phillips_aus.corrgm(y='PC_ARDL_1_0_e', order=12, acf_only=True)
    PC_ARDL_4_1 = gretl.ols(formula='inf const inf(-1 to -4) d_u').fit()                # (9.57)
    print(PC_ARDL_4_1)
    print("\nTable 9.4 AIC and SC Values for Philips Curve ARDL Models")
    phillips_aus.sample('1988:3', '2009:3')
    for q in range(0,2):
        for p in range(1,7):
            spec = f'inf const inf(-1 to -{p}) d_u(0 to -{q})'
            m = gretl.ols(formula=spec, simple=True).fit()
            aic = m.aic/m.T - (1 + math.log(2*math.pi))
            sc = m.bic/m.T - (1 + math.log(2*math.pi))
            print(p, q, f'{aic:.4}', f'{sc:.4}')

    print("\n9.6.2. Okun's Law")
    okun.set_as_default()
    OL_ARDL_0_2 = gretl.ols(formula='d_u const g(0 to -2)', simple=True).fit()          # (9.58)
    print(OL_ARDL_0_2)
    OL_ARDL_1_1 = gretl.ols(formula='d_u const d_u(-1) g(0 to -1)', simple=True).fit()  # (9.59)
    print(OL_ARDL_1_1)
    print("\nTable 9.5 AIC and SC Values for Okun's Law ARDL Models")
    okun.sample('1986:1', '2009:3')
    for p in range(0,3):
        for q in range(1,4):
            if p == 0:
                spec = f'd_u const g(0 to -{q})'
            else:
                spec = f'd_u const d_u(-1 to -{p}) g(0 to -{q})'
            m = gretl.ols(formula=spec).fit()
            aic = m.aic / m.T - (1 + math.log(2*math.pi))
            sc = m.bic / m.T - (1 + math.log(2*math.pi))
            print(p, q, f'{aic:.4}', f'{sc:.4}')

    print("\n9.6.3. Autoregressive Models")
    okun.sample('1985:2', '2009:3')
    OL_AR_2 = gretl.ols(formula='g const g(-1 to -2)', simple=True).fit()               # (9.61)
    print(OL_AR_2)

if __name__ == "__main__":
    run_example()
