# Makefile for src/mod/notes.mod
# $Id: Makefile,v 1.9 2000/03/24 23:00:25 fabian Exp $

srcdir = .


doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../../../ && make

static: ../notes.o

modules: ../../../notes.so

../notes.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c $(srcdir)/notes.c
	@rm -f ../notes.o
	mv notes.o ../

../../../notes.so: ../notes.o
	$(LD) -o ../../../notes.so ../notes.o
	$(STRIP) ../../../notes.so

depend:
	$(CC) $(CFLAGS) $(CPPFLAGS) -MM $(srcdir)/*.c > .depend

clean:
	@rm -f .depend *.o *.so *~
distclean: clean

#safety hash
cmdsnote.o: .././notes.mod/cmdsnote.c
notes.o: .././notes.mod/notes.c ../../../src/mod/module.h \
 ../../../src/main.h ../../../src/lang.h ../../../src/eggdrop.h \
 ../../../src/flags.h ../../../src/proto.h ../../../lush.h \
 ../../../src/misc_file.h ../../../src/cmdt.h ../../../src/tclegg.h \
 ../../../src/tclhash.h ../../../src/chan.h ../../../src/users.h \
 ../../../src/compat/compat.h ../../../src/compat/inet_aton.h \
 ../../../src/compat/snprintf.h ../../../src/compat/memset.h \
 ../../../src/compat/memcpy.h ../../../src/compat/strcasecmp.h \
 ../../../src/mod/modvals.h ../../../src/tandem.h ../notes.mod/notes.h \
 ../notes.mod/cmdsnote.c
