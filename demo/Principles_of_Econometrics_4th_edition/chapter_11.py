import gretl

# switch of verbosity
gretl.set('verbose', 'off')

# force libgretl to use the decimal point character
gretl.set('force_decpoint', 'on')

# datasets
truffles = gretl.get_data('data/truffles.gdt')
fultonfish = gretl.get_data('data/fultonfish.gdt')

############################################
# 11.6 An Example of Two-Stage Least Squares
############################################
print("\n11.6. Aa Example of Two-Stage Least Squares")
print("\n11.6.2. The Reduced-form Equations")
print("Table 11.1 Representative Truffle Data")
truffles.head()
gretl.summary(simple=True)
truffles.new_list(name="Z", value="const ps di pf")
truffles_q_reduced = gretl.ols("q Z", simple=True).fit()
truffles_p_reduced = gretl.ols("p Z", simple=True).fit()
print("Table 11.2a Reduced Form for Quantity for Truffle (d)")
print(truffles_q_reduced)
print("Table 11.2b Reduced Form for Price for Truffle (p)")
print(truffles_p_reduced)

print("\n11.6.3. The Structural Equations")
truffles_q_structural = gretl.tsls("q const p ps di ; Z").fit()    # (11.11)
truffles_p_structural = gretl.tsls("q const p pf ; Z").fit()       # (11.12)
print("Table 11.3a 2SLS Estimates for Truffle Demand")
print(truffles_q_structural)
print("Table 11.3b 2SLS Estimates for Truffle Supply")
print(truffles_p_structural)

############################################
# 11.7 Supply and Demand at the Fulton Fish Market
############################################
print("\n11.7. Supply and Demand at the Fulton Fish Market")
fultonfish.set_as_default()
print("\n11.7.2. The Reduced-Form Equations")
fultonfish.new_list(name="DAYS", value="mon tue wed thu")
fultonfish.new_list(name="Z", value="const stormy DAYS")
fultonfish_quan_reduced = gretl.ols("lquan Z", simple=True).fit()     # (11.15)
fultonfish_price_reduced = gretl.ols("lprice Z", simple=True).fit()   # (11.16)
print("Table 11.4a/b Reduced Form for ln(Quantity)/ln(Price) Fish")
gretl.modeltab([fultonfish_quan_reduced, fultonfish_price_reduced])

print("\n11.7.3. Two-Stage Least Squares Estimation of Fish Demand")
fultonfish_quan_structural = gretl.tsls("lquan const lprice DAYS ; Z").fit()  # (11.13)
print(fultonfish_quan_structural)
