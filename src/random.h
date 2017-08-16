#include <stdlib.h>

double rand01()
{
    return (double) rand() / RAND_MAX;
}

double randrange(double start, double end)
{
    return (end-start) * rand01() + start;
}

int randrange_int(int start, int end)
{
    return (int) randrange((double) start, (double) end);
}
