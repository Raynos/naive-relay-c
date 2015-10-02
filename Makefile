LIBUV_FLAGS=-pthread
CC_FLAGS=-Wall -Werror -Wextra -std=c++11

UV_PATH=$(shell pwd)/deps/libuv
UV_LIB=$(UV_PATH)/out/Debug/libuv.a

BUFFER_READER_PATH=$(shell pwd)/deps/buffer-reader
BUFFER_READER_LIB=$(BUFFER_READER_PATH)/buffer-reader.cc

APP_FILES=
APP_FILES+=naive-relay.cc
APP_FILES+=connection.cc
APP_FILES+=parser.cc
APP_FILES+=lazy-frame.cc

FILES=$(APP_FILES) $(UV_LIB) $(BUFFER_READER_LIB)

all: relay.out

relay.out: main.cc $(FILES)
	c++ $^ -g $(CC_FLAGS) $(LIBUV_FLAGS) -o relay.out

$(UV_LIB):
	cd $(UV_PATH) && \
	test -d ./build/gyp || (mkdir -p ./build && git clone https://chromium.googlesource.com/external/gyp ./build/gyp) && \
	./gyp_uv.py -f make && \
	$(MAKE) -C ./out

run: relay.out
	./relay.out

.PHONY: run
