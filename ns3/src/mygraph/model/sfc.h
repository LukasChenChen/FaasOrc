/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef SFC_H
#define SFC_H
#include <vector>
#include <string>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <boost/algorithm/string.hpp> // is_any_of
#include <map>
#include <algorithm>

namespace sfc {

enum Function_type {non_io_bound, io_bound};

typedef std::pair<int, std::vector<int>> Traffic_pair;//traffic_rate, routing path

//contains several flows result in turn
typedef std::vector<Traffic_pair> Request_result;



struct Sf_instance{
    int m_request_id; // request id
    int m_vnf_index;
    int m_id;
    int m_host_node;
    int m_capacity;
    int m_used_capacity;

};

struct Instance_map{
    std::map<int, Sf_instance> m_instance_map; // instance_id, instance

    Sf_instance get_instance(int instance_id){
        std::map<int, Sf_instance>::iterator it = m_instance_map.find(instance_id);

        if( it != m_instance_map.end()){

            return it->second;
        }
    }

    bool insert_instance(int& instance_id, Sf_instance& sf)
    {   
        // if empty
        if(m_instance_map.empty())
        {   
            instance_id = 1 ;

            sf.m_id = 1;

            std::pair<std::map<int, Sf_instance>::iterator,bool> ret =  m_instance_map.insert(std::pair<int,Sf_instance>(1, sf));

            std::cout << " insert instance id 1" << std::endl;

            return ret.second;
        }


        //get the biggest key
        int big_key = m_instance_map.rbegin()->first;

        sf.m_id = big_key + 1;
        
        // insert a key = big_key + 1
        std::pair<std::map<int, Sf_instance>::iterator,bool> ret =  m_instance_map.insert(std::pair<int,Sf_instance>(big_key+1, sf));

        instance_id = big_key + 1;

        std::cout << " insert instance id "<< instance_id <<std::endl;

        return ret.second;
    }

    //insert for 
    bool insert_instance_slot(int instance_id, Sf_instance& sf)
    {   
        std::cout << " insert instance slot id" << instance_id << std::endl;

        sf.m_id = instance_id;

        std::pair<std::map<int, Sf_instance>::iterator,bool> ret =  m_instance_map.insert(std::pair<int,Sf_instance>(instance_id, sf));

        return ret.second;
    }


    bool modify_instance(int instance_id, Sf_instance sf)
    {   
        //get the biggest key
        auto it = m_instance_map.find(instance_id);

        if(it == m_instance_map.end())
        {   
            std::cout << "cannot modify instance" << std::endl;
            return false;
        }
        
        it->second = sf;

        std::cout << "sfc modify instance id " << sf.m_id << " " << sf.m_used_capacity << std::endl;

        return true;
    }

    bool remove_instance(int instance_id)
    {   
        //get the biggest key
        auto it = m_instance_map.find(instance_id);

        if(it == m_instance_map.end())
        {
            return false;
        }
        
        m_instance_map.erase(it);

        return true;
    }

    int get_size()
    {
        return m_instance_map.size();
    }





   

};

class Request_instance_map
{   public:
    Request_instance_map()
    {
        m_map = std::map<int, Instance_map>();
    };

    std::map<int, Instance_map> m_map; // <vnf_index, instance_map
    int m_request_id;

    std::map<int, Instance_map>::iterator get_instance_map(int vnf_order_id)
    {
        std::map<int, Instance_map>::iterator it = m_map.find(vnf_order_id);

        return it;
    };

    Sf_instance get_instance(int vnf_index, int instance_id)
    {
        std::map<int, Instance_map>::iterator it = m_map.find(vnf_index);

        if( it != m_map.end()){

            Instance_map tmp_map = it->second;

            return tmp_map.get_instance(instance_id);

        }
    
    };


    //insert a new instance
    bool insert_instance(int vnf_order_id, int& instance_id, Sf_instance sf)
    {   
        std::cout << "--- insert instance ----" << std::endl; 

        std::map<int, Instance_map>::iterator it = get_instance_map(vnf_order_id);

        //if the vnf map does not exist yet, create one
        if(it == m_map.end())
        {
            //create a instance_map
            Instance_map im = {std::map<int, Sf_instance>()};
            bool ret = im.insert_instance(instance_id, sf);
            m_map.insert(std::pair<int,Instance_map>(vnf_order_id, im));
            return ret;
        }

        bool ret = it->second.insert_instance(instance_id, sf);

        // if exist then false
        return ret; 

    };

