#ifndef ZIPF_H
#define ZIPF_H


#include <string>
using namespace ns3;

namespace ns3{
//----- Constants -----------------------------------------------------------
#define  FALSE          0       // Boolean false
#define  TRUE           1       // Boolean true

class Zipf_generator{

    public:
    //----- Function prototypes -------------------------------------------------
    int      zipf(double alpha, int n);  // Returns a Zipf random variable
    double   rand_val(int seed);         // Jain's RNG

    int first = TRUE;      // Static first time flag
    double c = 0;          // Normalization constant
    double *sum_probs;     // Pre-calculated sum of probabilities
    long x;               // Random int value
};


}//end namespace ns3
#endif /* ZIPF_H */
