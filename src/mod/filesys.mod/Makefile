# Source-make
FILENAME=filesys
include ../Makefile.generic

install:
	@echo Installing filesys language file.
	@$(CP1) files.english.lang $(REALDEST)/language

filesys.o: \
   ../module.h \
   filesys.h \
   ../../tandem.h \
   files.h \
   ../../users.h \
   ../../cmdt.h \
   filedb.c \
   files.c \
   tclfiles.c
filesys.h: \
   ../../lang.h \
   ../transfer.mod/transfer.h
	@touch filesys.h
