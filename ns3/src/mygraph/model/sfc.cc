/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "sfc.h"
#include <string>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include "ns3/log.h"
#include <boost/algorithm/string.hpp> // is_any_of


#include <random> // possion process
#include <iomanip>
#include <math.h>
#include <cmath>


NS_LOG_COMPONENT_DEFINE ("sfc");
const int G_TIME_SLOT = 300; //5min
const int G_END_TIME = 200; //MAX TIME SLOT NUM
const int G_INSTANCE_THRESHOLD = 400; // MAXIMUM INSTANCE CAPACITY

namespace sfc {

/* ... */
    
Flow Flows::get_flow(int index){

    return m_flows[index];

}

void Flows::add_flow(Flow f){

    m_flows.push_back(f);

    NS_LOG_DEBUG("add_flow " << m_flows.back().m_id <<" " << m_flows.back().m_traffic_rate);
}

int Flows::get_size(){

    return m_flows.size();
    
}

std::vector<Flow> Flows::get_flows()
{
    return m_flows;
}

void Flows::clear()
{
    m_flows.clear();
}


Sfc_vec::Sfc_vec(int number, int max_node_number, int chain_length, int flow_num, int max_flow_r, int min_flow_r , int max_t_slot)
{
    m_num = number;
    m_max_node = max_node_number;
    m_chain_len = chain_length;
    m_max_flow_num = flow_num;
    m_max_flow_rate = max_flow_r;
    m_min_flow_rate = min_flow_r;
    m_max_time = max_t_slot;

}

Chain Sfc_vec::create_chain(){
    int random_index;

    Chain chain;

    for (int i = 0; i < m_chain_len; i++)
    {   
        // generate a num from 0 to vnf_vec.size
        random_index = rand()%(vnf_vec.size());
        
        chain.push_back(vnf_vec[random_index]);
    }
    
    return chain;
}



void Sfc_vec::get_traffic_from_csv(){

    std::string file_name = "flow_size.csv";

    std::ifstream myFile("scratch/" + file_name);

    if(!myFile.is_open()) throw std::runtime_error("Could not open file");

    std::string line;

    double traffic_size;

    while(std::getline(myFile, line))
    {
        traffic_size = stod(line);

        //120 equals 400 processing capa
        if(0 < traffic_size < 120)// protect here, the traffic should not exceed the smallest instance capacity.  traffic rate < traffic size *1000/300
        {
            m_traffic_v.push_back(traffic_size);

            NS_LOG_INFO("read traffic rate " << traffic_size);
        }

        
    }

}



Flows Sfc_vec::create_flows(int r_start_time, int r_end_time){

    int id, traffic_rate, start_time, end_time;

    Flows flows = Flows();

    for (int i = 0; i < m_max_flow_num; i++){

        id = i;

        traffic_rate = m_min_flow_rate + rand()%(m_max_flow_rate); //[min_flow_rate,max_flow_rate)

        start_time = r_start_time + rand()%(r_end_time - r_start_time); //rand()%(max-min + 1) + min;  [min, max]

        end_time = r_end_time;


        Flow f;
        f.init(id, traffic_rate, start_time, end_time);

        flows.add_flow(f);
    }


    return flows;
}


void Sfc_vec::create_flows_from_csv(int r_start_time, int r_end_time){

    srand((unsigned)time(NULL));

    int id, start_time, end_time;

    double traffic_rate;

    id = 0;

    for (int i = 0 ; i < m_traffic_v.size(); i++){

        // 0 1 2 0 1 2 repeat
        if(id%m_max_flow_num == 0)
        {
            id = 0;
        }

        traffic_rate = 10000*m_traffic_v[i]/G_TIME_SLOT; // amplify 1000 times, unit is still kb

        if(traffic_rate == 0 || traffic_rate >= G_INSTANCE_THRESHOLD) {
            traffic_rate = G_INSTANCE_THRESHOLD - 1;
        }

        start_time = r_start_time + rand()%(r_end_time - r_start_time); //rand()%(max-min + 1) + min;  [min, max]

        end_time = start_time + 20;

        if(start_time > end_time)
        {
            int tmp_time = end_time;

            end_time = start_time;

            start_time = tmp_time;

        }


        Flow f;

        f.init(id, traffic_rate, start_time, end_time);

        m_flows.add_flow(f);

        id++;
    }

}


