# Sample fitness algorithm (non-batch)
# fitness = sum(genes)
# Run method: python fitness.py %s

import sys

genes = map(float, sys.argv[1].split(","))
print sum(genes)
