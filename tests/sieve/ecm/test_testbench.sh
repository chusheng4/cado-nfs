#!/usr/bin/env bash

TESTBENCH="$1"
INPUTFILE="$2"
shift 2

REQUIRED_OUTPUT="`mktemp`"
INPUTNUMBERS="`mktemp`"
sed "s/ *#.*$//" < "${INPUTFILE}" | grep . > "${REQUIRED_OUTPUT}" || exit 1
cat "${REQUIRED_OUTPUT}" | tr " " "*" | bc >> "${INPUTNUMBERS}" || exit 1

ACTUAL_OUTPUT="`mktemp`"
"${TESTBENCH}" -inp "${INPUTNUMBERS}" "$@" >> "${ACTUAL_OUTPUT}"

if ! cmp "${REQUIRED_OUTPUT}" "${ACTUAL_OUTPUT}"
then
  echo "testbench produced output in file \"${ACTUAL_OUTPUT}\", but expected result as in \"${REQUIRED_OUTPUT}\""
  exit 1
fi

rm -f "${REQUIRED_OUTPUT}" "${INPUTNUMBERS}" "${ACTUAL_OUTPUT}"
exit 0