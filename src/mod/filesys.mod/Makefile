# Makefile for src/mod/filesys.mod/

doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../../..; make

clean:
	@rm -f *.o *.so *~

modules: ../../../filesys.so

static: ../filesys.o

filesys.o: ../module.h ../modvals.h ../../eggdrop.h filedb.c files.c \
tclfiles.c files.h filesys.h ../../lang.h ../../tandem.h \
../transfer.mod/transfer.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c filesys.c

../../../filesys.so: filesys.o
	$(LD) -o ../../../filesys.so filesys.o
	$(STRIP) ../../../filesys.so

../filesys.o: filesys.o
	@rm -f ../filesys.o
	@ln -s filesys.mod/filesys.o ..

filesys.h: ../transfer.mod/transfer.h ../../lang.h

#safety hash
