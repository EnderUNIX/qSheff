bin_PROGRAMS= qsheff

qsheff_SOURCES= djb.c main.c loadconfig.c toolkit.c log.c smtp.c scanengine.c miscutil.c

noinst_HEADERS= \
	loadconfig.h \
	log.h \
	djb.h \
	main.h \
	miscutil.h \
	scanengine.h \
	smtp.h \
	toolkit.h

if ENABLE_CLAMAV
qsheff_SOURCES += clam.c
noinst_HEADERS += clam.h
endif

if ENABLE_CPROG
qsheff_SOURCES += exec.c
noinst_HEADERS += exec.h
endif

if ENABLE_DEBUG
AM_CFLAGS= -D_DEBUG_ -g -I/usr/local/include -L/usr/local/lib
else
AM_CFLAGS= -ansi -I/usr/local/include -L/usr/local/lib
endif

EXTRA_DIST = $(qsheffdoc_DATA)

# Copy all the spec files. Of cource, only one is actually used.
dist-hook:
	for specfile in *.spec; do \
		if test -f $$specfile; then \
			cp -p $$specfile $(distdir); \
		fi \
	done

install-data-hook:
	cp -f qsheff @QMAILDIR@/bin/qmail-qsheff
	chown root:@QMAILGROUP@ @QMAILDIR@/bin/qmail-qsheff
	chmod 4511 @QMAILDIR@/bin/qmail-qsheff
	@echo
	@echo " ----------------------------------------------------------------------------"
	@echo "                                                                             "
	@echo "  WARNING! qsheff has not installed yet.                                     "
	@echo "  Type '@sysconfdir@/qsheff-II/install-wrapper.sh' to install it manually.   "
	@echo "                                                                             "
	@echo " ----------------------------------------------------------------------------"
	@echo

uninstall-hook:
	@echo
	@echo " ----------------------------------------------------------------------------"
	@echo "                                                                             "
	@echo "  WARNING! qsheff has not uninstalled yet.                                   "
	@echo "  Type '@sysconfdir@/qsheff-II/install-wrapper.sh' to uninstall it manually. "
	@echo "                                                                             "
	@echo " ----------------------------------------------------------------------------"
	@echo

