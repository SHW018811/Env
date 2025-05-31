#!/bin/bash
# rebuild.sh

echo -e "\nmake clean..."
make clean

echo "make..."
make

if [ $? -eq 0 ]; then
    echo -e "🍀🍀Build complete🍀🍀\n"
else
    echo -e "❌❌Build failed!❌❌\n"
    exit 1
fi
