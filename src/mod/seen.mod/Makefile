# Makefile for src/mod/seen.mod/

doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../../../; make

clean:
	@rm -f *.o *.so *~

static: ../seen.o

modules: ../../../seen.so

../seen.o: ../module.h ../modvals.h ../../eggdrop.h \
../channels.mod/channels.h ../../chan.h ../../users.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c seen.c
	mv seen.o ../

../../../seen.so: ../seen.o
	$(LD) -o ../../../seen.so ../seen.o
	$(STRIP) ../../../seen.so

#safety hash
