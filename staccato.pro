#-------------------------------------------------
# Top-level SUBDIRS orchestrator.
# Use this .pro to build the lib, the app, and the tests in dependency order:
#   qmake staccato.pro && nmake
#
# Loja.pro at the repo root still works on its own (it expects libstaccato.lib
# to be present under libstaccato/<release|debug>/), but new contributors
# should prefer staccato.pro for a one-shot build.
#-------------------------------------------------

TEMPLATE = subdirs

SUBDIRS = libstaccato Loja tests

libstaccato.file = libstaccato/libstaccato.pro

Loja.file = Loja.pro
Loja.depends = libstaccato

tests.file = tests/tests.pro
tests.depends = libstaccato

CONFIG += ordered
