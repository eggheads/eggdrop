# Makefile for src/mod/dns.mod/

doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../../../; make

clean:
	@rm -f *.o *.so *~

static: ../dns.o

modules: ../../../dns.so

../dns.o: ../module.h ../modvals.h ../../eggdrop.h dns.h dns.c coredns.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c dns.c
	mv dns.o ../

../../../dns.so: ../dns.o
	$(LD) -o ../../../dns.so ../dns.o $(RESLIB)
	$(STRIP) ../../../dns.so

#safety hash
