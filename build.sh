#!/bin/bash

code="$PWD"
app_name="lith_lobby"

WARNINGS="-Wall -Werror -Wshadow -Wno-unused-value -Wno-missing-braces -Wno-unused-variable -Wno-unused-function  -Wno-write-strings"

link_opts="-pthread -no-pie"
opts="-g -DDEBUG_BUILD=0"
mkdir -p build
cd build > /dev/null

gcc $WARNINGS $opts $link_opts -I$code/code $code/code/test/test_main.c -o lobby_test
gcc $WARNINGS $opts $link_opts -o3 -I$code/code $code/code/os/linux/linux_main.c -o $app_name

cd $code > /dev/null

