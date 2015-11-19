if ENABLE_CLIENT
if WITH_RADOS
if WITH_RBD

PY_DISTUTILS = \
	VERSION_RELEASE="@PACKAGE_VERSION@" \
	CPPFLAGS="-iquote \${abs_srcdir}/include ${AM_CPPFLAGS} ${CPPFLAGS}" \
	CFLAGS="-iquote \${abs_srcdir}/include ${AM_CFLAGS} ${CFLAGS}" \
	LDFLAGS="-L\${abs_builddir}/.libs $(subst -pie,,${AM_LDFLAGS}) ${LDFLAGS}" \
	${PYTHON} ./setup.py

pybind-all: librbd.la
	cd $(srcdir)/pybind; $(PY_DISTUTILS) build \
	--build-base $(shell readlink -f $(builddir))/build \
	--verbose

pybind-clean:
	cd $(srcdir)/pybind; $(PY_DISTUTILS) clean \
	--build-base $(shell readlink -f $(builddir))/build \
	--verbose

pybind-install-exec:
	cd $(srcdir)/pybind; $(PY_DISTUTILS) install \
	--prefix $(DESTDIR)$(prefix) \
	--single-version-externally-managed \
	--record $(DESTDIR)$(pythondir)/rbd_install_files.txt \
	--verbose

LOCAL_ALL += pybind-all
LOCAL_CLEAN += pybind-clean
LOCAL_INSTALLEXEC += pybind-install-exec

endif
endif
endif
