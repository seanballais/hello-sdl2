.DEFAULT_GOAL := all
.PHONY: all setup_data clean

all: app

app: setup_data hello-sdl2/main.cpp
	g++ hello-sdl2/main.cpp -o bin/hello-sdl2 -lSDL2

setup_data:
	mkdir -p bin/ bin/data/
	cp -R -u -p data/ bin/data

clean:
	rm -rf data/* bin/*
