# Makefile for src/mod/seen.mod/
# $Id: Makefile,v 1.8 2000/03/24 23:00:25 fabian Exp $

srcdir = .


doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../../../ && make

static: ../seen.o

modules: ../../../seen.so

../seen.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c $(srcdir)/seen.c
	@rm -f ../seen.o
	mv seen.o ../

../../../seen.so: ../seen.o
	$(LD) -o ../../../seen.so ../seen.o
	$(STRIP) ../../../seen.so

depend:
	$(CC) $(CFLAGS) $(CPPFLAGS) -MM $(srcdir)/*.c > .depend

clean:
	@rm -f .depend *.o *.so *~
distclean: clean

#safety hash
seen.o: .././seen.mod/seen.c ../../../src/mod/module.h \
 ../../../src/main.h ../../../src/lang.h ../../../src/eggdrop.h \
 ../../../src/flags.h ../../../src/proto.h ../../../lush.h \
 ../../../src/misc_file.h ../../../src/cmdt.h ../../../src/tclegg.h \
 ../../../src/tclhash.h ../../../src/chan.h ../../../src/users.h \
 ../../../src/compat/compat.h ../../../src/compat/inet_aton.h \
 ../../../src/compat/snprintf.h ../../../src/compat/memset.h \
 ../../../src/compat/memcpy.h ../../../src/compat/strcasecmp.h \
 ../../../src/mod/modvals.h ../../../src/tandem.h \
 ../../../src/mod/channels.mod/channels.h
