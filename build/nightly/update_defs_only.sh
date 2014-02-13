#!/bin/sh
# assume $WK is defined
# Alter the command below to either
# a/ use the system installed version, everywhere, avoid miss-match between different releases
# b/ Test the latest release, requires compatible client/server versions

set -u # fail when using an undefined variable
set -x # echo script lines as they are executed

cd $SCRATCH

# Generate the defs
python $WK/build/nightly/build.py

# load the generated defs, *ASSUMES* server is running
python $WK/build/nightly/load.py
