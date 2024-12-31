## Calculating stock option using monte carlo

#### this program uses openmp to calculate monte-carlo stock prices in many iterations

- please compile the project first

```
    gcc -Wall -fopenmp -o monte-carlo ./monte-carlo.c -lm
```

NOTE: after compilation a change could be needed inside analyze.py file

##### if compiling was done in windows, no change is needed, however, if the file is compiled in linux

##### please make sure to use the right prefex of the subprocess call inside analyze.py

##### you can check different params of monte-carlo using -h argument

##### to do the same thing in analyze.py you can call analyze.py -h

###### all plots that could be benifitial are extracted from analyze.py

A powerpoint showing main extracted insights was added to the project

feel free to ask any questions [Ali Saleem Hasan](https://www.linkedin.com/in/ali-saleem-hasan/)
