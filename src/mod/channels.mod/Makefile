# Makefile for src/mod/channels.mod/

doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../../../; make

static: ../channels.o

modules: ../../../channels.so

../channels.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c channels.c
	rm -f ../channels.o
	mv channels.o ../

../../../channels.so: ../channels.o
	$(LD) -o ../../../channels.so ../channels.o
	$(STRIP) ../../../channels.so

depend:
	$(CC) $(CFLAGS) $(CPPFLAGS) -MM *.c > .depend

clean:
	@rm -f .depend *.o *.so *~

#safety hash
../channels.o: channels.c ../module.h ../../../config.h ../../main.h \
 ../../lang.h ../../eggdrop.h ../../flags.h ../../proto.h \
 ../../../lush.h ../../cmdt.h ../../tclegg.h ../../tclhash.h \
 ../../chan.h ../../users.h ../modvals.h ../../tandem.h channels.h \
 cmdschan.c tclchan.c userchan.c
