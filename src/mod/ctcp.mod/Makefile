# Makefile for src/mod/ctcp.mod/

doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../../../; make

clean:
	@rm -f *.o *.so *~

static: ../ctcp.o

modules: ../../../ctcp.so

../ctcp.o: ctcp.c ctcp.h ../module.h ../modvals.h ../../eggdrop.h \
../server.mod/server.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c ctcp.c
	rm -f ../ctcp.o
	mv ctcp.o ../

../../../ctcp.so: ../ctcp.o
	$(LD) -o ../../../ctcp.so ../ctcp.o
	$(STRIP) ../../../ctcp.so

#safety hash
