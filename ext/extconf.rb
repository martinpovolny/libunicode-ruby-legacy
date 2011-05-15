require 'mkmf'

$CPPFLAGS << ' ' << `icu-config --cppflags`.chomp << ' '
$CFLAGS   << ' ' << `icu-config --cflags`.chomp << ' '
$LDFLAGS  << ' ' << `icu-config --ldflags`.chomp << ' '

# have_library("icu", "??")

create_makefile("unicode")

