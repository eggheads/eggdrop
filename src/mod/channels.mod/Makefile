# Makefile for src/mod/channels.mod/

doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../../..; make

clean:
	@rm -f *.o *.so *~

modules: ../../../channels.so

static: ../channels.o

../../../channels.so: channels.o
	$(LD) -o ../../../channels.so channels.o
	$(STRIP) ../../../channels.so

channels.o: ../module.h ../modvals.h ../../eggdrop.h \
	cmdschan.c tclchan.c userchan.c channels.h
	$(CC) $(CFLAGS) -DMAKING_MODS -c channels.c

../channels.o: channels.o
	@rm -f ../channels.o
	@ln -s channels.mod/channels.o ..

#safety hash
