# spec file for package dtrace-utils.
#
# Copyright 2011, 2012 Oracle, Inc.  All rights reserved.
#

BuildRequires: rpm
Name:         dtrace-utils
License:      Oracle Corporation
Group:        Development/Tools
Provides:     dtrace-utils
Requires:     gcc elfutils-libelf
BuildRequires: elfutils-libelf-devel kernel-headers glibc-headers fakeroot byacc flex zlib-devel
Summary:      DTrace user interface.
Version:      0.2.5
Release:      2.el6
Source:       dtrace-utils-%{version}.tar.bz2
BuildRoot:    %{_tmppath}/%{name}-%{version}-build
ExclusiveArch:    x86_64

%description
DTrace user interface

Authors:
-------
Pearly Zhao <pearly.zhao@oracle.com>
Nick Alcock <nick.alcock@oracle.com>

%prep
%setup -q

%build
make -j $(getconf _NPROCESSORS_ONLN) VERSION=%{version}

%install
echo rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/usr/sbin
fakeroot make DESTDIR=$RPM_BUILD_ROOT VERSION=%{version} install
# Because systemtap creates a dtrace.1 manpage we have to rename
# ours and then shift theirs out of the way (since the systemtap
# dtrace page references a non-existent binary)
mv $RPM_BUILD_ROOT/usr/share/man/man1/dtrace.1 \
   $RPM_BUILD_ROOT/usr/share/man/man1/orcl-dtrace.1


%clean
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf "$RPM_BUILD_ROOT"
rm -rf $RPM_BUILD_DIR/%{name}-%{version}

%post
/sbin/ldconfig
# if systemtap-dtrace.1.gz doesn't exist then we can move the existing dtrace manpage
MANDIR=/usr/share/man/man1
if [ -e $MANDIR/dtrace.1.gz -a ! -e $MANDIR/systemtap-dtrace.1.gz ]; then
    mv $MANDIR/dtrace.1.gz $MANDIR/systemtap-dtrace.1.gz
    ln -s $MANDIR/orcl-dtrace.1.gz $MANDIR/dtrace.1.gz
elif [ ! -e $MANDIR/dtrace.1.gz ]; then
    ln -s $MANDIR/orcl-dtrace.1.gz $MANDIR/dtrace.1.gz
fi

%postun
/sbin/ldconfig
MANDIR=/usr/share/man/man1
if [ -h $MANDIR/dtrace.1.gz ]; then
    rm -f $MANDIR/dtrace.1.gz
fi


%files
%defattr(-,root,root,755)
%exclude /usr/src/debug
%exclude /usr/lib/debug
/usr/lib/dtrace/*
/usr/lib/libdtrace.so*
/usr/sbin/dtrace
/usr/share/man/man1/orcl-dtrace.1.gz
/usr/include/dtrace.h
/usr/include/sys/dtrace.h
/usr/include/sys/dtrace_types.h
/usr/include/sys/ctf.h
/usr/include/sys/ctf_api.h
/usr/share/doc/dtrace-%{version}/*

%changelog
* Mon Mar 19 2012 - nick.alcock@oracle.com - 0.2.5-2
- Call ldconfig at appropriate times.
* Tue Mar 13 2012 - nick.alcock@oracle.com - 0.2.5
- libdtrace is now a shared library, with non-stable API/ABI.
* Thu Feb 16 2012 - nick.alcock@oracle.com - 0.2.4
- Updated README; new NEWS and PROBLEMS; synch with module version
* Thu Feb  9 2012 - nick.alcock@oracle.com - 0.2.3
- Fixes for reproducibility of test results under load
- Fix -G when setting the syslibdir
* Mon Feb  6 2012 - nick.alcock@oracle.com - 0.2.2
- Fix spurious failures of tst.resize*.d.
* Tue Jan 31 2012 - nick.alcock@oracle.com - 0.2.1
- Fix 'make check-installed' with an unbuilt source tree.
* Thu Jan 26 2012 - kris.van.hees@oracle.com - 0.2.0
- Branch for 0.2.0 release.
* Fri Oct  7 2011 - philip.copeland@oracle.com - 0.1-0.3
- The systemtap package in the 'wild' creates a dtrace.1 manpage
  which is bizarre since it doesn't have an associated dtrace 
  binary. This will cause a conflict and the rpm will not install
  Since that man page is superfluous I've had to add a %post
  section here to move our manpage into position in such a way as
  to keep the rpm database happy. Technically this is a bit evil.
* Tue Oct 06 2011 - nick.alcock@oracle.com - 0.2
- Fix copyright.
* Tue Sep 27 2011 - nick.alcock@oracle.com - 0.1
- Branch for initial release.
* Mon Jun 27 2011 - pearly.zhao@oracle.com - 0.0.1
- Initial build for dtrace.  
