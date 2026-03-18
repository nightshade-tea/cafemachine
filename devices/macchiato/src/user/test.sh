#!/bin/bash
# run_tests.sh

echo "--- RUNNING MMAP TEST ---"
./test_mmap

echo -e "\n--- RUNNING BOUNDS TEST ---"
./test_bounds

echo -e "\n--- RUNNING PIPELINE TEST ---"
./test_pipeline "rule eicar { strings: $a=\"EICAR\" condition: $a }" "some random EICAR string data"

echo -e "\n--- RUNNING CONCURRENCY TEST ---"
./test_mmap & 
./test_mmap &
wait
echo "[+] Concurrency test finished (one should fail with EBUSY)"
