PROG	 = sdl2trs

SRCS	+= blit.c
SRCS	+= debug.c
SRCS	+= dis.c
SRCS	+= error.c
SRCS	+= load_cmd.c
SRCS	+= load_hex.c
SRCS	+= main.c
SRCS	+= trs_cassette.c
SRCS	+= trs_disk.c
SRCS	+= trs_hard.c
SRCS	+= trs_imp_exp.c
SRCS	+= trs_interrupt.c
SRCS	+= trs_io.c
SRCS	+= trs_memory.c
SRCS	+= trs_mkdisk.c
SRCS	+= trs_printer.c
SRCS	+= trs_sdl_gui.c
SRCS	+= trs_sdl_interface.c
SRCS	+= trs_sdl_keyboard.c
SRCS	+= trs_state_save.c
SRCS	+= trs_stringy.c
SRCS	+= trs_uart.c
SRCS	+= z80.c
SRCS	+= PasteManager.c

OBJS	 = ${SRCS:.c=.o}

ifeq (${OS},Windows_NT)
	MACROS	:= -DWIN32
endif

.PHONY: all bsd clean clean-win sdl2 win32 win64

all:
	@echo "make (bsd|clean|clean-win|depend|sdl2|win32|win64)"

bsd:
	make -f BSDmakefile

clean:
	rm -f ${OBJS} ${PROG}

clean-win:
	del *.o sdl2trs.exe sdl2trs64.exe

depend:
	makedepend -Y -- ${CFLAGS} -- ${SRCS} 2>&1 | \
		(egrep -v 'cannot find|not in' || true)

sdl2:	ENDIAN	 = $(shell echo "ab" | od -x | grep "6261" > /dev/null || echo "-Dbig_endian")
sdl2:	INCS	?= $(shell sdl2-config --cflags)
sdl2:	LIBS	?= $(shell sdl2-config --libs)
sdl2:	READLINE?= -DREADLINE
sdl2:	${PROG}

win32:	MINGW	?= \MinGW
win32:	CC	 = ${MINGW}\bin\gcc.exe
win32:	INCS	?= -I${MINGW}\include\SDL2
win32:	LIBS	?= -L${MINGW}\lib -lmingw32 -lSDL2main -lSDL2
win32:	${PROG}

win64:	MINGW64	?= \MinGW64
win64:	CC	 = ${MINGW64}\bin\gcc.exe
win64:	INCS	?= -I${MINGW64}\include\SDL2
win64:	LIBS	?= -L${MINGW64}\lib -lmingw32 -lSDL2main -lSDL2
win64:	PROG	 = sdl2trs64
win64:	${PROG}

READLINELIBS	 =$(if ${READLINE},-lreadline,)
ZBX		?= -DZBX
CFLAGS		?= -g -O2 -Wall
CFLAGS		+= ${INCS} ${ENDIAN} ${MACROS} ${READLINE} ${ZBX}

${PROG}: ${OBJS}
	${CC} -o ${PROG} ${OBJS} ${LIBS} ${LDFLAGS} ${READLINELIBS}
