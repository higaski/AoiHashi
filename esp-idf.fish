#!/bin/fish
if test -n "$IDF_PATH"
  return 0
end

set CWD $PWD
export IDF_PATH=$HOME/esp/esp-idf
. $HOME/esp/esp-idf/export.fish
cd $CWD