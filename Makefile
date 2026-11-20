.PHONY: all clean rebuild run bench

all:
	cmake -S . -B build
	cmake --build build

clean:
	cmake -E rm -rf build build-bench

rebuild: clean all

run: all
	./build/pixel

# Mesure : toujours compilé avec optimisations (-O3 -DNDEBUG), dans son
# propre répertoire pour ne pas toucher à la compilation habituelle
bench:
	cmake -S . -B build-bench -DCMAKE_BUILD_TYPE=Release
	cmake --build build-bench --target pixel_bench
	./build-bench/pixel_bench
