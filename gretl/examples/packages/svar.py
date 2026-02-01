import gretl

def run_example():
  gretl.set('verbose', 'off')
  sv = gretl.include('SVAR')
  sv.show_functions()

  d = gretl.get_data('sw_ch14.gdt')
  d.new_series(name='infl', value='400*ldiff(PUNEW)')
  d.rename('LHUR', 'unemp')
  d.new_list(name='X', value='unemp infl')
  d.new_list(name='Z', value='const')
  
  Mod = gretl.SVAR_setup('C', 'X', 'Z', 3) # equivalent to Mod = gretl.SVAR_setup(type_string='C', lY='X', lX='Z', varorder=3)
  Mod['horizon'] = 36
  gretl.SVAR_restrict(Mod, 'C', 1, 2)      # equivalent to gretl.SVAR_restrict(b=Mod, code='C', rR=1, c=2)
  
  gretl.set('stopwatch')
  gretl.SVAR_estimate(Mod)                 # equivalent to gretl.SVAR_estimate(obj=Mod)
  print(f"Time (Cmodel) = {gretl.get_accessor('$stopwatch'):.2g}")
  
  fevdmat = gretl.FEVD(Mod)                # equivalent to fevdmat = gretl.FEVD(SVARobj=Mod)
  print(fevdmat)
  
  gretl.set('stopwatch')
  bfail = gretl.SVAR_boot(Mod, 1024, 0.90) # equivalent to bfail = gretl.SVAR_boot(obj=Mod, rep=1024, alpha=0.90)
  print(f"Number of failed bootstrap iterations = {int(bfail):d}, "
        f"Time (bootstrap) = {gretl.get_accessor('$stopwatch'):.2g}")
  
if __name__ == "__main__":
    run_example()
