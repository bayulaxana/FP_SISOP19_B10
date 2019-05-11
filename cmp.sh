#!/bin/bash
gcc -Wall `pkg-config fuse --cflags` $1 -o file `pkg-config fuse --libs`