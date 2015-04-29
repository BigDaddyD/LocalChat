#!/bin/bash
# build script for LocalChat, includes debugging flags

gcc -pg -o client client.c user.c userTable.c -g -pthread
