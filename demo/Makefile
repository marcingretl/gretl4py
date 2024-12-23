TESTS = about.py dataset_tools.py estimators.py na_check.py settest.py smpl.py

.PHONY: clean alltests

dataset_tools: dataset_tools.py
	python3 dataset_tools.py

estimators: estimators.py
	python3 estimators.py

arma_speed: speed/genarma22_test.inp speed/arma_speed.py
	gretlcli -b genarma22_test.inp; python3 speed/arma_speed.py

probit_speed: speed/genprobit_test.inp speed/probit_speed.py
	gretlcli -b genprobit_test.inp; python3 speed/probit_speed.py

alltests:
	for f in $(TESTS) ; do python3 $$f ; done

clean:
	rm -f speed/large_arma22.csv speed/large_probit.csv speed/prob_frml.txt *~
