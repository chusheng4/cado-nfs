#! /bin/bash
k1om_rpm=/home/mpss-3.4.6/k1om/
filetypes="python-core*.rpm libpython2.7-1.0*.rpm python-lang*.rpm python-re*.rpm python-readline*.rpm python-modules*.rpm python-2to3*.rpm python-audio*.rpm python-bsddb*.rpm python-codecs*.rpm python-compile*.rpm python-compiler*.rpm python-compression*.rpm python-zlib*.rpm python-crypt*.rpm openssl-1.0.0r-r15.1.k1om.rpm python-ctypes*.rpm python-curses*.rpm libpanel5-5.9-r8.1.k1om.rpm python-datetime*.rpm python-db*.rpm python-debugger*.rpm python-io*.rpm python-pprint*.rpm python-math*.rpm python-shell*.rpm python-stringold*.rpm python-difflib*.rpm python-doctest*.rpm python-unittest*.rpm python-elementtree*.rpm python-email*.rpm python-image*.rpm python-mime*.rpm python-netclient*.rpm python-logging*.rpm python-pickle*.rpm python-fcntl*.rpm python-gdbm*.rpm libgdbm4-1.10-r3.k1om.rpm python-hotshot*.rpm python-html*.rpm python-idle*.rpm python-tkinter*.rpm python-json*.rpm python-mailbox*.rpm python-mmap*.rpm python-multiprocessing*.rpm python-netserver*.rpm python-numbers*.rpm python-pkgutil*.rpm python-profile*.rpm python-textutils*.rpm python-pydoc*.rpm python-robotparser*.rpm python-smtpd*.rpm python-sqlite3*.rpm libsqlite3*.rpm  pkgconfig*.rpm python-threading*.rpm python-subprocess*.rpm python-syslog*.rpm python-terminal*.rpm python-tests*.rpm python-unixadmin*.rpm python-xml*.rpm wget-1*.rpm python-distutils*.rpm libz-dev-*.rpm libsqlite3-*.rpm libsqlite3-dev-*.rpm perl-5.14.2-r7.k1om.rpm libperl5-*.rpm perl-module-posix-sigrt-*.rpm perl-module-corelist-*.rpm perl-module-data-dumper-*.rpm perl-module-fcntl-*.rpm perl-module-file-basename-*.rpm perl-module-file-temp-*.rpm perl-module-list-util-*.rpm perl-module-posix-*.rpm perl-module-posix-sigaction-*.rpm perl-module-xsloader-*.rpm perl-module-exporter-*.rpm perl-module-overload-*.rpm perl-module-symbol-*.rpm perl-module-autoloader-*.rpm perl-module-tie-hash-*.rpm perl-module-carp-*.rpm perl-module-file-spec-*.rpm perl-module-file-path-*.rpm perl-module-cwd-*.rpm perl-module-io-seekable-*.rpm perl-module-io-handle-*.rpm perl-module-selectsaver-*.rpm perl-module-io-5.14.2-r7.k1om.rpm perl-module-errno-*.rpm perl-module-base-*.rpm perl-module-constant-*.rpm perl-module-exporter-heavy*.rpm perl-module-bytes*.rpm"

mics="mic0 mic1 mic2 mic3 mic4 mic5 mic6 mic7"
for mic in $mics
do
echo install python2 on $mic
ssh root@$mic "cd $k1om_rpm ; rpm -ihv $filetypes"
done

for mic in $mics
do
echo verify python2 on $mic
ssh root@$mic python --version
done
