# Makefile for src/mod/filesys.mod/
# $Id: Makefile,v 1.14 2000/07/09 14:10:49 fabian Exp $

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
	$(LD) -o ../../../filesys.$(MOD_EXT) ../filesys.o $(XLIBS)
	$(STRIP) ../../../filesys.$(MOD_EXT)

depend:
	$(CC) $(CFLAGS) $(CPPFLAGS) -MM $(srcdir)/filesys.c > .depend

clean:
	@rm -f .depend *.o *.$(MOD_EXT) *~
distclean: clean

#safety hash
../filesys.o: .././filesys.mod/filesys.c ../../../config.h \
 ../../../src/mod/module.h ../../../src/main.h ../../../src/lang.h \
 ../../../src/eggdrop.h ../../../src/flags.h ../../../src/proto.h \
 ../../../lush.h ../../../src/misc_file.h ../../../src/cmdt.h \
 ../../../src/tclegg.h ../../../src/tclhash.h ../../../src/chan.h \
 ../../../src/users.h ../../../src/compat/compat.h \
 ../../../src/compat/inet_aton.h ../../../src/compat/snprintf.h \
 ../../../src/compat/memset.h ../../../src/compat/memcpy.h \
 ../../../src/compat/strcasecmp.h ../../../src/mod/modvals.h \
 ../../../src/tandem.h ../filesys.mod/filedb3.h \
 ../filesys.mod/filesys.h ../../../src/mod/transfer.mod/transfer.h \
 ../filesys.mod/files.h ../filesys.mod/dbcompat.h \
 ../filesys.mod/filelist.h ../filesys.mod/files.c ../../../src/stat.h \
 ../filesys.mod/filedb3.c ../filesys.mod/tclfiles.c \
 ../filesys.mod/dbcompat.c ../filesys.mod/filelist.c
