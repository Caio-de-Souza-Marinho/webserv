#!/bin/bash

valgrind \
--leak-check=full \
--show-leak-kinds=all \
--track-fds=yes \
--log-file=/tmp/webserv_valgrind.log \
./webserv config/default.conf &
VPID=$!

sleep 2

curl -s http://localhost:8080/ > /dev/null

kill -TERM $VPID

wait $VPID

grep "definitely lost: 0 bytes" /tmp/webserv_valgrind.log
