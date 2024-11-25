#!/bin/bash
echo "Compiling"
g++ book.cpp -std=c++14 -lpthread -lcpprest -lcrypto -lssl
echo "Compiled"
exit 0
