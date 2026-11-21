.PHONY: all clean rebuild run amb96

all:
	cmake -S . -B build
	cmake --build build

clean:
	cmake -E rm -rf build

rebuild: clean all

run: all
	./build/pixel

amb96: all
	./build/amb96
