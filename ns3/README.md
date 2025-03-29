# NS3 simulation
## tested on ns3 version 3.29, disable not used warning incurred by boost lib
> CXXFLAGS="-Wno-error" ./waf configure \

# The main code are in myalgorithm.cc, fixedcaching.cc and hist.cc
> type.h defines data structure \
> network controller creates traffic \
> zipf.cc create zipf distribution \
> sfc_tag define the byteTag \

## How to run
> To run CFC, \
> run sh runmy.sh \

> To run fix, \
> run sh fc.sh \

> To run Hist
> run sh hist.sh \
