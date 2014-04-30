#!/bin/bash

EXCLUDES="-e org.springframework"
CLASSROOT=$1
JAVAROOT=$2
PACKAGE=$3
UNIT=$4

rm -f testdata/deps/${PACKAGE}/${UNIT}.d testdata/origdeps/${PACKAGE}/${UNIT}.d
echo jdep ${EXCLUDES} -d testdata/deps -c ${CLASSROOT} -j ${JAVAROOT} ${CLASSROOT}/${PACKAGE}/${UNIT}.class
jdep ${EXCLUDES} -d testdata/deps -c ${CLASSROOT} -j ${JAVAROOT} ${CLASSROOT}/${PACKAGE}/${UNIT}.class
origjdep ${EXCLUDES} -d testdata/origdeps -c ${CLASSROOT} -j ${JAVAROOT} ${CLASSROOT}/${PACKAGE}/${UNIT}.class
diff -q testdata/deps/${PACKAGE}/${UNIT}.d testdata/origdeps/${PACKAGE}/${UNIT}.d

