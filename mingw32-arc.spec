%define __strip %{_mingw32_strip}
%define __objdump %{_mingw32_objdump}
%define _use_internal_dependency_generator 0
%define __find_requires %{_mingw32_findrequires}
%define __find_provides %{_mingw32_findprovides}

%define pkgdir arc

Name: mingw32-nordugrid-arc1
Version: 0.9.0
Release: 1%{?dist}
Summary: ARC
Group: System Environment/Daemons
License: ASL 2.0
URL: http://www.nordugrid.org/
Source: nordugrid-arc1-%{version}.tar.gz
BuildRoot:%{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildArch: noarch

BuildRequires: mingw32-filesystem >= 23
BuildRequires: mingw32-runtime >= 3.15.1
BuildRequires: mingw32-gcc
BuildRequires: mingw32-gcc-c++
BuildRequires: mingw32-binutils
BuildRequires: mingw32-gettext
BuildRequires: mingw32-glibmm24
BuildRequires: mingw32-glib2
BuildRequires: mingw32-libxml2
BuildRequires: mingw32-openssl
BuildRequires: mingw32-w32api
BuildRequires: mingw32-libgnurx
BuildRequires: mingw32-xmlsec1
BuildRequires: pkgconfig
#BuildRequires: gsoap-devel >= 2.7.2
#BuildRequires: db4-devel
BuildRequires: mingw32-globus-common
BuildRequires: mingw32-globus-ftp-client
BuildRequires: mingw32-globus-ftp-control
BuildRequires: mingw32-globus-rls-client

%define pyver %(python -c 'import sys; print sys.version[:3]')
%if "%{pyver}" < "2.4"
%define ifpy #
%else
%define ifpy %{nil}
%endif

%description
ARC

%package client
Summary: ARC prototype clients
Group: Applications/Internet
Requires: %{name} = %{version}
Requires: %{name}-plugins-base = %{version}

%description client
ARC prototype clients.

%package server
Summary: ARC Hosting Environment Daemon
Group: System Environment/Libraries
Requires: %{name} = %{version}

%description server
ARC Hosting Environment Daemon (HED).

%package arex
Summary: ARC Remote EXecution service
Group: System Environment/Libraries
Requires: %{name} = %{version}
Requires: %{name}-server = %{version}
#Requires: globus_common
#Requires: globus_rsl
Requires: perl-XML-Simple

%description arex
ARC Remote EXecution service (AREX)

%package plugins-base
Summary: ARC base plugins
Group: System Environment/Libraries
Requires: %{name} = %{version}

%description plugins-base
ARC base plugins. This includes the Message Chain Components (MCCs) and
Data Manager Components (DMCs).

%package plugins-globus
Summary: ARC Globus plugins
Group: System Environment/Libraries
Requires: %{name} = %{version}
#Requires: globus_common
#Requires: globus_ftp_client
#Requires: globus_ftp_control
#Requires: globus_rls_client
#Requires: LFC-client

%description plugins-globus
ARC Globus plugins. This includes the Globus dependent Data Manager
Components (DMCs):

  libdmcgridftp.so
  libdmclfc.so
  libdmcrls.so

%package devel
Summary: ARC development files
Group: Development/Libraries
Requires: %{name} = %{version}
Requires: mingw32-glibmm24
Requires: mingw32-glib2
Requires: mingw32-libxml2
Requires: mingw32-openssl

%description devel
Development files for ARC

%package python
Summary: ARC Python wrapper
Group: Development/Libraries
Requires: %{name} = %{version}
Requires: python

%description python
Python wrapper for ARC

%if "%{!?disable_java:java}"
%package java
Summary: ARC Java wrapper
Group: Development/Libraries
Requires: %{name} = %{version}

%description java
Java wrapper for ARC
%endif

%package doc
Summary: ARC API documentation
Group: Documentation

%description doc
ARC API docmentation

%prep
%setup -q -n nordugrid-arc1-%{version}

%build
%{_mingw32_configure} \
  --disable-xmlsec1 \
  --disable-java \
  --disable-python \
  --disable-doc \
  --disable-static LDFLAGS="-Wl,--enable-auto-import"

make %{?_smp_mflags}

#make check

%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT
find $RPM_BUILD_ROOT -type f -name \*.la -exec rm -fv '{}' ';'
#mkdir -p $RPM_BUILD_ROOT/etc/init.d
#cp -p src/hed/daemon/scripts/arched.redhat $RPM_BUILD_ROOT/etc/init.d/arched
#chmod +x $RPM_BUILD_ROOT/etc/init.d/arched

# RPM does it's own doc handling
rm -fr $RPM_BUILD_ROOT%{_mingw32_datadir}/doc/%{pkgdir}/

%find_lang arc

%clean
rm -rf $RPM_BUILD_ROOT

%files -f arc.lang
%defattr(-,root,root,-)
%doc ChangeLog README AUTHORS LICENSE
#%{_mingw32_libdir}/lib*.so.*
%{_mingw32_bindir}/lib*.dll
%{_mingw32_datadir}/%{pkgdir}/schema

%files client
%defattr(-,root,root,-)
%doc src/clients/charon/charon_client.xml.example
%doc src/clients/charon/charon_request.xml.example
%doc src/tests/echo/echo.wsdl
%{_mingw32_datadir}/%{pkgdir}/examples/client.conf.example
%{_mingw32_bindir}/arcecho.exe
%doc %{_mingw32_mandir}/man1/arcecho.1*
%{_mingw32_bindir}/arcsrmping.exe
%doc %{_mingw32_mandir}/man1/arcsrmping.1*
%{_mingw32_bindir}/arcinfo.exe
%doc %{_mingw32_mandir}/man1/arcinfo.1*
%{_mingw32_bindir}/arcproxy.exe
%doc %{_mingw32_mandir}/man1/arcproxy.1*
#%{_mingw32_bindir}/arcslcs.exe
%doc %{_mingw32_mandir}/man1/arcslcs.1*
%{_mingw32_bindir}/arccat.exe
%doc %{_mingw32_mandir}/man1/arccat.1*
%{_mingw32_bindir}/arccp.exe
%doc %{_mingw32_mandir}/man1/arccp.1*
%{_mingw32_bindir}/arcls.exe
%doc %{_mingw32_mandir}/man1/arcls.1*
%{_mingw32_bindir}/arcrm.exe
%doc %{_mingw32_mandir}/man1/arcrm.1*
%{_mingw32_bindir}/arcstat.exe
%doc %{_mingw32_mandir}/man1/arcstat.1*
%{_mingw32_bindir}/arcsub.exe
%doc %{_mingw32_mandir}/man1/arcsub.1*
%{_mingw32_bindir}/arcsync.exe
%doc %{_mingw32_mandir}/man1/arcsync.1*
%{_mingw32_bindir}/arcresub.exe
%doc %{_mingw32_mandir}/man1/arcresub.1*
%{_mingw32_bindir}/arcget.exe
%doc %{_mingw32_mandir}/man1/arcget.1*
%{_mingw32_bindir}/arcclean.exe
%doc %{_mingw32_mandir}/man1/arcclean.1*
%{_mingw32_bindir}/arckill.exe
%doc %{_mingw32_mandir}/man1/arckill.1*
%{_mingw32_bindir}/arcdecision.exe
%doc %{_mingw32_mandir}/man1/arcdecision.1*
%{_mingw32_bindir}/arcmigrate.exe
%doc %{_mingw32_mandir}/man1/arcmigrate.1*
%{_mingw32_bindir}/arcrenew.exe
%doc %{_mingw32_mandir}/man1/arcrenew.1*
%{_mingw32_bindir}/arcresume.exe
%doc %{_mingw32_mandir}/man1/arcresume.1*
#%{_mingw32_bindir}/voms_assertion_init
#%doc %{_mingw32_mandir}/man1/voms_assertion_init.1*
%{_mingw32_bindir}/perftest.exe
%doc %{_mingw32_mandir}/man1/perftest.1*
#%{_mingw32_bindir}/arc_storage_cli
#%doc %{_mingw32_mandir}/man1/arc_storage_cli.1*
%{_mingw32_sysconfdir}/arc/client.conf
#%{_mingw32_bindir}/arexlutsclient.exe
#%{_mingw32_bindir}/arexlutsclient-wrapper.sh
%{_mingw32_bindir}/jura.exe
%{_mingw32_bindir}/isistest.exe
%{_mingw32_bindir}/paul_gui.exe

%files server
%defattr(-,root,root,-)
#/etc/init.d/arched
%{_mingw32_sbindir}/arched.exe
%doc %{_mingw32_mandir}/man8/arched.8*
#%{_mingw32_sbindir}/manage_jobq
#%doc %{_mingw32_mandir}/man8/manage_jobq.8*
#%doc src/services/charon/charon_policy.xml.example
#%doc src/services/charon/charon_service.xml.example
#%doc src/services/hopi/hopi_service.xml.example
#%doc src/services/storage/storage_service.xml.example
#%doc src/tests/echo/echo_service.xml.example
%{_mingw32_libdir}/%{pkgdir}/libecho.dll*
#%{_mingw32_libdir}/%{pkgdir}/libcharon.dll*
#%{_mingw32_libdir}/%{pkgdir}/libhopi.dll*
#%{_mingw32_libdir}/%{pkgdir}/libisis.dll*
#%{_mingw32_libdir}/%{pkgdir}/libgrid_sched.so
#%{_mingw32_libdir}/%{pkgdir}/libpaul.dll*
%{_mingw32_libdir}/%{pkgdir}/libslcs.dll*
#%{_mingw32_libdir}/%{pkgdir}/libcompiler.dll*
#%{_mingw32_libdir}/%{pkgdir}/libsaml2sp.dll*
%{_mingw32_libdir}/%{pkgdir}/libdelegation.dll*

%files arex
%defattr(-,root,root,-)
#/etc/init.d/a-rex
#%doc src/services/a-rex/arex_service.xml.example
#%doc src/services/a-rex/arex_minimalistic.xml.example
#%doc src/services/a-rex/arex_service-secure.xml.example
#%doc src/services/a-rex/arc_arex.conf
#%doc %{_mingw32_mandir}/man1/cache-clean.1*
#%doc %{_mingw32_mandir}/man1/cache-list.1*
#%config(noreplace) %{_mingw32_sysconfdir}/arc_arex.conf
#%{_mingw32_libdir}/%{pkgdir}/libarex.so
#%{_mingw32_libexecdir}/%{pkgdir}/downloader
#%{_mingw32_libexecdir}/%{pkgdir}/uploader
#%{_mingw32_libexecdir}/%{pkgdir}/smtp-send
#%{_mingw32_libexecdir}/%{pkgdir}/smtp-send.sh
#%{_mingw32_libexecdir}/%{pkgdir}/gm-kick
#%{_mingw32_libexecdir}/%{pkgdir}/cache-clean
#%{_mingw32_libexecdir}/%{pkgdir}/cache-list
##
#%{_mingw32_libdir}/%{pkgdir}/ARC0mod.pm
#%{_mingw32_libdir}/%{pkgdir}/FORKmod.pm
#%{_mingw32_libdir}/%{pkgdir}/SGEmod.pm
#%{_mingw32_libdir}/%{pkgdir}/LL.pm
#%{_mingw32_libdir}/%{pkgdir}/LSF.pm
#%{_mingw32_libdir}/%{pkgdir}/PBS.pm
#%{_mingw32_libdir}/%{pkgdir}/Condor.pm
##
#%{_mingw32_libdir}/%{pkgdir}/ARC0ClusterInfo.pm
#%{_mingw32_libdir}/%{pkgdir}/ARC0ClusterSchema.pm
#%{_mingw32_libdir}/%{pkgdir}/ARC1ClusterInfo.pm
#%{_mingw32_libdir}/%{pkgdir}/ARC1ClusterSchema.pm
#%{_mingw32_libdir}/%{pkgdir}/ConfigParser.pm
#%{_mingw32_libdir}/%{pkgdir}/GMJobsInfo.pm
#%{_mingw32_libdir}/%{pkgdir}/HostInfo.pm
#%{_mingw32_libdir}/%{pkgdir}/InfoChecker.pm
#%{_mingw32_libdir}/%{pkgdir}/InfoCollector.pm
#%{_mingw32_libdir}/%{pkgdir}/LRMSInfo.pm
##
#%{_mingw32_libdir}/%{pkgdir}/LogUtils.pm
#%{_mingw32_libdir}/%{pkgdir}/Sysinfo.pm
##
#%{_mingw32_libexecdir}/%{pkgdir}/submit-*-job
#%{_mingw32_libexecdir}/%{pkgdir}/cancel-*-job
#%{_mingw32_libexecdir}/%{pkgdir}/scan-*-job
#%{_mingw32_libdir}/%{pkgdir}/configure-*-env.sh
##
#%{_mingw32_libdir}/%{pkgdir}/submit_common.sh
#%{_mingw32_libdir}/%{pkgdir}/cancel_common.sh
#%{_mingw32_libdir}/%{pkgdir}/config_parser.sh
##
#%{_mingw32_libdir}/%{pkgdir}/condor_env.pm
#%{_mingw32_libdir}/%{pkgdir}/change-lsf-mode.sh
#%{_mingw32_libexecdir}/%{pkgdir}/finish-condor-job
##
#%{_mingw32_libexecdir}/%{pkgdir}/CEinfo.pl
#%{_mingw32_datadir}/%{pkgdir}/nordugrid.schema

%files devel
%defattr(-,root,root,-)
%{_mingw32_includedir}/%{pkgdir}
#%{_mingw32_libdir}/lib*.a
#%{_mingw32_libdir}/lib*.la
%{_mingw32_libdir}/lib*.dll.a
%{_mingw32_bindir}/wsdl2hed.exe
%doc %{_mingw32_mandir}/man1/wsdl2hed.1*
%{_mingw32_libdir}/pkgconfig/arcbase.pc

%files plugins-base
%defattr(-,root,root,-)
%{_mingw32_libdir}/%{pkgdir}/libmcchttp.dll**
%{_mingw32_libdir}/%{pkgdir}/libmccmsgvalidator.dll*
%{_mingw32_libdir}/%{pkgdir}/libmccsoap.dll*
%{_mingw32_libdir}/%{pkgdir}/libmcctcp.dll*
%{_mingw32_libdir}/%{pkgdir}/libmcctls.dll*
%{_mingw32_libdir}/%{pkgdir}/libdmcfile.dll*
%{_mingw32_libdir}/%{pkgdir}/libdmchttp.dll*
%{_mingw32_libdir}/%{pkgdir}/libdmcarc.dll*
%{_mingw32_libdir}/%{pkgdir}/libdmcldap.dll*
%{_mingw32_libdir}/%{pkgdir}/libarcshc.dll*
%{_mingw32_libdir}/%{pkgdir}/libidentitymap.dll*
%{_mingw32_libdir}/%{pkgdir}/libaccARC1.dll*
%{_mingw32_libdir}/%{pkgdir}/libaccCREAM.dll*
%{_mingw32_libdir}/%{pkgdir}/libaccBroker.dll*
%{_mingw32_libdir}/%{pkgdir}/libaccUNICORE.dll*

%files plugins-globus
%defattr(-,root,root,-)
%{_mingw32_libdir}/%{pkgdir}/libmccgsi.dll*
%{_mingw32_libdir}/%{pkgdir}/libdmcgridftp.dll*
#%{_mingw32_libdir}/%{pkgdir}/libdmclfc.so
%{_mingw32_libdir}/%{pkgdir}/libdmcrls.dll*
#%{_mingw32_libdir}/%{pkgdir}/libdmcsrm.so
%{_mingw32_libdir}/%{pkgdir}/libaccARC0.dll*

%files python
%defattr(-,root,root,-)
#%{_mingw32_libdir}/python?.?/site-packages/
#%{ifpy}%{_mingw32_libdir}/%{pkgdir}/libpythonservice.so
#%{ifpy}%{_mingw32_libdir}/%{pkgdir}/libaccPythonBroker.so
#%{ifpy}%doc src/hed/acc/PythonBroker/SampleBroker.py

#%if "%{!?disable_java:java}"
#%files java
#%defattr(-,root,root,-)
#%{_mingw32_libdir}/java/libjarc.so
#%{_mingw32_datadir}/java/arc.jar
#%{_mingw32_libdir}/%{pkgdir}/libjavaservice.so
#%endif

%files doc
%defattr(-,root,root,-)
%doc doc/tech_doc/doxygen/*pdf

%changelog

* Fri Jul 27 2007 Anders Waananen <waananen@nbi.dk> - 1.0-1
- Initial release

