.DEFAULT_GOAL := all
.PHONY: setup_data clean

all: part0

part0: setup_data part0/main.cpp
	g++ part0/main.cpp -o bin/part0 -lSDL2

setup_data:
	mkdir -p bin/ bin/data/
	cp -R -u -p data/ bin/data

clean:
	rm -rf data/* bin/*
