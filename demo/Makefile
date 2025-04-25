TESTS = module_related_tools.py data_management.py estimators.py tests.py accessors.py forecast.py

.PHONY: clean alltests

data_management: data_management.py
	python3 data_management.py

estimators: estimators.py
	python3 estimators.py

tests: tests.py
	python3 tests.py

accessors: accessors.py
	python3 accessors.py

forecast: forecast.py
	python3 forecast.py

arma_speed: speed/genarma22_test.inp speed/arma_speed.py
	gretlcli -b genarma22_test.inp; python3 speed/arma_speed.py

probit_speed: speed/genprobit_test.inp speed/probit_speed.py
	gretlcli -b genprobit_test.inp; python3 speed/probit_speed.py

alltests:
	for f in $(TESTS) ; do python3 $$f ; done

clean:
	rm -f speed/large_arma22.csv speed/large_probit.csv speed/prob_frml.txt *~
