TESTS = demo.py demo2.py pandas_test.py dset_test.py

.PHONY: clean tests

demo: demo.py
	python3 demo.py

pandas: pandas_test.py
	python3 pandas_test.py

arma_speed: genarma22_test.inp arma_speed.py
	gretlcli -b genarma22_test.inp; python3 arma_speed.py

probit_speed: genprobit_test.inp probit_speed.py
	gretlcli -b genprobit_test.inp; python3 probit_speed.py

tests:
	for f in $(TESTS) ; do python3 $$f ; done

clean:
	rm -f large_arma22.csv large_probit.csv prob_frml.txt *~
