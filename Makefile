.PHONY: all clean

all: bin/chan

bin/%.o: src/%.c
	@mkdir -p bin
	gcc -Wall -c $< -o $@

bin/chan: $(patsubst src/%.c, bin/%.o, $(wildcard src/*.c))
	@mkdir -p bin
	gcc $^ -Wall -lncurses -lcurl -o $@

clean:
	rm -rf bin
