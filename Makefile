.PHONY: all clean

all: bin/chan

bin/%.o: src/%.c $(wildcard src/*.h)
	@mkdir -p bin
	gcc $(DEBUG) -Wall -c $< -o $@

bin/chan: $(patsubst src/%.c, bin/%.o, $(wildcard src/*.c))
	@mkdir -p bin
	gcc $(DEBUG) $^ -Wall -lncurses -lcurl -o $@

debug: DEBUG = -g

debug: bin/chan

clean:
	rm -rf bin
