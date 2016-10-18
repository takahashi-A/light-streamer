CC            = gcc
CFLAGS        = -O3 -Wall
INCLUDE       = /usr/local/include
LIBDIR        = /usr/local/lib
LIB           = avcodec -lavutil
SRCDIR        = src
PROGRAM       = streamer
DEST          = target
TARGET        = $(DEST)/$(PROGRAM)
SRCS          = $(wildcard $(SRCDIR)/*.c)
OBJS          = $(patsubst %.c,%.o,$(SRCS))


all:$(TARGET)

$(TARGET):$(OBJS)
	$(CC) -I$(INCLUDE) -L$(LIBDIR) -l$(LIB) -o $@ $^

$(OUTDIR)/%.o:%.c
	@if [ ! -e `dirname $@` ]; then mkdir -p `dirname $@`; fi
	$(CC) -o $@ -c $<

clean:
	@rm -f $(DEST)/* $(SRCDIR)/*o

install:$(PROGRAM)
	install -s $(PROGRAM) $(SRCDIR)
