# Makefile for src/mod/filesys.mod/

doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../../../; make

clean:
	@rm -f *.o *.so *~

static: ../filesys.o

modules: ../../../filesys.so

../filesys.o: ../module.h ../modvals.h ../../eggdrop.h filedb.c \
files.c tclfiles.c files.h filesys.h ../../lang.h ../../tandem.h \
../transfer.mod/transfer.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c filesys.c
	mv filesys.o ../

../../../filesys.so: ../filesys.o
	$(LD) -o ../../../filesys.so ../filesys.o
	$(STRIP) ../../../filesys.so

filesys.h: ../transfer.mod/transfer.h ../../lang.h

#safety hash
