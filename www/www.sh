#!/bin/bash

# Wrapper script to install and build the www files. Required for nvm to work correctly.

[ -s "$NVM_DIR/nvm.sh" ] && \. "$NVM_DIR/nvm.sh" # Source nvm if requried

# Navigate to the correct directory and run the necessary yarn commands
cd "$1" || exit
"$2" install
"$2" build
