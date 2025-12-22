all: fsm.png

.PHONY: fsm

fsm: fsm.cpp
	g++ -o fsm fsm.cpp
	./fsm

fsm.png: fsm log.csv plot_log.py
	python3 plot_log.py log.csv -o fsm.png

.PHONY: build
build:
	mkdir -p build
	cmake -B build
	cmake --build build

clean:
	@rm -f fsm
	@rm -f log.csv
	@rm -f log.png
	@rm -rf build