# Makefile for src/mod/share.mod/

doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../../../; make

clean:
	@rm -f *.o *.so *~

static: ../share.o

modules: ../../../share.so

../share.o: share.c ../module.h ../modvals.h ../../eggdrop.h ../../chan.h \
../../users.h ../channels.mod/channels.h ../transfer.mod/transfer.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c share.c
	rm -f ../share.o
	mv share.o ../

../../../share.so: ../share.o
	$(LD) -o ../../../share.so ../share.o
	$(STRIP) ../../../share.so

#safety hash
