# Makefile for src/mod/ctcp.mod/

doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../../..; make

clean:
	@rm -f *.o *.so *~

modules: ../../../ctcp.so

static: ../ctcp.o

ctcp.o: ../module.h ../modvals.h ../../eggdrop.h ctcp.h \
../server.mod/server.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c ctcp.c

../../../ctcp.so: ctcp.o
	$(LD) -o ../../../ctcp.so ctcp.o
	$(STRIP) ../../../ctcp.so

../ctcp.o: ctcp.o
	@rm -f ../ctcp.o
	@ln -s ctcp.mod/ctcp.o ..

#safety hash