    // insert instance for m_slot_instance_map
    bool insert_instance_slot(int vnf_order_id, int instance_id, Sf_instance& sf)
    {   
        std::map<int, Instance_map>::iterator it = get_instance_map(vnf_order_id);

        //if the vnf map does not exist yet, create one
        if(it == m_map.end())
        {
            //create a instance_map
            Instance_map im = {std::map<int, Sf_instance>()};
            bool ret = im.insert_instance_slot(instance_id, sf);
            m_map.insert(std::pair<int,Instance_map>(vnf_order_id, im));
            return ret;
        }

        bool ret = it->second.insert_instance_slot(instance_id, sf);

        // if exist then false
        return ret; 

    }


    //modify a current instance
    bool modify_instance(int vnf_order_id, int instance_id, Sf_instance sf)
    {   
        
        std::map<int, Instance_map>::iterator it = get_instance_map(vnf_order_id);

        //if the vnf map does not exist yet, create one
        if(it == m_map.end())
        {   
            std::cout <<"cannot modify instance, no instance map" <<std::endl;
            return false;
        }

        bool ret = it->second.modify_instance(instance_id, sf);

        // if exist then false
        return ret; 

    };

    void clear()
    {
        m_map.clear();
    };

    bool remove_instance(int vnf_order_id, int instance_id)
    {   
        std::map<int, Instance_map>::iterator it = get_instance_map(vnf_order_id);

        //if the vnf map does not exist yet, create one
        if(it == m_map.end())
        {
            return false;
        }


        bool ret = it->second.remove_instance(instance_id);

        // if exist then false
        return ret; 

    };

    //how many instance in the map in total
    int get_size()
    {   
        int num = 0;
        for(auto it = m_map.begin(); it != m_map.end(); it++)
        {
            num += it->second.get_size(); 
        }

        return num;
    };


   
};




/* ... */
class Vnf{
    public:
        Vnf(int id, int cap, int sfc_id, float change_factor, Function_type type,  double p_time){
            m_id = id;
            m_cap = cap;
            m_sfc_id = sfc_id;
            m_change_factor = change_factor;
            m_type = type;
            m_processing_time = p_time;
        };
    int m_id;
    int m_cap;
    int m_sfc_id;
    double m_change_factor;
    Function_type m_type;
    double m_processing_time;// in mu second
};







class Flow{

    public:
    Flow(){

    };

    void init(int id, int traffic_rate, int start_time, int end_time){

        m_id = id;
        m_traffic_rate = traffic_rate;
        m_start_time = start_time;
        m_end_time = end_time;
        m_is_deployed = false;
        m_is_gone = false;

        m_routing_path = std::vector<int>();

        m_host_instance =std::vector<int>();   //vnf_order_id, instance_id


    };
    int m_id;
    int m_traffic_rate;
    int m_start_time; // flow start time should be between request start and end time
    int m_end_time; // flow end time should equal to request end time
    bool m_is_deployed;
    bool m_is_gone;
    std::vector<int> m_routing_path;

    std::vector<std::vector<int>> m_routing_segments; // put the segment here.

    std::vector<int> m_host_instance; // <vnf_order_id, instance_id>

    std::vector<int> m_host_node;


    bool check_deployed()
    {
        return m_is_deployed;
    };
    
    //assemble the results from m_routing_segments
    void create_result_path()
    {   
        if(m_routing_segments.size() == 1)
        {
            return;
        }

        for(auto it = m_routing_segments.begin(); it != m_routing_segments.end(); it++)
        {  
            m_routing_path.insert(m_routing_path.end(), (*it).begin(), (*it).end());
        }
        
        //remove adjacent dulipcate node
        auto it = std::unique(m_routing_path.begin(), m_routing_path.end());

        m_routing_path.resize( std::distance(m_routing_path.begin(),it) ); 

    };

    void print_routing_path(){

        for(auto it = m_routing_path.begin(); it != m_routing_path.end(); it++){

            std::cout << *it << " -> ";

        }

        std::cout << std::endl;
    };

    void set_routing_path(std::vector<int> p){

        m_routing_path = p;
    };

    void set_host_nodes(std::vector<int> h_nodes){

        m_host_node = h_nodes;

    };

    double get_flows_hops(){
        return m_routing_path.size();
    };


};



typedef std::vector<Vnf> Chain;


const Vnf FW = Vnf(1, 400, 0, 0.9, io_bound, 120);
const Vnf IDS = Vnf(2, 600, 0,0.95, non_io_bound, 160);
const Vnf Caching = Vnf(3, 580, 0, 0.8, non_io_bound, 83);
const Vnf FM = Vnf(4, 550, 0, 1.0, io_bound, 200); // flow monitor
const Vnf LB = Vnf(5,500, 0, 1.0,non_io_bound, 647.5);
const std::vector<Vnf> vnf_vec = {FW, IDS, Caching, FM, LB};





class Flows{

    public:
        Flows(){


        };
        Flow get_flow(int index);
        void add_flow(Flow f);
        int get_size();
        std::vector<Flow> get_flows();
        void clear();
        std::vector<Flow> m_flows;
};



class Request{
    
