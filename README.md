# Genetic Algorithm Framework

This is a framework for creating genetic algorithms. To use it, create a program which takes in a list of genes, and outputs a fitness for those genes.

If batch gene proccessing is off, the input will be provided as a comma-separated list of numbers from 0 to 1. If it's on, it will be provided as a semicolon-separated list of comma-separated lists of numbers from 0 to 1.

`fitness.py` is an example of a non-batch function. If you run `python fitness.py 0.4,0.5,0.6`, it will output 1.7 (0.4 + 0.5 + 0.6). So, if you set the run method to `python fitness.py %s`, it will maximize the sum of the genes (bringing all of them slowly to 1).

`batch.py` is an example of a batch function. If you run `python batch.py "0.3,0.4;0.5,0.6"`, it will output `0.7,1.1`. This is identical to `fitness.py`, but since it can process multiple sets of genes at once, it will have better performance. To use it, set the run method to `python batch.py "%s"`, and turn batch genes on.
