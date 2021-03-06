%global _enable_debug_package 0
%global debug_package %{nil}

Summary:            Messaging Performance Tool
Name:               msg-perf-tool
Version:            0.1.1
Release:            0%{?dist}
License:            Apache v2
Group:              Development/Tools
Source:             msg-perf-tool-%{version}.tar.gz
URL:                https://github.com/orpiske/msg-perf-tool
BuildRequires:      cmake
BuildRequires:      make
BuildRequires:      gcc
BuildRequires:      gcc-c++
BuildRequires:      qpid-proton-c-devel
Requires:           qpid-proton-c
Requires:           python
Requires:           python-requests


%description
A tool for measuring messaging system performance

%prep
%setup -n msg-perf-tool-%{version}

%build
mkdir build && cd build
cmake -DSTOMP_SUPPORT=OFF -DAMQP_SUPPORT=ON -DCMAKE_BUILD_TYPE=RELEASE -DCMAKE_INSTALL_PREFIX=%{buildroot}/usr ..
make

%install
cd build
make install

%files
%doc README.md LICENSE
%{_bindir}/*
%{_libdir}/*
%{_datadir}/*


%changelog
* Fri Oct 14 2016 Otavio R. Piske <angusyoung@gmail.com> - 20161014
- Version 0.1.1 release
- Several bug fixes in the loader
- The loader now uses time-based index for greater performance on the UI
- The loader now adds additional mappings for all loaded index
- The loader now requires the test start time for additional operations


* Fri Aug 05 2016 Otavio R. Piske <angusyoung@gmail.com> - 20160805
- Version 0.1.0 release
- Removed self generated data using gnuplot
- Removed the log parser
- Performance data is saved straight to CSV instead of being parsed later
- Added a loader to load data to ElasticSearch database
- Cleaned up the logs
- Added stomp support
- Improved support for Raspberry PI
- Minor fixes for memory management and file usage
- Fixes an incorrect variable reference


* Thu Jun 15 2016 Otavio R. Piske <angusyoung@gmail.com> - 20160615
- Improved the runner script with additional information about the test execution
- Small fixes for validating input parameters

* Thu Jun 09 2016 Otavio R. Piske <angusyoung@gmail.com> - 20160609
- Initial release
