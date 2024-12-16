#!/bin/bash

echo "benchmarking monte carlo method with k path and n iteration"


for((i=100;i<100000000 ; i*=10))
do
    monte_carlo=$(./monte-carlo.exe 365 $i 100)
    echo -e "i=${i} price=${monte_carlo} \n"
done



