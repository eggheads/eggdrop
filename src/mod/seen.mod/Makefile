# Makefile for src/mod/seen.mod/

doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../../..; make

clean:
	@rm -f *.o *.so *~

modules: ../../../seen.so

static: ../seen.o
	
../../../seen.so: seen.o
	$(LD) -o ../../../seen.so seen.o
	$(STRIP) ../../../seen.so

seen.o: ../module.h ../modvals.h ../../eggdrop.h \
	seen.c ../channels.mod/channels.h \
	../../chan.h ../../users.h
	$(CC) $(CFLAGS) -DMAKING_MODS -c seen.c

../seen.o: seen.o
	@rm -f ../seen.o
	@ln -s seen.mod/seen.o ..
	
#safety hash
