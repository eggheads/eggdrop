# Generated automatically from Makefile.in by configure.
#
#  This is the Makefile for EGGDROP (the irc bot)
#  You should never need to edit this.
#

prefix = ${HOME}/eggdrop
DEST=${prefix}
NEWVERSION=`grep 'char egg_version' src/main.c | awk '{print $4}' | sed s/\"//g | sed s/\;// | sed s/.\*=\ //`

SHELL=/bin/sh

# things you can put here:
#   -Wall            if you're using gcc and it supports it (configure
#                      usually detects this anyway now)
#   -DEBUG_MEM       to be able to debug memory allocation (.debug)
# this can now be set by using 'make debugmem'
CFLGS = -DRFC_COMPLIANT

# configure SHOULD set these...you may need to tweak them to get modules
# to compile .. if you do...let the devel-team know the working settings
# btw to turn STRIP off, do STRIP=touch not STRIP=

CC=gcc
LD=gcc

#making eggmod
MOD_CC = gcc
MOD_LD = gcc
MOD_STRIP = strip
DLFLAGS= 

#making modules
SHLIB_CC = gcc
SHLIB_LD  = gcc -shared -nostartfiles
SHLIB_STRIP = strip

# configure SHOULD set these properly if not, modify them appropriately
# these are the cp commands that make install uses
CP1 = cp -f
CP2 = cp -rf
CP3 = cp -pf

# With any luck this won't burst into flames or cause sterility
LN_S = ln -s

# STOP STOP STOP STOP STOP STOP STOP STOP STOP STOP STOP STOP STOP STOP #
# - - - - - - - - do not edit anything below this line. - - - - - - - - #
# - - - - - - - - -  it's all done by configure now.  - - - - - - - - - #
# STOP STOP STOP STOP STOP STOP STOP STOP STOP STOP STOP STOP STOP STOP #


# stuff for Tcl
XREQ = /lib/libtcl.so
TCLLIB = /lib
CFLAGS = -c -g -O2 -Wall -I.. -DHAVE_CONFIG_H ${CFLGS}


all:    eggdrop

help:
	@echo "To make eggdrop :"
	@echo "   % make "

