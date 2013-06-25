$ sudo apt-get install libelf-dev php5-dbg
[...]

$ pmake re
$ LD_PRELOAD=$PWD/libpreload-accept.so /usr/bin/php5-cgi -b 12349
ACCEPT HOOKED!
BuildID : 0x8985c01da7c3a193ee940e929d05eab8aa091416
Sym: fp_in_shutdown/0x762fe0 fp_terminate/0x762ff0
shutdown ? 0
shutdown ? 1
