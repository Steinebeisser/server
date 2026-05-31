#!/usr/bin/env bash

set -euo pipefail

port=2022
binary=./server_release
duration=60
out=perf.data
freq=999
# ip=127.0.0.1
ip=localhost
url="http://${ip}:${port}/"

echo "[1/3] building profile binary"
make release

echo "[2/3] starting server on port: ${port}"
"${binary}" -p "${port}" 1>/dev/null &
server_pid=$!
sleep 1

function cleanup {
    echo "Killing server"
    kill -9 "$server_pid"
}

trap cleanup EXIT INT TERM

echo "[3/3] running wrk load against ${url}"
wrk -t4 -c500 -d"${duration}s" "${url}"

