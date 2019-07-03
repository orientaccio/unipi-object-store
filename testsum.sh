#!/bin/bash
filename='testout.log'

#counters
test1_counter=0
test2_counter=0
test3_counter=0
#limits
test1_max=1000
test2_max=30
test3_max=20

nclient=0
test_n=0
n=1

while read line; 
do
    if [ -n "$line" ]; then
        if [ "$line" == "Start Test 1" ] ;
            then    test_n=1
        fi
        if [ "$line" == "Start Test 2" ] ;
                then    test_n=2
        fi
        if [ "$line" == "Start Test 3" ] ;
                    then     test_n=3
        fi
        
        # switch case
        case "$test_n" in
            1)  if [ "$line" == "RESPONSE: OK" ]; then
                    test1_counter=$((test1_counter + 1))
                fi
                ;;
            2) if [ "$line" == "Test2 OK" ]; then
                    test2_counter=$((test2_counter + 1))
                fi
                ;;
            3) if [ "$line" == "Test3 OK" ]; then
                    test3_counter=$((test3_counter + 1))
                fi
                ;;
        esac
        
        # client launched
        if [ "$line" == "OK" ] ;
            then nclient=$((nclient+1))
        fi        
    fi
    n=$((n+1))
done < $filename

# printing the results
echo "Client launched: $nclient"
echo "========== STATUS TEST 1 =========="
echo "  Success: $test1_counter "
echo "  Fail: $((test1_max - test1_counter))"
echo "========== STATUS TEST 2 =========="
echo "  Success: $test2_counter "
echo "  Fail: $((test2_max - test2_counter))"
echo "========== STATUS TEST 3 =========="
echo "  Success: $test3_counter "
echo "  Fail: $((test3_max - test3_counter))"

if [ $test1_counter == 1000 ] && [ $test2_counter == 30 ] && [ $test3_counter == 20 ] ;
    then 
        echo "Test completed"
    else
        echo "Test failed"
fi

# trigger the signal
BPID="$(pidof oserver)"
kill -SIGUSR1 $BPID      
