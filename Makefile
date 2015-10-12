# Compiler flags
CXX=c++
CXXFLAGS=-Wall -Werror -Wextra -std=c++11 -pedantic -g
LDFLAGS=-pthread

# Third party code
UV_PATH=$(shell pwd)/deps/libuv
UV_LIB=$(UV_PATH)/out/Debug/libuv.a

BUFFER_READER_PATH=$(shell pwd)/deps/buffer-reader
BUFFER_READER_LIB=$(BUFFER_READER_PATH)/buffer-reader.o

LDLIBS=$(UV_LIB) $(BUFFER_READER_LIB)

# My code
SOURCES=$(filter-out main.cc, $(wildcard *.cc))
EXECUTABLE=relay.out

all: $(EXECUTABLE)

$(EXECUTABLE): main.cc $(SOURCES) $(LDLIBS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ -o $(EXECUTABLE)

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(UV_LIB):
	cd $(UV_PATH) && \
	test -d ./build/gyp || (mkdir -p ./build && git clone https://chromium.googlesource.com/external/gyp ./build/gyp) && \
	./gyp_uv.py -f make && \
	$(MAKE) -C ./out

run: relay.out
	./relay.out

clean:
	rm -f $(EXECUTABLE)
	rm -f *.o
	rm -f $(BUFFER_READER_LIB)

clean_deps:
	rm -f $(UV_LIB)

.PHONY: run clean
