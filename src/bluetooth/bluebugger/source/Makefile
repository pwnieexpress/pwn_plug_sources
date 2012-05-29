CC 		= gcc
FLAGS 		= -O2
D_FLAGS         = -g -Wall # -pedantic #-DDEBUG
LIBS		= -lbluetooth
PROG		= bluebugger
SOURCES		= bb.c bt.c at.c wrap.c debug.c

.PHONY:		default clean
.c.o:
		${CC} ${FLAGS} ${D_FLAGS} -c $< 

default:	${SOURCES}
		${CC} ${FLAGS} ${D_FLAGS} -o ${PROG} ${SOURCES} ${LIBS}

clean:
		rm -f ${PROG} *.o *~

