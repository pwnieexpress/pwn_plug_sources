OBJ=fang.o
EXE=fang

DEPS=list.h

LIBS=-lbluetooth -lpthread

all: $(EXE)

$(EXE): $(OBJ) $(DEPS)
	cc -o $(EXE) $(OBJ) $(LIBS)

clean:
	rm -f $(EXE) $(OBJ) *~
