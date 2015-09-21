UV_PATH=$(shell pwd)/deps/libuv
UV_LIB=$(UV_PATH)/out/Debug/libuv.a
LIBUV_FLAGS=-pthread

FILES=
FILES+=naive-relay.cc
FILES+=connection.cc
FILES+=parser.cc

all: relay.out

relay.out: main.cc $(FILES) $(UV_LIB)
	c++ $^ -g -Wall -Werror -Wextra $(LIBUV_FLAGS) -o relay.out

$(UV_LIB):
	cd $(UV_PATH) && \
	test -d ./build/gyp || (mkdir -p ./build && git clone https://chromium.googlesource.com/external/gyp ./build/gyp) && \
	./gyp_uv.py -f make && \
	$(MAKE) -C ./out

run: relay.out
	./relay.out

.PHONY: run
