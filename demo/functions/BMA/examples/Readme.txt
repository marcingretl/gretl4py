Example scripts (together with datasets as *.gdt
files and results as *.txt files):

1. BMA_sample.inp - very simple example of how to
run BMA with default settings.

2. priorIP.inp - simple example how to set individual
Prior Inclusion Probabilities for variables under consideration.

3. FLS_41t72.inp - replication of classical paper by
Carmen Fernández, Eduardo Ley, Mark J.F. Steel (2001),
"Model Uncertainty in Cross-Country Growth Regressions",
Journal of Applied Econometrics, Vol. 16, No. 5, pp. 563-576
(original data and results can be found at:
http://qed.econ.queensu.ca/jae/2001-v16.5/fernandez-ley-steel).

We run BMA analysis with the following non-default parameters:
  a. number of Monte Carlo draws: 3000000
  b. percentage of burn-in draws: 33
  c. number of top-ranked models: 20
  d. jointness analysis as modified Yule's Q measure

Note: on machine with 4 Intel(R) Core(TM) i7-8550U CPU @1.80GHz
and 16 GB RAM execution takes +/- 101 sec.

4. UKM1.inp - UK M1 model by David F. Hendry in the version
published in Krolzig, Hans-Martin, David F. Hendry (2001),
"Computer automation of general-to-specific model selection
procedures", Journal of Economic Dynamics and Control 25:

We run BMA analysis with the following non-default parameters:
  a. g-prior: RIC
  b. model prior: Binomial and Binomial-Beta
  c. number of out-of-sample forecasts: 16
