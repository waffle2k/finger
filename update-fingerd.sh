#!/bin/sh
set -e

bastille cmd finger sh -c '
  cd /usr/local/src/finger &&
  git pull &&
  meson compile -C builddir &&
  meson install -C builddir
'

bastille cmd finger service fingerd restart
