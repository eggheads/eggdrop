# Makefile for src/mod/filesys.mod/

srcdir = .


doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../../../ && make

static: ../filesys.o

modules: ../../../filesys.$(MOD_EXT)

../filesys.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c $(srcdir)/filesys.c
	@rm -f ../filesys.o
	mv filesys.o ../

../../../filesys.$(MOD_EXT): ../filesys.o
	$(LD) -o ../../../filesys.$(MOD_EXT) ../filesys.o $(XLIBS) $(MODULE_XLIBS)
	$(STRIP) ../../../filesys.$(MOD_EXT)

depend:
	$(CC) $(CFLAGS) -MM $(srcdir)/filesys.c -MT ../filesys.o > .depend

clean:
	@rm -f .depend *.o *.$(MOD_EXT) *~

distclean: clean

#safety hash
../filesys.o: .././filesys.mod/filesys.c ../../../src/mod/module.h \
 ../../../src/main.h ../../../config.h ../../../eggint.h ../../../lush.h \
 ../../../src/lang.h ../../../src/eggdrop.h ../../../src/compat/in6.h \
 ../../../src/flags.h ../../../src/cmdt.h ../../../src/tclegg.h \
 ../../../src/tclhash.h ../../../src/chan.h ../../../src/users.h \
 ../../../src/compat/compat.h ../../../src/compat/inet_aton.h \
 ../../../src/compat/snprintf.h ../../../src/compat/memset.h \
 ../../../src/compat/memcpy.h ../../../src/compat/strcasecmp.h \
 ../../../src/compat/strftime.h ../../../src/compat/inet_ntop.h \
 ../../../src/compat/inet_pton.h ../../../src/compat/gethostbyname2.h \
 ../../../src/mod/modvals.h ../../../src/tandem.h \
 .././filesys.mod/filedb3.h .././filesys.mod/filesys.h \
 ../../../src/lang.h ../../../src/mod/transfer.mod/transfer.h \
 .././filesys.mod/files.h .././filesys.mod/dbcompat.h \
 .././filesys.mod/filelist.h .././filesys.mod/files.c ../../../src/stat.h \
 .././filesys.mod/filedb3.c .././filesys.mod/tclfiles.c \
 .././filesys.mod/dbcompat.c .././filesys.mod/filelist.c
