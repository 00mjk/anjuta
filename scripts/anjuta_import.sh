#!/bin/bash
#
# Shell script to automatically create an Anjuta 1.x project file
# Copyright: Biswapesh Chattopadhyay (biswapesh_chatterjee@tcscal.co.in) 2001-2002
# This file can be freely copied for any purpose. There are NO guarantees.
#

# Template file is in data dir, so:
# TEMPLATE="$(dirname $0)/anjuta_project.template"
TEMPLATE="/usr/share/anjuta/anjuta_project.template"

PROGRAM=anjuta
DIR=${1:-"."}
TMPFILE=/tmp/$PROGRAM.$$.tmp

if [ ! -s "${TEMPLATE}" ]; then
  echo "Template file ${TEMPLATE} does not exist!" >&2
  exit 1
fi
${PROGRAM} --version >${TMPFILE}
if [ $? -ne 0 ]; then
  echo "Anjuta not found !" >&2
  \rm -f ${TMPFILE}
  exit 1
fi
PROGRAM_VERSION=`cat $TMPFILE | awk '{print $NF}' | sed 's/\.//g' | cut -c1-3`
if [ "${PROGRAM_VERSION}" -lt  19 ]; then
  AN_INCLUDE_NAME="incude"
else
  AN_INCLUDE_NAME="include"
fi

if [ "$DIR" != '.' ]; then
  if [ ! -d "$DIR" ]; then
    echo "Directory $1 does not exist !" >&2
    \rm -f ${TMPFILE}
    exit 1
  fi
  if ! \cd $1; then
    echo "Unable to chdir to $1" >&2
    \rm -f ${TMPFILE}
    exit 1
  fi
fi

if [ -f "configure.in" ]; then
  package_line=$(grep AM_INIT_AUTOMAKE configure.in)
  if [ ! -z "${package_line}" ]; then
    AN_PROJECT_NAME=$(echo ${package_line} | awk -F'[(),]' '{print $2}')
    AN_PROJECT_VERSION=$(echo ${package_line} | awk -F'[(),]' '{print $3}')
  fi
fi

if [ -z "${AN_PROJECT_NAME}" ]; then
  AN_PROJECT_NAME=$(pwd | awk -F'/' '{print $NF}' | sed 's/-[0-9][^-]*$//')
  AN_PROJECT_VERSION=$(pwd | awk -F'/' '{print $NF}' | awk -F'-' '{if ((NF > 1) && ($NF ~ /^[0-9]/)) print $NF}')
fi

AN_PROJECT_NAME=$(echo ${AN_PROJECT_NAME} | sed 's/[ 	]//g' | sed 's/[^0-9a-zA-Z_]/_/g')
AN_PROJECT_VERSION=$(echo ${AN_PROJECT_VERSION} | sed 's/[ 	]//g')

if [ -z "${AN_PROJECT_NAME}" ]; then
  echo "WARNING!! Unable to derive project name. Using default value.." >&2
  AN_PROJECT_NAME="template"
fi
if [ -z "${AN_PROJECT_VERSION}" ]; then
  echo "WARNING!! Unable to derive project version. Using default value.." >&2
  AN_PROJECT_VERSION="0.99"
fi

PRJ_FILE="${AN_PROJECT_NAME}.prj"

if [ -f "${PRJ_FILE}" ]; then
  if ! mv ${PRJ_FILE} ${PRJ_FILE}.$$; then
    echo "Unable to backup existing project file! Exiting." >&2
    \rm -f ${TMPFILE}
    exit 1
  fi
  echo "WARNING!! Existing project file ${PRJ_FILE} backed up as ${PRJ_FILE}.$$" >&2
fi

AN_SOURCE_FILES=""
for file in `find . -name "*.c" -o -name "*.cpp" -o -name "*.cxx" -o -name "*.cc" -o -name "*.C"`
do
  AN_SOURCE_FILES="$AN_SOURCE_FILES `echo $file | cut -c3-`"
done

AN_HEADER_FILES=""
for file in `find . -name "*.h" -o -name "*.hpp" -o -name "*.H" -o -name "*.hh"`
do
  AN_HEADER_FILES="$AN_HEADER_FILES `echo $file | cut -c3-`"
done

AN_PIXMAP_FILES=""
for file in `find . -name "*.xpm" -o -name "*.png" -o -name "*.gif" -o -name "*.jpg"`
do
  AN_PIXMAP_FILES="$AN_PIXMAP_FILES `echo $file | cut -c3-`"
done

AN_DOC_FILES=""
for file in `find . -name "README" -o -name "INSTALL" -o -name "NEWS" -o -name "COPYING" -o -name "ChangeLog" \
	-o -name "CREDITS" -o -name "AUTHORS" -o -name "TAHNKS" -o -name "TODO" -o -name "HACKING" \
	-o -name "FUTURE" -o -name "BUGS" -o -name "*.sgml" -o -name "*.html"`
do
  AN_DOC_FILES="$AN_DOC_FILES `echo $file | cut -c3-`"
done

AN_PO_FILES=""
if [ -d "po" ]; then
  if \cd po; then
    for file in `find . -name "*.po"`
    do
      AN_PO_FILES="$AN_PO_FILES `echo $file | cut -c3-`"
    done
    \cd ..
  fi
fi

AN_BUILD_FILES=""
for file in `find . -name "Makefile.am" -o -name "Makefile.in" -o -name "configure.in" -o -name "Makefile" \
	-o -name "*.spec" -o -name "*.m4"`
do
  AN_BUILD_FILES="$AN_BUILD_FILES `echo $file | cut -c3-`"
done


cat ${TEMPLATE} | while read line; do
  eval "echo ${line}" >>${PRJ_FILE}
done

echo "Created project file ${PRJ_FILE} successfully."

\rm -f ${TMPFILE}
exit 0

