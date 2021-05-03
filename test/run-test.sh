#!/bin/bash

echo "Creating directory test-out/$2"
mkdir -p "test-out"
mkdir "test-out/$2"

echo "Running $1 with output redirected to test-out/$2/run.log"
"$1" &> "test-out/$2/run.log"

echo "Running Valgrind test..."
valgrind --leak-check=full --track-origins=yes --xml=yes --log-file="test-out/$2/valgrind-log.log" --xml-file="test-out/$2/valgrind.xml" "$1" > "test-out/$2/run-valgrind.log"

echo "Running Massif test..."
valgrind --tool=massif --massif-out-file="test-out/$2/massif.out" --stacks=yes "$1" > "test-out/$2/run-massif.log"
