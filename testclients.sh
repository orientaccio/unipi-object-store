#!/bin/bash
echo "Start test 1" >> testout.log;
for i in `seq 1 50`; 
do 
    ./oclient client$i 1 2>> testout.log; 
done;
echo "End test 1" >> testout.log;

echo "Start test 2" >> testout.log;	
for i in `seq 1 30`;     
do 
    ./oclient client$i 2 2>> testout.log; 
done;
echo "End test 2" >> testout.log;

echo "Start test 3" >> testout.log;
for i in `seq 31 50`; 
do 
    ./oclient client$i 3 2>> testout.log; 
done;
echo "End test 3">> testout.log;
