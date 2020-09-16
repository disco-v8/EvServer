Ver0.0.x
=========

These files in this folder are older versions.

Building
---------

EvServer depends on few things to get compiled...

* [CentOS Linux] 7+
* [GNU Make] 3.82+
* [GNU Automake] 1.13+
* [Libev] 4.15+
* [OpenSSL] 1.0.2+

[CentOS Linux]: https://www.centos.org/
[GNU Make]: https://www.gnu.org/software/make/
[GNU Automake]: https://www.gnu.org/software/automake/
[Libev]: http://software.schmorp.de/pkg/libev.html
[OpenSSL]: https://www.openssl.org/

etc ...

When dependencies are installed just run:

    1) Download and compile, link.

    $ git clone https://github.com/disco-v8/EvServer.git
    $ cd EvServer

    $ autoheader
    $ aclocal
    $ automake --add-missing --copy
    $ autoconf

    $ make clean
    $ ./configure --prefix=/usr
    $ make

    2) Make PID, SockFile, Log's directorys.

    $ mkdir /var/run/EvServer/
    $ chown userid.groupid /var/run/EvServer/
    $ mkdir /var/log/EvServer/
    $ chown userid.groupid /var/log/EvServer/

    3) Edit INI file, and run.

    $ ./evserver (./evserver.ini)
    $ tail -F /var/log/EvServer/evserver.log

Regards.

T.Kabu/MyDNS.JP
