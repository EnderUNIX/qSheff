qsheffconfdir = ${sysconfdir}/qsheff-II
qsheffconf_DATA =\
		qsheff.attach-default\
		qsheff.conf-default\
		qsheff.rules-default\
		qsheff.ignore-default

EXTRA_DIST = $(qsheffconf_DATA)

install-data-hook:
	cp -f qsheff.conf-default ${qsheffconfdir}/qsheff.conf
	if [ ! -f "${qsheffconfdir}/qsheff.attach" ]; then cp -f qsheff.attach-default ${qsheffconfdir}/qsheff.attach; fi
	if [ ! -f "${qsheffconfdir}/qsheff.ignore" ]; then cp -f qsheff.ignore-default ${qsheffconfdir}/qsheff.ignore; fi
	if [ ! -f "${qsheffconfdir}/qsheff.rules" ]; then cp -f qsheff.rules-default ${qsheffconfdir}/qsheff.rules; fi
	chmod 755 uninstall-wrapper.sh install-wrapper.sh
	cp -f uninstall-wrapper.sh install-wrapper.sh ${qsheffconfdir}/

