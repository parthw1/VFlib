%define ver       3.7.2
%define rel       1
%define prefix    /usr

Summary: VFlib version 3.7.2
Name: VFlib3
Version: %ver
Release: %rel
Copyright: LGPL
Group: Other
Vendor: The TypeHack Project
Packager: Hirotsugu Kakugawa
Url: http://www-masu.ist.osaka-u.ac.jp/~kakugawa/VFlib/
Source: http://www-masu.ist.osaka-u.ac.jp/~kakugawa/download/TypeHack/VFlib3-%{ver}.tar.gz
BuildRoot: /var/tmp/VFlib3-root

Requires: t1lib >= 5.1.0
Requires: freetype >= 1.2
#Requires: kpathsea >= 3.2
Requires: tetex >= 1.0.7


%description
VFlib is a font library written in C language with several functions
to obtain bitmaps of fonts.  Unique feature of VFlib is that fonts
in different formats are accessed by unified interface.

%prep
%setup -n VFlib3-%{ver}

%build
rm -rf $RPM_BUILD_ROOT
./configure --prefix=%prefix \
            --with-freetype \
              --with-freetype-includedir=/usr/include/freetype1/freetype \
              --with-freetype-libdir=/usr/lib \
            --with-t1lib \
              --with-t1lib-includedir=/usr/include \
              --with-t1lib-libdir=/usr/lib \
            --with-kpathsea \
              --with-kpathsea-includedir=/usr/include \
              --with-kpathsea-libdir=/usr/lib

make

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%clean
rm -rf $RPM_BUILD_ROOT

%install
mkdir -p $RPM_BUILD_ROOT/%{prefix}/bin
mkdir -p $RPM_BUILD_ROOT/%{prefix}/lib
mkdir -p $RPM_BUILD_ROOT/%{prefix}/info
mkdir -p $RPM_BUILD_ROOT/%{prefix}/include
mkdir -p $RPM_BUILD_ROOT/%{prefix}/share
mkdir -p $RPM_BUILD_ROOT/%{prefix}/share/VFlib
mkdir -p $RPM_BUILD_ROOT/%{prefix}/share/texmf
make prefix=${RPM_BUILD_ROOT}%prefix install

%files
%defattr(-,root,root,-)
%doc ANNOUNCE* CHANGES DISTRIB.txt LICENSE-*

%{prefix}/bin/*
%{prefix}/lib/*
%{prefix}/include/*
%{prefix}/share/VFlib/*
#%{prefix}/info/*

