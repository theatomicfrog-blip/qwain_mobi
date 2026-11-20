#!/bin/bash
while [ ! -f build/Qwain ]; do sleep 1; done
xvfb-run ./build/Qwain --diag > /tmp/qwain.log 2>&1 &
sleep 5
killall Qwain
cat /tmp/qwain_logo_diag.txt
cat /tmp/qwain.log
