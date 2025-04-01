#!/bin/bash

# Check for undocumented public methods in C++ files
undocumented=$(grep -r "^\s*public:" --include="*.h" . | xargs grep -B 1 -A 2 "^\s*[a-zA-Z0-9_]\+ [a-zA-Z0-9_]\+(" | grep -v "/\*\*" | grep -v "///")

if [ ! -z "$undocumented" ]; then
  echo "Error: Found undocumented public methods:"
  echo "$undocumented"
  echo "Please add Doxygen comments to these methods."
  exit 1
fi

# Run Doxygen with warnings as errors to catch documentation issues
doxygen -q Doxyfile 2>&1 | grep "warning:" > doxygen_warnings.txt

if [ -s doxygen_warnings.txt ]; then
  echo "Error: Doxygen generated warnings:"
  cat doxygen_warnings.txt
  rm doxygen_warnings.txt
  exit 1
fi

rm -f doxygen_warnings.txt
exit 0
