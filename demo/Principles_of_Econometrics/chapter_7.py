import gretl

# switch of verbosity
gretl.set('verbose', 'off')

# force libgretl to use the decimal point character
gretl.set('force_decpoint', 'on')

# datasets
utown = gretl.get_data('data/utown.gdt')
njmin3 = gretl.get_data('data/njmin3.gdt')


############################################
# 7.1.3: The University Effect on House Prices
############################################
print('\n\n7.1.3: The University Effect on House Prices')
utown.new_series(name='sqft_utown', value='sqft * utown')
m_price = gretl.ols(formula='price const utown sqft sqft_utown age pool fplace').fit()
print(m_price)



############################################
# 7.5.7: Using Panel Data
############################################
print('\n\n7.5.7: Using Panel Data')
njmin3.set_as_default()
njmin3.sample(restriction='!missing(demp)')
m_demp = gretl.ols(formula='demp const nj').fit()
print(m_demp)
