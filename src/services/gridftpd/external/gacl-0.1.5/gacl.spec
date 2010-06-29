Version: 0.1.5
Summary: Grid ACL handling library
Name: gacl
Release: 1
Copyright: BSD
Group: Utilities/System
Source: %{name}-%{version}.src.tar.gz
Vendor: EDG/GridPP
Requires: libxml2
Buildrequires: libxml2-devel
Packager: Andrew McNab <mcnab@hep.man.ac.uk>

%description
Grid ACL handling library
See http://www.gridpp.ac.uk/authz/gacl/ and CHANGELOG in
/usr/share/doc/gacl-VERSION for details.

%package devel
Summary: Libraries and includes for building with Grid ACL library GACL
Group: Development/Libraries
Requires: %{name}

%description devel
Libraries and includes for building with Grid ACL library GACL
See http://www.gridpp.ac.uk/authz/gacl/

%prep

%setup

%build
make

%install
make install

%files
/usr/lib/libgacl.so.%{version}
/usr/lib/libgacl.so.0
/usr/lib/libgacl.so
/usr/share/doc/%{name}-%{version}

%files devel
/usr/include/gacl.h
/usr/lib/libgacl.a
