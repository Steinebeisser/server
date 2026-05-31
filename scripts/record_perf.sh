#!/usr/bin/env bash

set -euo pipefail

port=2022
binary=./server_profile
duration=60
out=perf.data
freq=999
ip=localhost
url="http://${ip}:${port}/"

echo "[0/5] requesting sudo"
sudo -v

echo "[1/5] building profile binary"
make profile

echo "[2/5] starting server on port: ${port}"
"${binary}" -p "${port}" 1>/dev/null &
server_pid=$!
sleep 1

if ! kill -0 "$server_pid" 2>/dev/null; then
  echo "Server failed to start"
  exit 1
fi

function cleanup {
    echo "Killing server"
    kill -9 "$server_pid"
}

trap cleanup EXIT INT TERM


echo "[3/5] recording ${duration}s to ${out}"
sudo perf record -F "${freq}" -g -o "${out}" -p "${server_pid}" -- sleep "${duration}" &
perf_pid=$!
sleep 0.5

echo "[4/5] running wrk load against ${url}"
wrk -t4 -c500 -d"${duration}s" "${url}"

echo "[5/5] wrote ${out}"
