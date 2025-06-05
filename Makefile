CC = clang
GDB = gf
CRELFLAGS = -std=gnu99 -O2
CDEVFLAGS = -std=gnu99 -g
LDLIBS =

SRC = `find . -path './src/*.c'`
OBJ = `find . -name '*.o'`
EXE = cmlc

all: setrel complink

complink:
	$(CC) -o $(EXE) $(SRC) $(CFLAGS) $(LDLIBS)
comp:
	$(CC) $(SRC) $(CFLAGS) -c
rel: setrel complink
dev: setdev complink
run:
	./$(EXE)
gdb: dev
	$(GDB) ./$(EXE)
memcheck: dev
	valgrind --leak-check=full -s ./$(EXE)
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
