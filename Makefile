.PHONY: cmake 
cmake:
	mkdir -p ./build
	cd ./build && cmake ..

.PHONY: build
build: cmake
	cd ./build && make
