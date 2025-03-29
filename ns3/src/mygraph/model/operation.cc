#include "operation.h"

#define PI 3.14159265358979323846
#define RADIO_TERRESTRE 6372797.56085
#define GRADOS_RADIANES PI / 180

#include "ns3/core-module.h"
#include <math.h>
#include <algorithm>
#include <chrono>  // chrono::system_clock
#include <ctime>   // localtime
#include <iomanip> // put_time
#include <time.h>
// #include "ns3/type.h"
namespace ns3{

float CalcGPSDistance(float latitud1, float longitud1, float latitud2, float longitud2){
    double haversine;
    double temp;
    double distancia_puntos;

    latitud1  = latitud1  * GRADOS_RADIANES;
    longitud1 = longitud1 * GRADOS_RADIANES;
    latitud2  = latitud2  * GRADOS_RADIANES;
    longitud2 = longitud2 * GRADOS_RADIANES;

    haversine = (pow(sin((1.0 / 2) * (latitud2 - latitud1)), 2)) + ((cos(latitud1)) * (cos(latitud2)) * (pow(sin((1.0 / 2) * (longitud2 - longitud1)), 2)));
    temp = 2 * asin(std::min(1.0, sqrt(haversine)));
    distancia_puntos = RADIO_TERRESTRE * temp;

   return distancia_puntos;
}

void write_time(std::string filename, ServerlessConfig config){

    // std::string filename = "result/my_result.csv";

    std::ofstream outfile;

    outfile.open(filename, std::ofstream::app);//append mode
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss; 

    outfile <<std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X") << std::endl;

    outfile <<  "Toponame" << ","  << config.TopoName <<std::endl;
    outfile <<  "RequestFile" << ","  << config.RequestFile <<std::endl;
    outfile <<  "LatencyPara" << ","  << config.LatencyPara <<std::endl;
    outfile <<  "MemCap" << ","  << config.MemCap <<std::endl;
    outfile <<  "NodeNum" << ","  << config.NodeNum <<std::endl;
    outfile <<  "Beta" << ","  << config.Beta <<std::endl;
    outfile <<  "SlotNum" << ","  << config.SlotNum <<std::endl;
    outfile <<  "ReduFactor" << ","  << config.ReduFactor <<std::endl;
    outfile <<  "predictRequestFile" << ","  << config.predictRequestFile <<std::endl;
     //result [id, linkdelay, processingdelay, coldstartdelay, iscoldstart, total delay]
    outfile <<"timeslot" << ","<< "id"<<","<<"type" <<"," <<"linkdelay"<<","<<"pdelay"<<","<<"cdelay"<<","<<"iscold"<<"," <<"totaldelay"<<std::endl;

    outfile.close();
}

void write_result_title(std::string filename){
    std::ofstream outfile;

    outfile.open(filename, std::ofstream::app);//append mode


    outfile <<"coldNum"<< ","<< "servedNum"<< ","<< "totalNum"<< "," <<"coldfreq"<< ","<<"avgdelay"<< ","<< " unusednum " <<","<< "unusedmem"<<std::endl;

    outfile.close();

}

void write_vector_file(std::string filename, std::vector<float> data = std::vector<float>())
{
    // std::string filename = "result/my_result.csv";
    std::ofstream outfile;

    outfile.open(filename, std::ofstream::app);//append mode

    
    if (outfile.is_open())

    {  

        for(auto it = data.begin(); it != data.end(); it++)
        {   
            outfile << *it;
            if(it != data.end()-1){
                outfile << ",";
            }
        }
        outfile<<std::endl;
        
        outfile.close();
        
    }
    else
    {
       std::cout<< "failes to open file " << filename << std::endl;
    }
}



}//end ns3 namespace