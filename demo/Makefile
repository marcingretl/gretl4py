.PHONY: clean

demo: demo.py
	python3 demo.py

pandas: pandas_test.py
	python3 pandas_test.py

arma_speed: genarma22_test.inp arma_speed.py
	gretlcli -b genarma22_test.inp; python3 arma_speed.py

clean:
	rm -f large_arma22.csv *~
