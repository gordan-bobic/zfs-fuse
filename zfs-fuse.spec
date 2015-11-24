%global	_sbindir	/sbin

Name:			zfs-fuse
Version:		0.7.2.2
Release:		1%{?dist}
Summary:		ZFS ported to Linux FUSE
Group:			System Environment/Base
License:		CDDL
URL:			https://github.com/gordan-bobic/zfs-fuse
Source00:		%{name}/%{name}-%{version}.tar.xz
BuildRequires:		fuse-devel libaio-devel scons zlib-devel openssl-devel libattr-devel prelink lzo-devel xz-devel bzip2-devel
Requires:		fuse >= 2.7.4-1
Requires:		lzo xz zlib bzip2 libaio
%if 0%{?rhel} <= 6
Requires(post):		chkconfig
Requires(preun):	chkconfig initscripts
Requires(postun):	initscripts
%else
Requires(post):		systemd
Requires(preun):	systemd
Requires(postun):	systemd
%endif
# (2010 karsten@redhat.com) zfs-fuse doesn't have s390(x) implementations for atomic instructions
ExcludeArch:		s390 s390x
BuildRoot:		%{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

%description
ZFS is an advanced modern general-purpose filesystem from Sun
Microsystems, originally designed for Solaris/OpenSolaris.

This project is a port of ZFS to the FUSE framework for the Linux
operating system.

Project home page is at http://zfs-fuse.net/

%prep
%setup -q

%build
export CCFLAGS="%{optflags}"
pushd src
%if 0%{?rhel} <= 6
%{__perl} -pi -e 's@-fstrict-volatile-bitfields@@' lib/libumem/Makefile.in SConstruct
%endif

scons --cache-disable --quiet debug=0 %{_smp_mflags}

%install
%{__rm} -rf %{buildroot}
pushd src
scons debug=0 install install_dir=%{buildroot}%{_sbindir} man_dir=%{buildroot}%{_mandir}/man8/ cfg_dir=%{buildroot}/%{_sysconfdir}/%{name}
%if 0%{?rhel} <= 6
%{__install} -Dp -m 0755 ../zfs-fuse.init %{buildroot}%{_initrddir}/%{name}
%else
%{__install} -Dp -m 0644 ../zfs-fuse.service %{buildroot}%{_unitdir}/%{name}.service
%{__install} -Dp -m 0644 ../zfs-fuse-pid.service %{buildroot}%{_unitdir}/%{name}-pid.service
%{__install} -Dp -m 0644 ../zfs-fuse-oom.service %{buildroot}%{_unitdir}/%{name}-oom.service
%endif
%{__install} -Dp -m 0755 ../zfs-fuse.scrub %{buildroot}%{_sysconfdir}/cron.weekly/98-%{name}-scrub
%{__install} -Dp -m 0644 ../zfs-fuse.sysconfig %{buildroot}%{_sysconfdir}/sysconfig/%{name}
%{__install} -Dp -m 0644 ../zfsrc %{buildroot}%{_sysconfdir}/zfs/zfsrc
%{__install} -Dp -m 0644 ../zfs-fuse.modules-load %{buildroot}%{_sysconfdir}/modules-load.d/fuse.conf
%{__install} -Dp -m 0644 ../zfs-fuse.modules-load %{buildroot}%{_sharedstatedir}/modules-load.d/fuse.conf

#set stack not executable, BZ 911150
for i in zdb zfs zfs-fuse zpool ztest; do
       /usr/bin/execstack -c %{buildroot}%{_sbindir}/$i
done

%clean
%{__rm} -rf %{buildroot}

%post
# echo "Post: $1 packages"

# Move cache if upgrading
oldcache=/etc/zfs/zpool.cache      # this changed per 0.6.9, only needed when upgrading from earlier versions
newcache=/var/lib/zfs/zpool.cache

if [[ -f $oldcache && ! -e $newcache ]]; then
  echo "Moving existing zpool.cache to new location"
  mkdir -p $(dirname $newcache)
  mv $oldcache $newcache
else
  if [ -e $oldcache ]; then
    echo "Note: old zpool.cache present but no longer used ($oldcache)"
  fi
fi

if [ $1 -eq 1 ] ; then
%if 0%{?rhel} <= 6
    /sbin/chkconfig --add %{name}
%else
    /bin/systemctl daemon-reload > /dev/null 2>&1 || :
%endif
fi

%preun
# echo "Preun: $1 packages"
if [ $1 -eq 0 ] ; then
    echo "Stopping service since we are uninstalling last package"
%if 0%{?rhel} <= 6
    /sbin/service %{name} stop >/dev/null 2>&1 || :
    /sbin/chkconfig --del %{name}
%else
    /bin/systemctl --no-reload disable zfs-fuse.service > /dev/null 2>&1 || :
    /bin/systemctl stop zfs-fuse.service > /dev/null 2>&1 || :
%endif
fi

%postun
# echo "Postun: $1 packages"
if [ $1 -ge 1 ] ; then
    echo "Restarting since we have updated the package"
%if 0%{?rhel} <= 6
    /sbin/service %{name} condrestart >/dev/null 2>&1 || :
%else
    /bin/systemctl try-restart zfs-fuse.service > /dev/null || :
%endif
else
    echo "Removing files since we removed the last package"
    rm -rf /var/run/zfs
    rm -rf /var/lock/zfs
fi

%files
%defattr(-, root, root, -)
%doc BUGS CHANGES contrib HACKING LICENSE README 
%doc README.NFS STATUS TESTING TODO
%{_sbindir}/mount.zfs
%{_sbindir}/zdb
%{_sbindir}/zfs
%{_sbindir}/zfs-fuse
%{_sbindir}/zpool
%{_sbindir}/zstreamdump
%{_sbindir}/ztest
%if 0%{?rhel} <= 6
%{_initrddir}/%{name}
%else
%{_unitdir}/%{name}.service
%{_unitdir}/%{name}-pid.service
%{_unitdir}/%{name}-oom.service
%endif
%{_sharedstatedir}/modules-load.d/fuse.conf
%{_sysconfdir}/modules-load.d/fuse.conf
%{_sysconfdir}/cron.weekly/98-%{name}-scrub
%config(noreplace) %{_sysconfdir}/zfs/zfsrc
%config(noreplace) %{_sysconfdir}/sysconfig/%{name}
%{_sysconfdir}/%{name}/zfs_pool_alert
%{_mandir}/man8/zfs-fuse.8.gz
%{_mandir}/man8/zdb.8.gz
%{_mandir}/man8/zfs.8.gz
%{_mandir}/man8/zpool.8.gz
%{_mandir}/man8/zstreamdump.8.gz

%changelog
* Tue Nov 24 2015 Gordan Bobic <gordan@redsleeve.org> - 0.7.2.2-1
- Fixed building RPMs directly from a tarball.
- Add systemd service to immunize zfs-fuse from OOM killer.
- Extra alignment compiler flags (only on EL7+)

* Thu Oct 29 2015 Gordan Bobic <gordan@redsleeve.org> - 0.7.2.1-1
- Additional systemd service to immunize zfs-fuse against the
  OOM killer
- Silenece the fuse_req_getgroups warning as it floods syslog
  when zfs-fuse is used for rootfs.

* Tue Jun 16 2015 Gordan Bobic <gordan@redsleeve.org> - 0.7.2-1
- Support for pool versions 27 and 28

* Fri Jun 12 2015 Gordan Bobic <gordan@redsleeve.org> - 0.7.1-4
- Add ashift setting support (Ray Vantassle)
- Additional ARM patches (Ray Vantassle)
- Backport extra out-of-tree Fedora patches
- Backport mount.zfs from ZoL
- Add the last of missing Seth Heeren's fixes
- Improved systemd integration

* Tue Aug 13 2013 Gordan Bobic <gordan@redsleeve.org> - 0.7.0.20131023-5
- Update to Emmanuel Anne's latest branch for pool v26 support.
- New compression dependencies and spec/build cleanup.

* Fri Feb 15 2013 Jon Ciesla <limburgher@gmail.com> - 0.7.0-3
- Patch to add stack-protector and FORTIFY_SOURCE, BZ 911150.
- Set stack not executable on some binaries, BZ 911150.

* Tue Feb 28 2012 Jon Ciesla <limburgher@gmail.com> - 0.7.0-2
- Fixed sysconfig permissions, BZ 757488.

* Mon Feb 27 2012 Jon Ciesla <limburgher@gmail.com> - 0.7.0-1
- New upstream, fix FTBFS BZ 716087.
- Patch out bad umem declaration.
- Stop starting automatically in post. BZ 755464.
- Marked sysconfig file noreplace, BZ 772403.
- Setting weekly scrub to off by default in sysconfig to silence crob job if service disabled, BZ 757488 et. al.



* Sat Jan 14 2012 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 0.6.9-9.20100709git
- Rebuilt for https://fedoraproject.org/wiki/Fedora_17_Mass_Rebuild

* Tue Feb 08 2011 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 0.6.9-8.20100709git
- Rebuilt for https://fedoraproject.org/wiki/Fedora_15_Mass_Rebuild

* Sun Aug 01 2010 Uwe Kubosch <uwe@kubosch.no> - 0.6.9-7.20100709git
- Moved to fedpkg and git
- Fixed missing dependency to libaio

* Fri Jul 09 2010 Uwe Kubosch <uwe@kubosch.no> - 0.6.9-6.20100709git
- Updated to upstream maintenance snapshot.
- Fixes build problems on EL5
- Added zfs-fuse man page
- Removed package patching of linked libraries

* Mon Jul 05 2010 Uwe Kubosch <uwe@kubosch.no> - 0.6.9-5
- Cleanup of RPM spec and init script

* Sun Jul 04 2010 Uwe Kubosch <uwe@kubosch.no> - 0.6.9-4
- Patched SConstruct to define NDEBUG instead of DEBUG to avoid debug code while still generating debug symbols
- Added moving of zfs.cache when updating from pre 0.6.9 version

* Sat Jul 03 2010 Uwe Kubosch <uwe@kubosch.no> - 0.6.9-2
- Updated to upstream stable release 0.6.9
- Patched default debug level from 0 to 1
- Fixed missing compiler flags and debug flag in build: BUG 595442

* Sat May 22 2010 Uwe Kubosch <uwe@kubosch.no> - 0.6.9_beta3-6
- Updated to upstream version 0.6.9_beta3
- Add more build requires to build on F13 BUG 565076
- Add patches for missing libraries and includes to build on F13 BUG 565076
- Added packages for ppc and ppc64
- Build on F13 BUG 565076
- Fixes BUG 558172
- Added man files
- Added zfs_pool_alert
- Added zstreamdump
- Fixed bug in automatic scrub script BUG 559518

* Mon Jan 04 2010 Uwe Kubosch <uwe@kubosch.no> - 0.6.0-6
- Added option for automatic weekly scrubbing.
  Set ZFS_WEEKLY_SCRUB=yes in /etc/sysconfig/zfs-fuse to enable
- Changed ZFS_AUTOMOUNT option value from "1" to "yes" for better readability.
  ZFS_AUTOMOUNT=1 deprecated and will be removed in version 0.7.0.
- Added option for killing processes with unknown working directory at zfs-fuse startup.
  This would be the case if zfs-fuse crashed.  Use with care.  It may kill unrelated processes.
  Set ZFS_KILL_ORPHANS=yes_really in /etc/sysconfig/zfs-fuse to enable.
- Relaxed dependency on fuse from 2.8.0 to 2.7.4 to allow installation on RHEL/Centos 5

* Sat Dec 26 2009 Uwe Kubosch <uwe@kubosch.no> - 0.6.0-5
- Removed chckconfig on and service start commands from install script
  See https://fedoraproject.org/wiki/Packaging:SysVInitScript#Why_don.27t_we

* Sat Dec 26 2009 Uwe Kubosch <uwe@kubosch.no> - 0.6.0-4
- Updated to upstream version 0.6.0 STABLE

* Mon Nov 30 2009 Uwe Kubosch <uwe@kubosch.no> - 0.6.0-3
- Updated the home page URL to http://zfs-fuse.net/

* Sat Nov 28 2009 Uwe Kubosch <uwe@kubosch.no> - 0.6.0-2
- Corrected some KOJI build errors.

* Fri Nov 27 2009 Uwe Kubosch <uwe@kubosch.no> - 0.6.0-1
- Updated to upstream version 0.6.0 BETA
- Updated dependency to Fuse 2.8.0
- Minor change in spec: Source0 to Source00 for consistency

* Mon Jul 27 2009 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 0.5.0-9.20081221.1
- Rebuilt for https://fedoraproject.org/wiki/Fedora_12_Mass_Rebuild

* Wed Jun 10 2009 Karsten Hopp <karsten@redhat.com> 0.5.0-8.20081221.1
- excludearch s390, s390x as there is no implementation for atomic instructions

* Wed Feb 25 2009 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 0.5.0-8.20081221
- Rebuilt for https://fedoraproject.org/wiki/Fedora_11_Mass_Rebuild

* Sat Jan 24 2009 Uwe Kubosch <uwe@kubosch.no> - 0.5.0-7.20081221
- Updated etc/init.d/zfs-fuse init script after feedback from Rudd-O
  Removed limits for the fuse process which could lead to a hung system
  or use lots of memory.

* Sun Dec 28 2008 Uwe Kubosch <uwe@kubosch.no> - 0.5.0-6.20081221
- Updated etc/init.d/zfs-fuse init script after feedback from Rudd-O at
  http://groups.google.com/group/zfs-fuse/browse_thread/thread/da94aa803bceef52
- Adds better wait at startup before mounting filesystems.
- Add OOM kill protection.
- Adds syncing of disks at shutdown.
- Adds pool status when asking for service status.
- Changed to start zfs-fuse at boot as default.
- Changed to start zfs-fuse right after installation.
- Cleanup of /var/run/zfs and /var/lock/zfs after uninstall.

* Wed Dec 24 2008 Uwe Kubosch <uwe@kubosch.no> - 0.5.0-5.20081221
- Development tag.

* Sun Dec 21 2008 Uwe Kubosch <uwe@kubosch.no> - 0.5.0-4.20081221
- Updated to upstream trunk of 2008-12-21
- Added config file in /etc/sysconfig/zfs
- Added config option ZFS_AUTOMOUNT=0|1 to mount filesystems at boot

* Tue Nov 11 2008 Uwe Kubosch <uwe@kubosch.no> - 0.5.0-3.20081009
- Rebuild after import into Fedora build system.

* Thu Oct 09 2008 Uwe Kubosch <uwe@kubosch.no> - 0.5.0-2.20081009
- Updated to upstream trunk of 2008-10-09
- Adds changes to make zfs-fuse build out-of-the-box on Fedora 9,
  and removes the need for patches.

* Sat Oct  4 2008 Terje Rosten <terje.rosten@ntnu.no> - 0.5.0-1 
- initial build
