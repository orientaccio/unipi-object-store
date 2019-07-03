#!/bin/bash
echo "Start Test 1" >> testout.log;
for i in `seq 1 50`; 
do 
    ./oclient clientn$i 1 2>> testout.log; 
done;
echo "End Test 1" >> testout.log;

echo "Start Test 2" >> testout.log;	
for i in `seq 1 30`;     
do 
    ./oclient clientn$i 2 2>> testout.log; 
done;
echo "End test 2" >> testout.log;

echo "Start Test 3" >> testout.log;
for j in `seq 31 50`; 
do 
    ./oclient clientn$j 3 2>> testout.log; 
done;
echo "End test 3">> testout.log;
