# Makefile for src/mod/share.mod/

doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../../..; make

clean:
	@rm -f *.o *.so *~

modules: ../../../share.so

static: ../share.o
	
../../../share.so: share.o
	$(LD) -o ../../../share.so share.o
	$(STRIP) ../../../share.so

share.o: ../module.h ../modvals.h ../../eggdrop.h \
	share.c ../../chan.h ../../users.h \
	../channels.mod/channels.h ../transfer.mod/transfer.h
	$(CC) $(CFLAGS) -DMAKING_MODS -c share.c

../share.o: share.o
	@rm -f ../share.o
	@ln -s share.mod/share.o ..
	
#safety hash
