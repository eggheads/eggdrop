# Makefile for src/mod/server.mod/

doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../../../; make

clean:
	@rm -f *.o *.so *~

static: ../server.o

modules: ../../../server.so

../server.o: ../module.h ../modvals.h ../../eggdrop.h cmdsserv.c server.c \
servmsg.c server.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c server.c
	rm -f ../server.o
	mv server.o ../

../../../server.so: ../server.o
	$(LD) -o ../../../server.so ../server.o
	$(STRIP) ../../../server.so

#safety hash
