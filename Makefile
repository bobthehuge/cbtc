CC = clang
GDB = gf
CRELFLAGS = -std=gnu99 -O2
CDEVFLAGS = -std=gnu99 -g
LDLIBS =

SRC = `find . -path './src/*.c' -not -path './src/cbtbase/*'`
OBJ = `find . -name '*.o'`
EXE = cbtc

all: setrel complink

complink:
	$(CC) -o $(EXE) $(SRC) $(CFLAGS) $(LDLIBS)
comp:
	$(CC) $(SRC) $(CFLAGS) -c
rel: setrel complink
dev: setdev complink
run:
	./$(EXE) $(ARGS)
gdb: dev
	$(GDB) --args ./$(EXE) $(ARGS)
memcheck: dev
	valgrind --leak-check=full -s ./$(EXE) $(ARGS)
setdev:
	$(eval CFLAGS := $(CDEVFLAGS))
setrel:
	$(eval CFLAGS := $(CRELFLAGS))

.PHONY: setdev setrel debug mop clean

mop:
	$(RM) $(OBJ)
	$(RM) $(SSA)
	$(RM) $(ASM)

clean: mop
	$(RM) $(EXE)