    public:
        int m_id;
        int m_source;
        int m_target;
        Chain m_chain;
       
        int m_start_time; // request start time slot
        int m_end_time;// request finish time slot
        Flows m_flows;
        Request_instance_map m_map;

        bool m_succ;// successfull deployed

        bool m_finish;// all flows finished

        Request(){

        };
        void init(int id_input, int src, int dst, Chain chain_input, Flows f, int t_start, int t_end){
                m_id = id_input;
                m_source = src;
                m_target = dst;
                m_chain = chain_input;
                m_flows = f;
                m_start_time = t_start;
                m_end_time = t_end;
                m_succ = true;
                m_finish =false;
        };

        //get the accummlative change factor
        double get_change_factor(int vnf_order_id)
        {   
            double total_change_factor = 1.0;

            for (int i = 1; i < vnf_order_id; i++)
            {
                total_change_factor = total_change_factor * m_chain.at(i).m_change_factor;
            }

            return total_change_factor;
        };
        //get flow by id
        Flow get_flow(int id)
        {
            for(auto it = m_flows.m_flows.begin(); it != m_flows.m_flows.end(); it++)
            {
                if((*it).m_id == id)
                {
                    return *it;
                }
            }

        

        };

        void replace_flow(Flow f)
        {

            //  get_flow(f.m_id) = f;

             for(auto it = m_flows.m_flows.begin(); it != m_flows.m_flows.end(); it++)
             {
                if((*it).m_id == f.m_id)
                {
                    *it = f;
                }

             }

        }

        std::vector<int> get_flow_routing_path(int id)
        {
            Flow f = get_flow(id);

            return f.m_routing_path;
        }

        // return a vector contains the pair <traffic rate, routing path>
        Request_result get_flows_routing_path()
        {   Request_result r_result;

            for(auto it = m_flows.m_flows.begin(); it != m_flows.m_flows.end(); it++)
            {
                Traffic_pair t_pair = std::make_pair((*it).m_traffic_rate,  (*it).m_routing_path);

                r_result.push_back(t_pair);
            }

            return r_result;

        }

        std::vector<double> get_flows_hops()
        {   
            std::vector<double> hops_v;

            double hops = 0;

            for(auto it = m_flows.m_flows.begin(); it != m_flows.m_flows.end(); it++)
            {   
                if((*it).m_routing_path.empty()){
                    continue;
                }

                hops = (*it).m_routing_path.size();

                hops_v.push_back(hops);
            }

            return hops_v;

        }

        int get_total_traffic()
        {   
            int total_traffic = 0;

            for(auto it = m_flows.m_flows.begin(); it != m_flows.m_flows.end(); it++)
            {
                total_traffic += (*it).m_traffic_rate;
            }

            return total_traffic;
        }

        int get_flow_num()
        {
            return m_flows.m_flows.size();
        };

        Vnf get_vnf(int vnf_order_id){

            return m_chain.at(vnf_order_id );//vnf_order_id starts from 0
        }

       
};



class Sfc_vec
    {
        private:
           
        public:

        //  constructor
            Sfc_vec(int number, int max_node_number, int chain_length, int flow_num, int max_flow_r, int min_flow_r , int max_t_slot);
            int m_num;
            int m_max_node;   //starts from 0
            int m_chain_len;  
            int m_max_flow_num;
            int m_max_flow_rate;
            int m_min_flow_rate;
            int m_max_time;
            int m_current_index;
            std::vector<int> m_r_num_v;

            Flows m_flows;
            std::vector<double> m_traffic_v; // traffic size from .csv


            std::vector<sfc::Request> sfc_request_vec;

           
            Flows create_flows(int r_start_time, int r_end_time);
            Chain create_chain();
            void create_sfc();

            Flows get_flows(int index);

            std::pair<int, int> get_max_min_time(Flows flows);

            void get_traffic_from_csv();
            void create_flows_from_csv(int r_start_time, int r_end_time);
            void create_sfc_from_csv();

            void create_sfc_poisson();

            void read_request_num();

            void create_one_slot_requests(int request_num, int time_slot);

            double get_throughput();

           
    };

    

    Chain string_to_chain(std::string chain_s);

    Flows string_to_flow(std::string flows_s);

    

    
    void print_request(std::vector<sfc::Request> request_vec);

    std::vector<sfc::Request>  create_sfc_vec();

    void duplicate_sfc(std::vector<Request>& request_vec, int request_num, double factor);

    bool read_sfc(std::string file_name, std::vector<sfc::Request>& request_vec,  int request_num, double factor = 1.0);

    void possion_process(int average_value, int loop_num);

}

#endif /* SFC_H */

