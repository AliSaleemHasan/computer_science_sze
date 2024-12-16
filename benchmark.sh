#!/bin/bash

echo "benchmarking monte carlo method with k path and n iteration"


for((i=100;i<10000 ; i+=100))
do
    monte_carlo=$( ./monte-carlo.exe $i )
    echo -e "i=${i} price=${monte_carlo} \n"
done



