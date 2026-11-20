#!/bin/bash
echo "Running AdifMonitor under gdb to catch the exit crash..."
gdb -batch -ex "run" -ex "bt" -ex "quit" --args ./AdifMonitor > adifmonitor_backtrace.txt 2>&1
echo "Done. Please provide the contents of adifmonitor_backtrace.txt"
