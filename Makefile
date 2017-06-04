.PHONY: all clean

all: bin/chan

bin/%.o: src/%.c $(wildcard src/*.h)
	@mkdir -p bin
	gcc $(FLAGS) -Wall -c $< -o $@

bin/chan: $(patsubst src/%.c, bin/%.o, $(wildcard src/*.c))
	@mkdir -p bin
	gcc $(FLAGS) $^ -Wall -lncurses -lcurl -o $@

debug: FLAGS = -g

debug: bin/chan

release: FLAGS = -Os

release: bin/chan
	strip -s -R .comment -R .gnu.version bin/chan

clean:
	rm -rf bin
