# Makefile for src/mod/uptime.mod/
# $Id: Makefile,v 1.2 2001/06/12 17:44:42 poptix Exp $
srcdir = .


doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../../../ && make

static: ../uptime.o

modules: ../../../uptime.$(MOD_EXT)

../uptime.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c $(srcdir)/uptime.c
	@rm -f ../uptime.o
	mv uptime.o ../

../../../uptime.$(MOD_EXT): ../uptime.o
	$(LD) -o ../../../uptime.$(MOD_EXT) ../uptime.o
	$(STRIP) ../../../uptime.$(MOD_EXT)

depend:
	$(CC) $(CFLAGS) $(CPPFLAGS) -MM $(srcdir)/uptime.c > .depend

clean:
	@rm -f .depend *.o *.$(MOD_EXT) *~
distclean: clean

#safety hash
../uptime.o: .././uptime.mod/uptime.c ../uptime.mod/uptime.h \
 ../../../src/mod/module.h ../../../src/main.h ../../../config.h \
 ../../../src/lang.h ../../../src/eggdrop.h ../../../src/flags.h \
 ../../../src/proto.h ../../../lush.h ../../../src/misc_file.h \
 ../../../src/cmdt.h ../../../src/tclegg.h ../../../src/tclhash.h \
 ../../../src/chan.h ../../../src/users.h ../../../src/compat/compat.h \
 ../../../src/compat/inet_aton.h ../../../src/compat/snprintf.h \
 ../../../src/compat/memset.h ../../../src/compat/memcpy.h \
 ../../../src/compat/strcasecmp.h ../../../src/compat/strftime.h \
 ../../../src/mod/modvals.h ../../../src/tandem.h \
 ../../../src/mod/server.mod/server.h
