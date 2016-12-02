#!/bin/sh
# Run this to generate all the initial makefiles, etc.
srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.
echo "Entering $srcdir for autogen"
cd "$srcdir"

(test -f configure.ac) || {
        echo "**Error**: Directory "\`$srcdir\'" does not look like the top-level project directory"
        exit 1
}

PKG_NAME=`autoconf --trace 'AC_INIT:$1' "configure.ac"`

mkdir -p po

set -x
aclocal --install || exit 1
intltoolize --force --copy --automake || exit 1
autoreconf --verbose --force --install -Wno-portability || exit 1

{ set +x; } 2>/dev/null

echo -e "\n$PKG_NAME preparation finished."
