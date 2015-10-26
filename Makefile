# Compiler flags
CXX=c++
CXXFLAGS=-Wall -Werror -Wextra -std=c++11 -pedantic -g
LDFLAGS=-pthread
RM=rm -f
CORES=$(shell sysctl -n hw.ncpu 2>/dev/null || nproc || echo 1)

# Third party code
UV_PATH=$(shell pwd)/deps/libuv
UV_LIB=$(UV_PATH)/out/Debug/libuv.a

BUFFER_READER_PATH=$(shell pwd)/deps/buffer-reader
BUFFER_READER_SRC=$(BUFFER_READER_PATH)/buffer-reader.cc
BUFFER_READER_LIB=$(BUFFER_READER_PATH)/buffer-reader.o

LDLIBS=$(BUFFER_READER_LIB) $(UV_LIB)

# My code
SOURCES=$(wildcard *.cc)
OBJECTS=$(SOURCES:.cc=.o)
DEPS=$(OBJECTS:.o=.d)
EXECUTABLE=relay.out

all:; @$(MAKE) _all -j$(CORES)
_all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) $(LDLIBS)
	$(CXX) $(LDFLAGS) $^ -o $@

%.o: %.cc
	$(CXX) -MMD -MF $(patsubst %.o,%.d,$@) $(CXXFLAGS) -c $< -o $@
-include $(DEPS)

$(UV_LIB):
	cd $(UV_PATH) && \
	test -d ./build/gyp || (mkdir -p ./build && git clone https://chromium.googlesource.com/external/gyp ./build/gyp) && \
	./gyp_uv.py -f make && \
	$(MAKE) -C ./out

$(BUFFER_READER_LIB): $(BUFFER_READER_SRC)
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(EXECUTABLE)
	./$(EXECUTABLE) 0.0.0.0 7000 "127.0.0.1:3000" 0

clean:
	$(RM) *.o
	$(RM) $(BUFFER_READER_LIB)
	$(RM) $(EXECUTABLE)

clean_deps:
	make clean
	$(RM) $(UV_LIB)

.PHONY: run clean _all
