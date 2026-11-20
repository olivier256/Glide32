.PHONY: all clean rebuild run

all:
	cmake -S . -B build
	cmake --build build

clean:
	cmake -E rm -rf build

rebuild: clean all

run: all
	./build/pixel