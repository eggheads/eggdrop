# Makefile for src/mod/server.mod/
# $Id: Makefile,v 1.9 2000/03/24 23:00:25 fabian Exp $

srcdir = .


doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../../../ && make

static: ../server.o

modules: ../../../server.so

../server.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c $(srcdir)/server.c
	@rm -f ../server.o
	mv server.o ../

../../../server.so: ../server.o
	$(LD) -o ../../../server.so ../server.o
	$(STRIP) ../../../server.so

depend:
	$(CC) $(CFLAGS) $(CPPFLAGS) -MM $(srcdir)/*.c > .depend

clean:
	@rm -f .depend *.o *.so *~
distclean: clean

#safety hash
cmdsserv.o: .././server.mod/cmdsserv.c
server.o: .././server.mod/server.c ../../../src/mod/module.h \
 ../../../src/main.h ../../../src/lang.h ../../../src/eggdrop.h \
 ../../../src/flags.h ../../../src/proto.h ../../../lush.h \
 ../../../src/misc_file.h ../../../src/cmdt.h ../../../src/tclegg.h \
 ../../../src/tclhash.h ../../../src/chan.h ../../../src/users.h \
 ../../../src/compat/compat.h ../../../src/compat/inet_aton.h \
 ../../../src/compat/snprintf.h ../../../src/compat/memset.h \
 ../../../src/compat/memcpy.h ../../../src/compat/strcasecmp.h \
 ../../../src/mod/modvals.h ../../../src/tandem.h \
 ../server.mod/server.h ../server.mod/servmsg.c \
 ../server.mod/cmdsserv.c ../server.mod/tclserv.c
servmsg.o: .././server.mod/servmsg.c
tclserv.o: .././server.mod/tclserv.c
