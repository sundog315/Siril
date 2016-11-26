#!/bin/sh
# Run this to generate all the initial makefiles, etc.
srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

(test -f $srcdir/configure.ac) || {
        echo "**Error**: Directory "\`$srcdir\'" does not look like the top-level project directory"
        exit 1
}

PKG_NAME=`autoconf --trace 'AC_INIT:$1' "$srcdir/configure.ac"`

set -x
aclocal --install || exit 1
intltoolize --force --copy --automake || exit 1
autoreconf --verbose --force --install -Wno-portability || exit 1

{ set +x; } 2>/dev/null

echo -e "\n$PKG_NAME preparation finished."
