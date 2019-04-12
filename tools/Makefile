.PHONY: all clean

all: cdparser fefi

CFLAGS += -g

cdparser: cdparser.o mbrop.o
	$(LINK.o) $^ -o $@

fefi: findefi.o
	$(LINK.o) $^ -o $@

clean:
	rm -f *.o
	rm -f cdparser fefi
