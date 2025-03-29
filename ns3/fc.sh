#!/bin/sh
# run multiple experiments in batch process.
# the lobster server is 24 cores, each run will take a core.
nohup ./waf --run "scratch/test-fc --beta=0.5 --reduFactor=10 --simu=false" & 
sleep 10
nohup ./waf --run "scratch/test-fc --beta=0.5 --reduFactor=20 --simu=false" &
sleep 10
nohup ./waf --run "scratch/test-fc --beta=0.5 --reduFactor=30 --simu=false" &
sleep 10
nohup ./waf --run "scratch/test-fc --beta=0.5 --reduFactor=40 --simu=false" & 
sleep 10
nohup ./waf --run "scratch/test-fc --beta=0.5 --reduFactor=50 --simu=false" &
sleep 10
nohup ./waf --run "scratch/test-fc --beta=0.5 --reduFactor=60 --simu=false" &
sleep 10
nohup ./waf --run "scratch/test-fc --beta=1 --reduFactor=10 --simu=false" & 
sleep 10
nohup ./waf --run "scratch/test-fc --beta=1 --reduFactor=20 --simu=false" &
sleep 10
nohup ./waf --run "scratch/test-fc --beta=1 --reduFactor=30 --simu=false" &
sleep 10
nohup ./waf --run "scratch/test-fc --beta=1 --reduFactor=40 --simu=false" & 
sleep 10
nohup ./waf --run "scratch/test-fc --beta=1 --reduFactor=50 --simu=false" &
sleep 10
nohup ./waf --run "scratch/test-fc --beta=1 --reduFactor=60 --simu=false" &
sleep 10
nohup ./waf --run "scratch/test-fc --beta=1.5 --reduFactor=10 --simu=false" & 
sleep 10
nohup ./waf --run "scratch/test-fc --beta=1.5 --reduFactor=20 --simu=false" &
sleep 10
nohup ./waf --run "scratch/test-fc --beta=1.5 --reduFactor=30 --simu=false" &
sleep 10
nohup ./waf --run "scratch/test-fc --beta=1.5 --reduFactor=40 --simu=false" & 
sleep 10
nohup ./waf --run "scratch/test-fc --beta=1.5 --reduFactor=50 --simu=false" &
sleep 10
nohup ./waf --run "scratch/test-fc --beta=1.5 --reduFactor=60 --simu=false" &
sleep 10
echo "experiments all running now!"
