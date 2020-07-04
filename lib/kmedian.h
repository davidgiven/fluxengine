#ifndef KMEDIAN_H
#define KMEDIAN_H

typedef float KValue;

extern std::vector<KValue> optimalKMedian(const std::vector<KValue>& points, int clusters);
extern std::vector<unsigned> optimalKMedian(const Fluxmap& fluxmap, int clusters);

#endif

