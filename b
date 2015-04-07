#!/bin/bash
# build script for LocalChat, includes debugging flags

gcc -pg -o client -g client.c user.c userTable.c -pthread
