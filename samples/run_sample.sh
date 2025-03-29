#!/bin/bash

# This script demonstrates how to use dirdoc on the sample project

# Ensure we're in the project root directory
SCRIPT_DIR=$(dirname "$0")
cd "$SCRIPT_DIR" || exit

# Build the dirdoc binary if it doesn't exist
if [ ! -f "../build/dirdoc" ]; then
    echo "Building dirdoc..."
    cd .. && make
    cd "$SCRIPT_DIR" || exit
fi

echo "Running dirdoc on the sample project..."
../build/dirdoc "./sample_project"

echo ""
echo "Sample documentation generated!"
echo "You can find the output file at: sample_project_documentation.md"
