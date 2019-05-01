#!/bin/bash

for arg in $@; do
  clang-format -style=file -i "$arg"
  command -v dos2unix >/dev/null 2>&1 && dos2unix "$arg"
done
