# Makefile for src/mod/server.mod/

doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../../..; make

clean:
	@rm -f *.o *.so *~

modules: ../../../server.so

static: ../server.o

server.o: ../module.h ../modvals.h ../../eggdrop.h cmdsserv.c server.c \
servmsg.c server.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c server.c

../../../server.so: server.o
	$(LD) -o ../../../server.so server.o
	$(STRIP) ../../../server.so

../server.o: server.o
	@rm -f ../server.o
	@ln -s server.mod/server.o ..

#safety hash