clean: cleanmods
	@rm -f eggdrop egg core *.o *.a *.so *~ *.stamp
	@cd src ; rm -f eggdrop egg core *.o *.a *~ language/*~
	
cleanmods:
	@cd src/mod ; ${MAKE} clean 'MAKE=${MAKE}'

dist: clean
	@rm -f Makefile config.cache config.log config.status config.h lush.h
	@rm -f doc/*~
	@echo "all:" >Makefile
	@echo "	@./configure; make" >>Makefile

eggdrop: modegg modules

debugmem: memegg debmodules

eggdrop.h:
	@echo You do not have the eggdrop source!
	@exit 1

OBJS =  botcmd.o botmsg.o botnet.o chanprog.o cmds.o \
	dcc.o dccutil.o flags.o language.o main.o mem.o misc.o \
	modules.o net.o rfc1459.o \
	tcl.o tcldcc.o tclhash.o tclmisc.o tcluser.o userent.o \
	userrec.o users.o

GMAKE_STATIC = ${MAKE} 'CC=${CC}' 'LD=${LD}' 'OBJS=${OBJS}'\
'STRIP=${MOD_STRIP}' 'CFLAGS=${CFLAGS} -DSTATIC' 'XLIBS=-L/lib -ltcl -lm -ldl -lnsl '

GMAKE_SHLIB = ${MAKE} 'CC=${SHLIB_CC}' 'LD=${SHLIB_LD}' \
'STRIP=${SHLIB_STRIP}' 'CFLAGS=${CFLAGS}'

GMAKE_MOD = ${MAKE} 'CC=${MOD_CC}' 'LD=${MOD_LD}' 'OBJS=${OBJS}'\
'CFLAGS=${CFLAGS}' 'XREQ=${XREQ}' \
'TCLLIB=${TCLLIB}' 'STRIP=${MOD_STRIP}' 'RANLIB=ranlib' \
'XLIBS=-L/lib -ltcl -lm -ldl -lnsl '

DMAKE_MEM = ${MAKE} 'CC=${MOD_CC} -DEBUG_MEM' 'LD=${MOD_LD} -g' \
'OBJS=${OBJS}' 'CFLAGS=-g ${CFLAGS}' 'XREQ=${XREQ}' \
'TCLLIB=${TCLLIB}' 'STRIP=touch' 'RANLIB=ranlib' \
'XLIBS=-L/lib -ltcl -lm -ldl -lnsl '

DMAKE_SHLIB = ${MAKE} 'CC=${SHLIB_CC}' 'LD=${SHLIB_LD} -g' \
'STRIP=touch' 'CFLAGS=-g ${CFLAGS}'

static: eggtest
	@echo ""
	@echo "Making module objects for static linking..."
	@echo ""
	@rm -f src/main.o
	@cd src/mod;${GMAKE_STATIC} static
	@echo ""
	@echo "Making core eggdrop for static linking..."
	@echo ""
	@cd src;${GMAKE_STATIC} static

modegg: modtest
	@cd src;${GMAKE_MOD} eggdrop
	@echo
	@./eggdrop -v
	@ls -la eggdrop
	
modules:  modtest
	@cd src/mod;${GMAKE_SHLIB} modules
	@echo
	@echo "modules made:"
	@ls -la *.so

debmodules:  modtest
	@cd src/mod;${DMAKE_SHLIB} modules
	@echo
	@echo "modules made:"
	@ls -la *.so

memegg:  modtest
	@cd src;${DMAKE_MEM} eggdrop
	@echo
	@./eggdrop -v
	@ls -la eggdrop

eggtest:
	@if [ -f EGGMOD.stamp ]; then \
		echo You\'re trying to do a STATIC build of eggdrop when you\'ve;\
		echo already run \'make\' for a module build.;\
		echo You must first type \"make clean\" before you can build;\
		echo a static version.;\
		exit 1;\
	fi
	@date >EGGDROP.stamp

modtest:
	@if [ -f EGGDROP.stamp ]; then \
		echo You\'re trying to do a MODULE build of eggdrop when you\'ve;\
		echo already run \'make\' for a static build.;\
		echo You must first type \"make clean\" before you can build;\
		echo a module version.;\
		exit 1;\
	fi
	@date >EGGMOD.stamp

install: eggdrop ainstall

dinstall: eggdrop ainstall

sinstall: static ainstall

ainstall:
	@if test X$(DEST) = X; then \
		echo "You must specify a destination directory with DEST="; \
		exit 1; \
	fi
	@if test ! -x eggdrop; then \
		echo "You haven't compiled eggdrop yet."; \
		exit 1; \
	fi
	@./eggdrop -v
	@echo
	@echo Installing in directory: $(DEST).
	@echo
	@if test ! -d $(DEST); then \
		echo Creating directory: $(DEST).; \
		mkdir $(DEST); \
	fi
	@$(CP1) README $(DEST)/
	@$(CP1) eggdrop.conf.dist $(DEST)/
	@if test ! -d $(DEST)/language; then \
		echo Creating language subdirectory.; \
		mkdir $(DEST)/language; \
	fi
	@$(CP2) language/* src/mod/*.mod/*.lang $(DEST)/language/
	@if test -r $(DEST)/eggdrop; then \
		rm -f $(DEST)/oeggdrop; \
	fi
	@if test ! -r $(DEST)/motd; then \
	        $(CP1) motd $(DEST)/; \
	fi
	@if test -h $(DEST)/modules; then \
		echo Removing symlink to archival modules directory.; \
		rm -f $(DEST)/modules; \
	fi
	@if test -d $(DEST)/modules/; then \
		echo Moving old modules into \'modules.old\' directory.; \
		rm -rf $(DEST)/modules.old; \
		mv -f $(DEST)/modules $(DEST)/modules.old; \
	fi
	@if test ! -d $(DEST)/modules\-${NEWVERSION}/; then \
		echo Creating modules\-${NEWVERSION} directory and symlink.; \
		mkdir $(DEST)/modules\-${NEWVERSION}; \
	fi
	@$(LN_S) modules\-${NEWVERSION} $(DEST)/modules
	@if test -r assoc.so; then \
		echo Copying new modules.; \
		$(CP3) *.so $(DEST)/modules/; \
	fi
	@if test -h $(DEST)/eggdrop; then \
		echo Removing symlink to archival eggdrop binary.; \
		rm -f $(DEST)/eggdrop; \
	fi
	@if test -r $(DEST)/eggdrop; then \
		echo Renamed the old \'eggdrop\' executable to \'oeggdrop\'.; \
		mv -f $(DEST)/eggdrop $(DEST)/oeggdrop; \
	fi
	@echo Copying new \'eggdrop\' executable and creating symlink.
	@$(CP1) eggdrop $(DEST)/eggdrop\-${NEWVERSION}
	@$(LN_S) eggdrop\-${NEWVERSION} $(DEST)/eggdrop
	@if test ! -d $(DEST)/doc; then \
		echo Creating \'doc\' subdirectory.; \
		mkdir $(DEST)/doc; \
	fi
	@echo Copying \'doc\' files.
	@$(CP2) doc/* $(DEST)/doc/
	@if test ! -d $(DEST)/help; then \
		echo Creating \'help\' subdirectory.; \
		mkdir $(DEST)/help; \
	fi
	@$(CP2) help/* $(DEST)/help/
	@if test ! -d $(DEST)/filesys; then \
		echo Creating a skeletal \'filesys\' subdirectory.; \
		mkdir $(DEST)/filesys; \
		mkdir $(DEST)/filesys/incoming; \
	fi
	@if test ! -d $(DEST)/scripts; then \
		echo Creating a \'scripts\' subdirectory.; \
		mkdir $(DEST)/scripts; \
		echo Copying scripts.; \
		$(CP2) scripts/* $(DEST)/scripts; \
	fi
	@echo
	@${MAKE} REALDEST=`cd $(DEST);pwd` install2
	@echo
	@echo Installation completed.
	@echo You MUST ensure that you edit/verify your configuration file.
	@echo \'eggdrop.conf.dist\' lists current options.
	@echo Remember to change directory to $(DEST) before you proceed.

install2:
	@echo Installing mods -- DEST = $(REALDEST)
	@cd src/mod;${MAKE} REALDEST=$(REALDEST) install CP1='${CP1}'; cd ../..

#safety hash
