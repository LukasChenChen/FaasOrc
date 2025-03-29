#ifndef OPERATION_H
#define OPERATION_H

#include "ns3/core-module.h"
#include "ns3/type.h"

using namespace ns3;

namespace ns3{
float CalcGPSDistance(float latitud1, float longitud1, float latitud2, float longitud2);

void write_vector_file(std::string filename, std::vector<float> data);

void write_time(std::string filename, ServerlessConfig config);

void write_result_title(std::string filename);

} //end namespace ns3

#endif /* OPERATION_H */