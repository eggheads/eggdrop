# Makefile for src/md5/

OBJS = md5c.o
HDRS = global.h md5.h

doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../..; make

clean:
	@rm -f *.o *~

$(OBJS): $(HDRS)

md5: $(OBJS)

.SUFFIXES:
.SUFFIXES: .c .o .h

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $<

#safety hash
