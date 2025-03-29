/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef MYGRAPH_H
#define MYGRAPH_H

#include "ns3/core-module.h"
#include "sfc.h"
// #include "scaling.h"
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/properties.hpp>
#include <boost/algorithm/string/split.hpp> // boost:split()
#include <boost/algorithm/string.hpp> // boost::is_any_of()
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <random>
#include <map>
#include <utility> // for make pair


#include <boost/graph/graph_utility.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/random.hpp>
#include <boost/property_map/transform_value_property_map.hpp>
#include "ns3/yen_ksp.h"  // for k short path
using namespace ns3;

namespace ns3{


struct Cfg{
        int inter_latency;  //create_graph()
        int intra_latency;  
        int inter_bw;       
        int intra_bw;             
        int domain_num;
        int processing_cap;
        

};

struct Nodes_and_Edges{
    
    std::vector<int> id;
    std::vector<int> domain;
    std::vector<int> tier;
    std::vector<int> source;
    std::vector<int> target;
    std::vector<int> border;        // if the node is border node
    std::vector<int> interdomain;   // if the link is inter domain link
    
    };



struct VertexProperty{
        int id;
        int processing_cap;
        int domain;
        int border;
        int init_processing_cap;
       
    };
    
     // edge property in boost graph
struct EdgeProperty{
        int id;
        int bw;
        int latency;
        int inter_domain; // equals 1 if it is interdomain edge.
        int init_bw;
    
    
        int get_weight(){
            return latency;
        }

        int get_interdomain(){
            return inter_domain;
        }
    };

typedef boost::adjacency_list<
    boost::vecS, //使用数组来存储 edge边
    boost::vecS, //使用数组来存储 vecS
    boost::bidirectionalS, //申明为有向图，可以访问其out-edge,若要都能访问
                //则需要定义为bidirectionalS directedS
    VertexProperty,
    EdgeProperty //定义边的属性
    > Graph;

typedef std::vector<int> Path;

struct MyNode{
        int id;
        int domain;
        int border;
    };

    struct Link{
        int source;
        int target;
        int inter_domain;
    };

typedef std::pair<int, int> Link_key;

typedef std::map<int, MyNode> Node_map;
typedef std::map<Link_key, Link> Link_map;

typedef boost::graph_traits<Graph>::edge_descriptor edge_descriptor_t;
typedef boost::graph_traits<Graph>::vertex_descriptor vertex_descriptor_t;

typedef boost::graph_traits<Graph>::vertex_iterator vertex_iterator_t;
typedef boost::graph_traits<Graph>::edge_iterator edge_iterator_t;
//
typedef boost::graph_traits<Graph>::out_edge_iterator out_edge_iterator_t;

typedef boost::graph_traits<Graph>::adjacency_iterator adjaceny_iterator_t;

typedef std::vector<std::vector<int>> All_paths_vec;

typedef std::vector<std::vector<vertex_descriptor_t>> All_paths_vec_descriptor;

typedef std::vector<Path> Short_path_vec;

struct Graph_map{
        std::map<int, MyNode> node_map;
        std::map<Link_key, Link> link_map;
    };


class MyGraph 
{

    private:

        bool get_vertex_descriptor();

        VertexProperty get_node_property(int node_id);

        void read_file();

        bool load_graph_map();


        std::string m_file;
     

        Nodes_and_Edges m_nodes_and_edges;


    public:

        MyGraph();

        void init(std::string file_name, Cfg cfg);

        bool create_graph();

        Graph get_graph();

        Graph_map get_graph_map();

        void set_graph(Graph g);

        void set_graph_map(Graph_map g_map);

        bool is_edge_exist(int v, int u);

        int get_node_cap(int id);

        int get_node_domain(int id);

        int get_edge_bw(int src,  int dst);

        int get_edge_latency( int src,  int dst);

        bool check_edge_bw(int src, int dst, int required_bw);

        bool check_path_bw(Path p, int required_bw);

        bool check_node_cap(sfc::Vnf vnf, int node_id);

        bool minus_node_cap(int node_id, int required_cap);

        bool minus_edge_bw(int src, int dst, int required_bw);

        bool minus_path_bw(Path p , int required_bw);

        bool release_edge_bw(int src, int dst, int bw);

        void release_path_bw(Path p, int bw);

        int get_path_latency(Path p);
        
        bool ksp(int src, int dst, std::vector<Path>& short_path_v, boost::optional<unsigned> K);

        void release_node_cap(int node_id, int required_cap);

        void print_graph();

        int get_domain_id(int node_id);

        int get_max_node();

        std::vector<int> get_short_path( int src, int dst);

        int get_short_path_delay( int src, int dst);

        std::vector<int> get_neighbours(int node_id);

        bool  is_digits(const std::string &str);

        double get_mean_node_util();

        double get_mean_edge_util();

        std::vector<double> get_every_edge_util();

        std::vector<int> get_graph_bw();

        int get_nodes_num();
        
        int get_edges_num();


        

        int check_fail_edge_num(Path p, int required_bw);
       
        Graph m_graph;

        Graph_map m_graph_map;

        Cfg m_cfg;
};
        

} 

#endif /* MY_GRAPH */
