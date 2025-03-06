#!/bin/bash

# Wrapper script to install and build the www files. Required for nvm to work correctly.

[ -s "$NVM_DIR/nvm.sh" ] && \. "$NVM_DIR/nvm.sh" # Source nvm if required

# Capture the first two arguments and shift them out
DIR="$1"
CMD="$2"
shift 2
EXTRA_ARGS="$@"

# Navigate to the correct directory and run the necessary commands
cd "$DIR" || exit
"$CMD" install
"$CMD" build $EXTRA_ARGS # Pass any extra arguments to the build command
