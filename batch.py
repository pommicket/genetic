# Sample batch fitness algorithm
# fitness = sum(genes)
# Run method: python batch.py "%s"

import sys

gene_lists = sys.argv[1].split(";")
genes = map(lambda genes: map(float, genes.split(",")), gene_lists)
fitnesses = map(sum, genes)
print ",".join(map(str, fitnesses))
