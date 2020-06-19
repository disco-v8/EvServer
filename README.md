EvServer
=========

EvServer is the basic skeleton of a server program that uses libev and OpenSSL. Written in C language.

Sources, bug tracking: <https://github.com/disco-v8/EvServer>

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

	$ git clone https://github.com/disco-v8/EvServer.git
	$ cd EvServer

	$ autoheader
	$ aclocal
	$ automake --add-missing --copy
	$ autoconf

	$ make clean
	$ ./configure --prefix=/usr
	$ make

Regards.

T.Kabu/MyDNS.JP
