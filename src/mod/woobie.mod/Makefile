# Makefile for src/mod/woobie.mod/

doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../../..; make

clean:
	@rm -f *.o *.so *~

modules: ../../../woobie.so

static: ../woobie.o
	
../../../woobie.so: woobie.o
	$(LD) -o ../../../woobie.so woobie.o
	$(STRIP) ../../../woobie.so

woobie.o: ../module.h ../modvals.h ../../eggdrop.h
	$(CC) $(CFLAGS) -DMAKING_MODS -c woobie.c

../woobie.o: woobie.o
	@rm -f ../woobie.o
	@ln -s woobie.mod/woobie.o ..
	
#safety hash