    // use a loop to output request.txt
void Sfc_vec::create_sfc(){
    std::cout << "---- create_sfc------"<<std::endl;
    std::ofstream outfile;
    outfile.open("scratch/requests.txt");
        
      
    int id;
    int src;
    int dst;
    int start_time;
    int end_time;
    std::vector<Flow> flows;

    srand((unsigned)time(NULL));

    for (int i = 1; i <= m_num; i++)
    {
        id = i;

        src = rand()%(m_max_node); //[0,max_node)

        dst = rand()%(m_max_node);

        Chain chain = create_chain();

        start_time = rand()%(m_max_time);

        end_time = m_max_time + rand()%4;  //max_time + 3

        Flows flows = create_flows(start_time, end_time);
    
        //src cant equal dst.

        
        if (src != dst)
        {

            sfc::Request r;

            r.init(id, src, dst, chain , flows, start_time, end_time);

            sfc_request_vec.push_back(r);

            outfile << "request="<<id<<","<<src<<","<<dst<<",";

            outfile << "[";

            for (Chain::iterator it = chain.begin(); it < chain.end(); it++)
            {
                outfile  <<(*it).m_id;
                if (it != chain.end()-1)
                {
                    outfile<<";";
                }
                
            }

            outfile << "]" <<",";


            for(int i=0; i < flows.get_size(); i++)
            {
                outfile << "[";
                outfile  << flows.get_flow(i).m_id;
                outfile  <<";";
                outfile  <<flows.get_flow(i).m_traffic_rate;
                outfile  <<";";
                outfile  <<flows.get_flow(i).m_start_time;
                outfile  <<";";
                outfile  <<flows.get_flow(i).m_end_time;
                outfile  <<"]";

            }
            outfile <<",";


            outfile<<start_time<<","<<end_time<<"\n";
        }
    }
        
    outfile.close();

    std::cout << "---- create_sfc finish------"<<std::endl;



}



//get m_max_flow_num flows such as 0 , 1, 2 then 3 ,4 ,5
Flows Sfc_vec::get_flows(int index){
    
    Flows flows;

    int tmp_index = m_max_flow_num*index;

    for(int i = 0; i < m_max_flow_num; i++)
    {
        flows.add_flow(m_flows.m_flows.at(tmp_index + i));
    }
    return flows;
}

std::pair<int, int> Sfc_vec::get_max_min_time(Flows flows)
{   
    int max_time = 0;
    int min_time = 9999999;

    for(auto it = flows.m_flows.begin(); it != flows.m_flows.end(); it++){
        if((*it).m_start_time < min_time){
            min_time = (*it).m_start_time;
        }

        if((*it).m_end_time > max_time){
            max_time = (*it).m_end_time;
        }

    }

    return std::make_pair(max_time, min_time);

}


//create sfc requests from flow_size.csv
void Sfc_vec::create_sfc_from_csv(){
    std::cout << "---- create_sfc from csv------"<<std::endl;
    std::ofstream outfile;
    outfile.open("scratch/requests.txt");

        
    int id;
    int src;
    int dst;
    int start_time = 1;
    int end_time;
    // std::vector<Flow> flows;

    srand((unsigned)time(NULL));

    //read data
    get_traffic_from_csv();

    end_time = G_END_TIME;  // [1-1000]

    create_flows_from_csv(start_time, end_time);

    for (int i = 1; i <= m_num; i++)
    {
        id = i;

        src = rand()%(m_max_node); //[0,max_node)

        dst = rand()%(m_max_node);

        Chain chain = create_chain();

       

        //src cant equal dst.

        //protect
        if(i-1 >= m_traffic_v.size())
        {
            break;
        }
        //src cant equal dst.
        if (src != dst)
        {

            sfc::Request r;

            // get the min and max time of flows
            std::pair<int,int> max_min = get_max_min_time(get_flows(i-1));

            r.init(id, src, dst, chain , get_flows(i-1), max_min.second, max_min.first);

            sfc_request_vec.push_back(r);

            outfile << "request="<<id<<","<<src<<","<<dst<<",";

            outfile << "[";

            for (Chain::iterator it = chain.begin(); it < chain.end(); it++)
            {
                outfile  <<(*it).m_id;
                if (it != chain.end()-1)
                {
                    outfile<<";";
                }
                
            }

            outfile << "]" <<",";


            for(int j=0; j < get_flows(i-1).get_size(); j++)
            {
                outfile << "[";
                outfile  << get_flows(i-1).get_flow(j).m_id;
                outfile  <<";";
                outfile  <<get_flows(i-1).get_flow(j).m_traffic_rate;
                outfile  <<";";
                outfile  <<get_flows(i-1).get_flow(j).m_start_time;
                outfile  <<";";
                outfile  <<get_flows(i-1).get_flow(j).m_end_time;
                outfile  <<"]";

            }
            outfile <<",";


            outfile<<max_min.second<<","<<max_min.first<<"\n";
        }
    }
        
    outfile.close();

    std::cout << "---- create_sfc finish------"<<std::endl;
}


// create the sfc requests for one time slot.
void Sfc_vec::create_one_slot_requests(int request_num, int time_slot){
    std::ofstream outfile;
    outfile.open("scratch/requests.txt", std::ios_base::app);

    int id;
    int src;
    int dst;
    int start_time = time_slot;
    int end_time = time_slot + 20;



    int ran_flow_id;

    for(int i = 1; i <= request_num; i++){

        id = m_current_index+1;

        m_current_index = id;

        src = rand()%(m_max_node); //[0,max_node)

        dst = rand()%(m_max_node);

        Chain chain = create_chain();

        

         if (src != dst)
        {

            outfile << "request="<<id<<","<<src<<","<<dst<<",";

            outfile << "[";

            for (Chain::iterator it = chain.begin(); it < chain.end(); it++)
            {
                outfile  <<(*it).m_id;
                if (it != chain.end()-1)
                {
                    outfile<<";";
                }
                
            }

            outfile << "]" <<",";

            //random take the flows
            for(int j=0; j < m_max_flow_num; j++)
            {   
                ran_flow_id =  rand()%(m_traffic_v.size());

                double traffic_rate = m_traffic_v.at(ran_flow_id);

                traffic_rate = 10000*traffic_rate/G_TIME_SLOT; // amplify 1000 times, unit is still kb

                if(traffic_rate == 0 || traffic_rate >= G_INSTANCE_THRESHOLD) {
                    traffic_rate = G_INSTANCE_THRESHOLD - 1;
                }

                outfile << "[";
                outfile  << j;
                outfile  <<";";
                outfile  << int(traffic_rate);
                outfile  <<";";
                outfile  << start_time;
                outfile  <<";";
                outfile  << end_time;
                outfile  <<"]";

            }
            outfile <<",";


            outfile<<start_time<<","<<end_time<<"\n";
        }
    }

    outfile.close();

}

//get the theoritical all traffic rate
double Sfc_vec::get_throughput(){

    double throughput = 0;

    for(auto it = sfc_request_vec.begin(); it != sfc_request_vec.end(); it++){

        throughput += (*it).get_total_traffic();
    }

    return throughput;


}



//get the current request number for csv
void Sfc_vec::read_request_num(){
    std::string file_name = "poisson-2.csv";

    std::ifstream myFile("scratch/" + file_name);

    if(!myFile.is_open()) throw std::runtime_error("Could not open file");

    std::string line;

    int request_num = 0;

    while(std::getline(myFile, line))
    {
        request_num = stod(line);

        m_r_num_v.push_back(request_num);
        
    }

}

//create sfc requests from flow_size.csv
void Sfc_vec::create_sfc_poisson(){

    std::cout << "---- create_sfc from csv------"<<std::endl;
   
    srand((unsigned)time(NULL));

    //read data
    get_traffic_from_csv(); // m_traffic_v

    read_request_num(); // m_r_num_v

    m_current_index = 0;

    int r_num = 0;

    //iterate over the time slots
    for (int i = 1; i <= G_END_TIME; i++)
    {   
        double index = i;

        //every slot has the same request number
        r_num = m_r_num_v.at(ceil(index)-1);

        //round up 
        create_one_slot_requests( r_num, i);
       
    }

    std::cout << "---- create_sfc finish------"<<std::endl;
}







void print_request(std::vector<sfc::Request> request_vec){

    for(std::vector<sfc::Request>::iterator it = request_vec.begin(); it != request_vec.end(); it++)
    {
        Request r = *it;

        std::cout<< r.m_id <<","<< r.m_source <<"," << r.m_target<<",";

        for(auto it_2 = r.m_chain.begin(); it_2 != r.m_chain.end(); it_2++)
        {

            std::cout <<(*it_2).m_id<<";";
        }

        Flows flows = r.m_flows;

        for(int i=0; i  < flows.get_size(); i++){

            Flow f= flows.get_flow(i);

            std::cout << " " << f.m_id << ";" << f.m_traffic_rate <<";" << f.m_start_time << ";" << f.m_end_time<< ",";
        }

        
        std::cout<< r.m_start_time << "," << r.m_end_time<<std::endl;
    }
}



//convert string to chain
Chain string_to_chain(std::string chain_s)
{
    Chain chain;

    std::vector<std::string> split_chain_v;

    boost::split(split_chain_v, chain_s, boost::is_any_of(";[]"), boost::token_compress_on);

   
    for (std::vector<std::string>::iterator it = split_chain_v.begin(); it  != split_chain_v.end(); it++)
    {
      
        if(*it != "")
        {
            int tmp = stoi(*it); 

            chain.push_back(vnf_vec[tmp-1]);
        }
      

    }

    return chain;

}


Flows string_to_flow(std::string flows_s){

    std::vector<std::string> split_flow_v, split_flow;

    boost::split(split_flow_v, flows_s, boost::is_any_of("[]"), boost::token_compress_on);

    Flows flows = Flows();

    for (std::vector<std::string>::iterator it = split_flow_v.begin(); it  != split_flow_v.end(); it++)
    {
        if(*it == ""){

            continue;
        }
        std::string flow = *it;

        boost::split(split_flow, flow, boost::is_any_of(";"), boost::token_compress_on);


        int count = 1;

        int id, traffic_rate, start_time, end_time;

        for (std::vector<std::string>::iterator it_2 = split_flow.begin(); it_2 != split_flow.end(); it_2++)
        {   
            if(*it == ""){

                continue;
            }
            int tmp = stoi(*it_2);  

            if(count == 1){
                id = tmp;

            }
            else if (count == 2)
            {
                traffic_rate = tmp;
            }
            else if (count == 3)
            {
                start_time = tmp;
            }
            else if (count == 4)
            {
                end_time = tmp;
            }

            count++;

        }
        Flow f;
        
        f.init(id, traffic_rate, start_time, end_time);

        flows.add_flow(f);

        

    }

    return flows;


}

//duplicate requests by factor
//note that the sfc id is also duplicated
void duplicate_sfc(std::vector<Request>& request_vec, int request_num, double factor){
   
    int count = request_vec.size();

    std::vector<Request> new_request_vec = request_vec;

    int total_num = int(request_num * factor);

    if(factor <= 1){
        return;
    }
  
    while(count < total_num && count < 10000){

       
        for(auto it = request_vec.begin(); it != request_vec.end(); it++){
         

            if(count >= total_num || count > 10000)
            {
                request_vec = new_request_vec;
                return;
            }
           
            new_request_vec.push_back(*it);

            count++;
          
        }
    }

   
    request_vec = new_request_vec;
    return; 

}
    

// read the chain length 
// factor is how many requests to reuse, for example 2, then reuse the request for one more time
bool read_sfc(std::string file_name, std::vector<Request>& request_vec, int request_num, double factor){

    std::string line;

    std::ifstream myfile("scratch/" + file_name);

    std::cout <<"-----read sfc file-----"<< std::endl;

        

       
    if (myfile.is_open()) {
        
        while (getline (myfile, line)) 
        {
            std::string id_s, src_s, dst_s, chain_s, flow_s, start_time_s, end_time_s;

            
            size_t pos = line.find("=");

            if (pos == std::string::npos){

                std::cout << "can't find =" <<std::endl;
                return false;
            }
                  
            std::string request_s = line.substr(pos+1);

            std::vector<std::string> split_request_v;

            boost::split(split_request_v, request_s, boost::is_any_of(","), boost::token_compress_on);
            
            int count = 1;

            Chain chain;

            
            
                // for every string, we split it and then get the value
            for (std::vector<std::string>::iterator it = split_request_v.begin(); it  != split_request_v.end(); it++)
            {

             

                
                int i = count;
             
                if(i == 1)
                {
                    id_s = *it;
                    
                }else if(i == 2)
                {
                    src_s = *it;
                    
                }else if(i==3)
                {
                    dst_s = *it;
                    
                }else if( i == 4)//chain
                {   
                    chain_s = *it;
                    
                }
                else if(i ==5) // flows
                {
                    flow_s = *it;
                }
                else if(i ==6)
                {
                    start_time_s = *it;    
                }
                else if(i ==7)
                {
                    end_time_s = *it;  
                }
                count ++;
            }

            
          
            int id = stoi(id_s); 

           
            int src = stoi(src_s);
            
            int dst = stoi(dst_s);

            chain = sfc::string_to_chain(chain_s);

            Flows flows = sfc::string_to_flow(flow_s);

            int start_time = stoi(start_time_s);

            int end_time = stoi(end_time_s);
                
            Request r;
            r.init(id, src, dst, chain, flows, start_time, end_time);
            
            request_vec.push_back(r);

            if (request_vec.size() == request_num)
            {
                myfile.close();

                break;
            }
            
        }
            
           
               
    }
    else 
    { 
        std::cout << "Unable to open request file"; 
        return false;
    }

    // if (request_vec.size() < request_num && factor <= 1)
    // {
    //     myfile.close();

    //     // this is alright, if we want all request we can just give a number like 10000
    //     std::cout << "---- not enough sfc request------" << std::endl;
    //     return true;
    // }

    myfile.close();

    duplicate_sfc(request_vec,  request_num,  factor);

    std::cout <<"-----read sfc file finish-----"<< std::endl;

    return true;
}

static std::random_device rd;
static std::mt19937 gen(rd());
//create a number for each slot, that represent the request number
//1000 slot, each 100 slot has a mean value
// starts from 30, 40, 50 , 60, 70, 80,70,60,50,40

void possion_process(int average_value, int loop_num)
{ 
  std::string filename = "scratch/poisson-1.csv";
  std::ofstream outfile;
 
  std::vector<int> num_v;

  std::poisson_distribution<int> pd(average_value);

  int ran;

  for (int i = 0; i < loop_num; i++){

      
    ran =  pd(gen);

    // std::cout << ran << '\n';

    num_v.push_back(ran);
  }
  
  outfile.open(filename, std::ofstream::app);
    
    if (outfile.is_open())
    {   

        for(int i = 0; i < loop_num; i++){
            outfile << num_v.at(i);
            outfile << std::endl;
        }

       
        outfile.close();
    }

    
}

// void possion_process(int p, int s){

//     // uncomment to use a non-deterministic generator
//     //    std::random_device gen;
//     std::mt19937 gen(1701);

//     std::poisson_distribution<> distr(p);

//     std::cout << std::endl;
//     std::cout << "min() == " << distr.min() << std::endl;
//     std::cout << "max() == " << distr.max() << std::endl;
//     std::cout << "p() == " << std::fixed << std::setw(11) << std::setprecision(10) << distr.mean() << std::endl;

//     // generate the distribution as a histogram
//     std::map<int, int> histogram;
//     for (int i = 0; i < s; ++i) {
//         ++histogram[distr(gen)];
//     }

//     // print results
//     std::cout << "Distribution for " << s << " samples:" << std::endl;
//     for (const auto& elem : histogram) {
//         std::cout << std::setw(5) << elem.first << ' ' << std::string(elem.second, ':') << std::endl;
//     }
//     std::cout << std::endl;
// }

    
}

