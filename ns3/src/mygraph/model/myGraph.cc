#include "myGraph.h"
#include "ns3/core-module.h"
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <random>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/random.hpp>
#include <boost/property_map/transform_value_property_map.hpp>
#include <cmath>
// #include "ns3/yen_ksp.h"  // for k short path

// using namespace ns3;

NS_LOG_COMPONENT_DEFINE("mygraph");

namespace ns3{

// constructor MyGraph
MyGraph::MyGraph()
{
  

}

void MyGraph::init(std::string file_name, Cfg cfg)
{
    m_file = file_name;
    m_cfg = cfg;

}


bool  MyGraph::is_digits(const std::string &str)
{
    return std::all_of(str.begin(), str.end(), ::isdigit); // C++11
}

void MyGraph::read_file( ){
    std::string line;
    Node_map node_map;
    Link_map link_map;

    // note that ns3 programs points to start from file ns3.26/. so we need to change the directory.
    std:: string file_name_ns3 = "scratch/" + m_file;
    std::ifstream myfile (file_name_ns3);
    std::vector<int> id;
    std::vector<int> domain;
    std::vector<int> source;
    std::vector<int> target;
    std::vector<int> inter_domain;
    std::vector<int> border;
    // Nodes_and_Edges nodes_and_edges;


    if (myfile.is_open())
    {
        
        while (getline (myfile, line)) {
            
//            lines.push_back(line);
            size_t found = line.find("id");
            if (found != std::string::npos )
                {   
                    if (is_digits(line.substr(found+3)) == false){

                        continue;
                    }
                    NS_LOG_INFO( "id" << line.substr(found+3));
                    int node_id = stoi(line.substr(found+3));
//                  std::cout << "type" << typeid(node_id).name() <<"\n";
                
                    id.push_back(node_id);
                }
                
              
            size_t found_1 = line.find("domain");
            if (found_1 != std::string::npos )
                {
                    NS_LOG_INFO( "domain" << line.substr(found_1+7));
                    int node_domain = stoi(line.substr(found_1+7));
                    domain.push_back(node_domain);
                }
                
                
            size_t found_3 = line.find("source");
            if (found_3 != std::string::npos )
                {
                    // std::cout << "source" << line.substr(found_3+7) <<endl;
                    
                    int edge_source = stoi(line.substr(found_3+7));
                    
                    source.push_back(edge_source);
                    
                }
                
            size_t found_4 = line.find("target");
            if (found_4 != std::string::npos )
                {
                    // std::cout << "target" << line.substr(found_4+7) <<endl;
                    int edge_target = stoi(line.substr(found_4+7));
                    target.push_back(edge_target);
                }

            size_t found_5 = line.find("border");
            if (found_5 != std::string::npos )
                {
                    // std::cout << "border" << line.substr(found_5+7) <<endl;
                    int is_border = stoi(line.substr(found_5+7));
                    border.push_back(is_border);
                }

            size_t found_6 = line.find("interdomain");
            if (found_6 != std::string::npos )
                {
                    // std::cout << "interdomain" << line.substr(found_6+12) <<endl;
                    int interDomain = stoi(line.substr(found_6+12));
                    inter_domain.push_back(interDomain);
                }
            
          
       
        }

        myfile.close();
    } else { 
       std::cout << "Unable to open file, MyGraph::read_file "; 
       NS_LOG_ERROR("Unable to open file, MyGraph::read_file "  ) ;
       
    }
    
    
    
    m_nodes_and_edges.id = id;
    m_nodes_and_edges.domain = domain;
    m_nodes_and_edges.source = source;
    m_nodes_and_edges.target = target;
    m_nodes_and_edges.border = border;
    m_nodes_and_edges.interdomain = inter_domain;

    for(int i = 0; i < m_nodes_and_edges.source.size(); i++){

        NS_LOG_INFO(" source " << m_nodes_and_edges.source.at(i) << " target " << m_nodes_and_edges.target.at(i));

    }

    NS_LOG_INFO(" num of source " << m_nodes_and_edges.source.size());

    NS_LOG_INFO(" num of target " <<  m_nodes_and_edges.target.size());


       
       

  
}



bool MyGraph:: load_graph_map()
{

    MyNode node, tmp_node_1, tmp_node_2;

    Node_map::iterator it_1, it_2;

    for(size_t i = 0; i < m_nodes_and_edges.id.size(); i++){
        node.id = m_nodes_and_edges.id[i];
        node.domain = m_nodes_and_edges.domain[i];
        node.border = 0;
        m_graph_map.node_map.insert(std::pair<int, MyNode>(node.id, node));
    }

    Link link;

    int link_count = 0;
    
    for(size_t i = 0; i < m_nodes_and_edges.source.size(); i++)
    {
        link.source = m_nodes_and_edges.source[i];
        link.target = m_nodes_and_edges.target[i];

        // check two nodes if same domain.
        it_1 = m_graph_map.node_map.find(link.source);
        it_2 = m_graph_map.node_map.find(link.target);


        // decide if one link is interdomain
        if (it_1 != m_graph_map.node_map.end()  && it_2 != m_graph_map.node_map.end())
        {
            tmp_node_1 = it_1->second;
            tmp_node_2 = it_2->second;

            if (tmp_node_1.domain == tmp_node_2.domain)
            {
                link.inter_domain = 0;
            }
            else
            {
                link.inter_domain = 1;      // modify the 
                it_1->second.border = 1; // if it is inter domain then border.
                it_2->second.border = 1;

            }
        
            
        }
        else
        {
            NS_LOG_ERROR("myGraph:: read_gml: cannot find the edge " << link.source <<"->"<<link.target);

            return false;

        }
    
        Link_key link_key = std::pair<int, int >(link.source, link.target);

        m_graph_map.link_map.insert(std::pair<Link_key, Link>(link_key, link));

        link_count++;

    }
    
    NS_LOG_INFO("link count of link map " << link_count);
    NS_LOG_INFO("size of link map " << m_graph_map.link_map.size());

    return true;
}

bool MyGraph::create_graph(){
    NS_LOG_DEBUG("-----create_graph----");
    m_graph.clear();
    read_file();

    load_graph_map();

    Cfg cfg = m_cfg;

    Node_map node_map = m_graph_map.node_map;

    Link_map link_map = m_graph_map.link_map;

    std::map<int, MyNode>::iterator it_node;

    std::map<Link_key, Link>::iterator it_link;
    
    VertexProperty vp;

    it_node = node_map.begin();

    while(it_node != node_map.end())
    {
        
        vp.id = it_node->second.id;
        vp.domain = it_node->second.domain;
        vp.border = it_node->second.border;

        vp.processing_cap = cfg.processing_cap;

        vp.init_processing_cap = cfg.processing_cap;
             
        NS_LOG_DEBUG("CREATE_GRAPH processing cap is" << vp.processing_cap);

        boost::add_vertex(vp, m_graph); 

        it_node++;
    }
        
    EdgeProperty ep;
        
    it_link = link_map.begin();
    while(it_link != link_map.end())
    {
        int src = it_link->second.source;
        
        int dst = it_link->second.target;

        int inter_domain = it_link->second.inter_domain;
            
        if (inter_domain == 1)
        {
            ep.bw = cfg.inter_bw;
        
            ep.latency = cfg.inter_latency;

            ep.init_bw = cfg.inter_bw;

        }
        else
        {
            ep.bw = cfg.intra_bw;
        
            ep.latency = cfg.intra_latency;

            ep.init_bw = cfg.intra_bw;

        }
            

        // NS_LOG_INFO("the edge " << src << " " << dst <<" interdomain is " << ep.inter_domain);


        boost::add_edge(src, dst, ep, m_graph);
    
        boost::add_edge(dst, src, ep, m_graph);
     

        it_link++;
        
    }

    NS_LOG_DEBUG("---Num of vertices in gloabl graph-----" << boost::num_vertices(m_graph));

    NS_LOG_DEBUG("---Num of edges in gloabl graph-----" << boost::num_edges(m_graph));

    return true;

}

Graph MyGraph::get_graph()
{
    return m_graph;
}

void MyGraph::set_graph(Graph g)
{
    m_graph = g;
}


Graph_map MyGraph::get_graph_map()
{
    return m_graph_map;
}

void MyGraph::set_graph_map(Graph_map g_map)
{
    m_graph_map = g_map;
}


bool MyGraph::is_edge_exist(int v, int u)
{

    return boost::edge(u, v, m_graph).second;

}

int MyGraph::get_node_cap(int id)
{
   
    VertexProperty vp = m_graph[id];
       
    return vp.processing_cap;

}


int MyGraph::get_node_domain(int id)
{
    VertexProperty vp = m_graph[id];
       
    return vp.domain;
}

int MyGraph::get_edge_bw(int src,  int dst)
{
    if (!is_edge_exist(src, dst))
        {
            NS_LOG_ERROR("get_edge_bw: edge invalid " << src <<"->" << dst);
            return 0;
        }

    std::pair<edge_descriptor_t, bool> pair_edge = boost::edge(src, dst, m_graph);

    boost::graph_traits<Graph>::edge_descriptor e = pair_edge.first;
        

    int bw = m_graph[e].bw;

    return bw;

}

bool MyGraph::check_edge_bw(int src,  int dst, int required_bw)
{
    if (!is_edge_exist(src, dst))
        {
            NS_LOG_ERROR("check_edge_bw: edge invalid " << src <<"->" << dst);
            return false;
        }

    std::pair<edge_descriptor_t, bool> pair_edge = boost::edge(src, dst, m_graph);

    boost::graph_traits<Graph>::edge_descriptor e = pair_edge.first;
        

    int bw = m_graph[e].bw;

    if(bw >= required_bw)
    {

        return true;
    }
    else
    {
        return false;
    }

}

bool MyGraph::check_path_bw(Path p, int required_bw)
{
    if(p.size() < 2)
    {
        return true;
    }

    for(Path::iterator it = p.begin(); it != p.end()-1; it++)
    {
        if(check_edge_bw(*it, *(it+1), required_bw) == false)
        {
            return false;
        }

    }

    return true;

}

//check the number of edge that fails
int MyGraph::check_fail_edge_num(Path p, int required_bw)
{   
    int count = 0;

    if(p.size() < 2)
    {
        return count;
    }


    for(Path::iterator it = p.begin(); it != p.end()-1; it++)
    {
        if(check_edge_bw(*it, *(it+1), required_bw) == false)
        {
            count++;
        }

    }

    return count;

}




int MyGraph::get_edge_latency( int src,  int dst)
{
    if (!is_edge_exist(src, dst))
        {
            NS_LOG_ERROR("get_edge_latency: edge invalid");
            return 0;
        }

    std::pair<edge_descriptor_t, bool> pair_edge = boost::edge(src, dst, m_graph);

    boost::graph_traits<Graph>::edge_descriptor e = pair_edge.first;
        
    int latency = m_graph[e].latency;

    return latency;

}



bool MyGraph::check_node_cap(sfc::Vnf vnf, int node_id)
{

    int current_cap = get_node_cap(node_id);

    if (current_cap >= vnf.m_cap)
    {
        return true;
    }
    else
    {   
        NS_LOG_ERROR(" node id is " << node_id);
        NS_LOG_ERROR(" cap is " << current_cap);
        NS_LOG_ERROR(" required cap is " << vnf.m_cap);
        return false;
    }

}



bool MyGraph::minus_node_cap(int node_id, int required_cap)
{
    if(m_graph[node_id].processing_cap >= required_cap)
    {

        m_graph[node_id].processing_cap = m_graph[node_id].processing_cap - required_cap;

        // std::cout << " minus_node_cap" << " id " << node_id << " " << required_cap <<std::endl;
        // std::cout << " remaining " << m_graph[node_id].processing_cap << std::endl;

        NS_LOG_DEBUG("minus node cap, remaining cap " << m_graph[node_id].processing_cap );
        return true;
    }
    else
    {
        NS_LOG_DEBUG("not enough cap\n");
        return false;
    }

}

void MyGraph::release_node_cap(int node_id, int required_cap)
{

    m_graph[node_id].processing_cap = m_graph[node_id].processing_cap + required_cap;

    NS_LOG_DEBUG("release node cap, remaining cap " << m_graph[node_id].processing_cap );

}


bool MyGraph::minus_edge_bw(int src, int dst, int required_bw)
{

        
    std::pair<edge_descriptor_t, bool> pair_edge = boost::edge(src, dst, m_graph);

    boost::graph_traits<Graph>::edge_descriptor e = pair_edge.first;

    if (!is_edge_exist(src, dst))
    {   
        NS_LOG_INFO("minus_edge_bw, edge do not exist");
        return false;
    }
        

    if(m_graph[e].bw >= required_bw){

        m_graph[e].bw -= required_bw;

        NS_LOG_INFO("rest bw " <<m_graph[e].bw <<"required bw" << required_bw);

        return true;
    }
    else
    {
        return false;
    }

}


bool MyGraph::minus_path_bw(Path p , int required_bw)
{
    int src;

    int dst;

    if (p.size()==1)
    {   
        return true;
    }
    

    // iterate each pair of vertices, so stop at n-1.
    for (std::vector<int>::iterator it = p.begin(); it < p.end()-1; it++)
    {
        src = *it;

        dst = *(it+1);

        if (!minus_edge_bw( src, dst, required_bw))
        {   
            std::cout << "minus_edge_bw failed " ;
            NS_LOG_INFO("minus_path_bw failed ");
            return false;
        }
        
    }

    return true;

}

bool MyGraph::release_edge_bw(int src, int dst, int bw)
{

        
    std::pair<edge_descriptor_t, bool> pair_edge = boost::edge(src, dst, m_graph);

    boost::graph_traits<Graph>::edge_descriptor e = pair_edge.first;

    if (!is_edge_exist(src, dst))
    {   
        NS_LOG_INFO("release_edge_bw, edge do not exist");
        return false;
    }
        
    m_graph[e].bw += bw;

    return true;
    
}

void MyGraph::release_path_bw(Path p , int bw)
{
    int src;

    int dst;

    if (p.size()==1)
    {   
        return;
    }
    

    // iterate each pair of vertices, so stop at n-1.
    for (std::vector<int>::iterator it = p.begin(); it < p.end()-1; it++)
    {
        src = *it;

        dst = *(it+1);

        release_edge_bw( src, dst, bw);
       
    }

    

}

int MyGraph::get_path_latency(Path p)
{
      
    int path_latency = 0 ;

    int tmp;

    if(p.size()<2){
        NS_LOG_ERROR(" get_path_latency invalid input");
        return 0;
    }


    for (auto it = p.begin(); it != p.end()-1; it++)
    {
        tmp = get_edge_latency(*it, *(it+1));
        
        path_latency += tmp;
    }
    
    return path_latency;


}

bool MyGraph::ksp(int src, int dst, std::vector<Path>& short_path_v, boost::optional<unsigned> K){
    // 1. create a graph from namespace yen_ksp.
    // loop graph g and create yen_ksp graph, because the graph type is slightly different.
    Path short_path;


    if (src == dst)
    {
        short_path.push_back(src);
        short_path_v.push_back(short_path);
        return true;
    }
        

    auto vertex_num = num_vertices(m_graph);

    yen_ksp::graph ksp_graph(vertex_num);

    std::cout << "src " << src << " dst " << dst << std::endl;

    auto es = edges(m_graph);
    
    // iterate over the origin graph edges
    //notice that g is bidirectional, so each eit represent a single direction edge.
    // so each time , we add a directed edge to ksp_graph
    for (auto eit = es.first; eit != es.second; ++eit)
    {   
        int latency = m_graph[*eit].latency;
        vertex_descriptor_t src_d = source(*eit, m_graph);
        vertex_descriptor_t dst_d = target(*eit, m_graph);
        yen_ksp::vertex src_yen, dst_yen;
        src_yen = (yen_ksp::vertex) src_d;
        dst_yen = (yen_ksp::vertex) dst_d;

        yen_ksp::ade(ksp_graph, src_yen, dst_yen, latency);
    
    }

    std::cout << "finish aue" << std::endl;

    auto r = yen_ksp::yen_ksp(ksp_graph, src, dst, K);

    std::cout << "finish yen_ksp" << std::endl;

    // r = std::list< std::pair<W, yen_ksp::Path<G>>>
    //using Path = std::list<Edge<G>>;
    // using Edge = typename G::edge_descriptor;
    for (auto it = r.begin(); it != r.end(); it++)
    {
        NS_LOG_INFO("latency " << (*it).first);

        short_path.clear();

        auto p = (*it).second;// std::list<edge<G>>
        NS_LOG_INFO(" short path ksp ");
        for (auto itp = p.begin(); itp != p.end(); itp++)
        {
            NS_LOG_INFO(*itp);

            NS_LOG_INFO("pair" << source((*itp), ksp_graph) << " and " << target((*itp), ksp_graph));

            vertex_descriptor_t edge_src = source((*itp), ksp_graph);

            vertex_descriptor_t edge_dst = target((*itp), ksp_graph);

            if(find(short_path.begin(), short_path.end(), edge_src) == short_path.end()){

                short_path.push_back(edge_src);

                NS_LOG_INFO(" result push back edge_src" << edge_src);
            }

            if(find(short_path.begin(), short_path.end(), edge_dst) == short_path.end()){

                short_path.push_back(edge_dst);

                NS_LOG_INFO(" result push back edge_dst"<< edge_dst);
            }

        }

        short_path_v.push_back(short_path);

        std::cout << std::endl;

        NS_LOG_INFO(" ---- ksp ------ short path vec size"<< short_path_v.size());

        
        
    }
    
    return true;
}

//iterate over boost graph and then print every node link and property.
void MyGraph::print_graph()
{
    Graph::vertex_iterator v, vend;
    for (boost::tie(v, vend) = vertices(m_graph); v != vend; ++v) {
        std::cout << "Vertex descriptor #" << *v 
             << " id:"     << m_graph[*v].id
             << " left processing cap:"  << m_graph[*v].processing_cap
             << "\n";
    }


    auto es = boost::edges(m_graph);
    for (auto eit = es.first; eit != es.second; ++eit) {
       std::cout << boost::source(*eit, m_graph) << ' ' << boost::target(*eit, m_graph) 
            << "bw " << m_graph[*eit].bw <<std::endl;
       
    }
}

int MyGraph::get_domain_id(int node_id)
{
    VertexProperty vp = m_graph[node_id];
       
    return vp.domain;
}

int MyGraph::get_max_node()
{
    return m_nodes_and_edges.id.back();
}

//diklstra shortest path in the sense of delay
std::vector<int> MyGraph::get_short_path( int src, int dst){
    //add source & target
    boost::graph_traits<Graph>::vertex_descriptor startV = boost::vertex( src, m_graph );
    
    boost::graph_traits<Graph>::vertex_descriptor endV = boost::vertex( dst, m_graph );
    
    //predecessors
    // Output for predecessors of each node in the shortest path tree result
    std::vector<int>  predMap(boost::num_vertices(m_graph));
    
    //distMap
    // Output for distances for each node with initial size
    // of number of vertices
    std::vector<int>  distMap(boost::num_vertices(m_graph));
    
    
    //solve shortest path problem
    boost::dijkstra_shortest_paths(m_graph, startV,
    weight_map(boost::get(&EdgeProperty::latency, m_graph)) //arc costs from bundled properties
    .predecessor_map(boost::make_iterator_property_map(predMap.begin(),//property map style
                                                        boost::get(boost::vertex_index, m_graph)))
    .distance_map(boost::make_iterator_property_map(distMap.begin(),//property map style
                                                        boost::get(boost::vertex_index, m_graph)))
    );
    
    std::vector<int> path;
    boost::graph_traits<Graph>::vertex_descriptor current = endV;
    while (startV != current)
    {
        path.push_back(current);
        current = predMap[current];
    }
    // add start as last element (=start node) to path
    path.push_back(startV);
    
    //print out the path with reverse iterator
    std::vector< int >::reverse_iterator rit;
    NS_LOG_FUNCTION("Path from "<< startV << " to "<< endV << " is: ");
    int totalLatency= distMap[endV];

    // the real path is reversed.
    std::vector<int> path_result;

    for (rit = path.rbegin(); rit != path.rend(); ++rit){
       
        NS_LOG_FUNCTION ( *rit << " -> ");
        path_result.push_back(*rit);
    }
        
    NS_LOG_FUNCTION( "shortest path delay is : "<< totalLatency );

    return path_result;

}

//return the total delay
int MyGraph::get_short_path_delay( int src, int dst){
    //add source & target
    boost::graph_traits<Graph>::vertex_descriptor startV = boost::vertex( src, m_graph );
    
    boost::graph_traits<Graph>::vertex_descriptor endV = boost::vertex( dst, m_graph );
    
    //predecessors
    // Output for predecessors of each node in the shortest path tree result
    std::vector<int>  predMap(boost::num_vertices(m_graph));
    
    //distMap
    // Output for distances for each node with initial size
    // of number of vertices
    std::vector<int>  distMap(boost::num_vertices(m_graph));
    
    
    //solve shortest path problem
    boost::dijkstra_shortest_paths(m_graph, startV,
    weight_map(boost::get(&EdgeProperty::latency, m_graph)) //arc costs from bundled properties
    .predecessor_map(boost::make_iterator_property_map(predMap.begin(),//property map style
                                                        boost::get(boost::vertex_index, m_graph)))
    .distance_map(boost::make_iterator_property_map(distMap.begin(),//property map style
                                                        boost::get(boost::vertex_index, m_graph)))
    );
    
    std::vector<int> path;
    boost::graph_traits<Graph>::vertex_descriptor current = endV;
    while (startV != current)
    {
        path.push_back(current);
        current = predMap[current];
    }
    // add start as last element (=start node) to path
    path.push_back(startV);
    
    //print out the path with reverse iterator
    std::vector< int >::reverse_iterator rit;
    NS_LOG_FUNCTION("Path from "<< startV << " to "<< endV << " is: ");
    int totalLatency= distMap[endV];

    // the real path is reversed.
    std::vector<int> path_result;

    for (rit = path.rbegin(); rit != path.rend(); ++rit){
        NS_LOG_FUNCTION( *rit << " -> ");
        path_result.push_back(*rit);
    }
        
    NS_LOG_FUNCTION( "shortest path delay is : "<< totalLatency );

    if(std::isnan(totalLatency) ){
        totalLatency = 0;
    }

    return totalLatency;

}

std::vector<int> MyGraph::get_neighbours(int node_id){

    std::vector<int> neighbours_v;

    auto neighbours = boost::adjacent_vertices(node_id, m_graph);

    

    for (auto vd : make_iterator_range(neighbours)){

        neighbours_v.push_back(vd);
    }

    return neighbours_v;

}

double MyGraph::get_mean_node_util()
{
    Graph::vertex_iterator v, vend;

    int cap = 0;

    int init_cap = 0;

    double rest_usage_rate = 0;

    int num_nodes = 0;

    for (boost::tie(v, vend) = vertices(m_graph); v != vend; ++v) {
        
        cap = m_graph[*v].processing_cap;

        init_cap = m_graph[*v].init_processing_cap;

        rest_usage_rate += double(cap)/double(init_cap);

        num_nodes++;
             
    }

    return 100*(1-rest_usage_rate/double(num_nodes));//percentage form
}


double MyGraph::get_mean_edge_util()
{
    int bw = 0;

    int init_bw = 0;

    double usage_rate = 0;

    //Iterate through all the edges
    std::pair<edge_iterator_t, edge_iterator_t> ei = boost::edges(m_graph);

    int num_edges = 0;

    for(edge_iterator_t edge_iter = ei.first; edge_iter != ei.second; ++edge_iter) 
    {
        bw = m_graph[*edge_iter].bw;

        init_bw = m_graph[*edge_iter].init_bw;

        // rest_usage_rate += double(bw)/double(init_bw);

        usage_rate += 100*(1-double(bw)/double(init_bw));

        NS_LOG_DEBUG(" the usage rate for link " <<100*(1-double(bw)/double(init_bw)) );

        NS_LOG_DEBUG(" rest bw  " <<bw << "init bw" <<init_bw);


        num_edges++;
    }

    return usage_rate/double(num_edges);//percentage form
}

std::vector<double> MyGraph::get_every_edge_util()
{
    int bw = 0;

    int init_bw = 0;

    double usage_rate = 0;

    std::vector<double> edge_usage_v;

    //Iterate through all the edges
    std::pair<edge_iterator_t, edge_iterator_t> ei = boost::edges(m_graph);

    for(edge_iterator_t edge_iter = ei.first; edge_iter != ei.second; ++edge_iter) 
    {
        bw = m_graph[*edge_iter].bw;

        init_bw = m_graph[*edge_iter].init_bw;

        // rest_usage_rate += double(bw)/double(init_bw);

        usage_rate = 100*(1-double(bw)/double(init_bw));

        edge_usage_v.push_back(usage_rate);

        NS_LOG_DEBUG(" the usage rate for link " <<100*(1-double(bw)/double(init_bw)) );

        NS_LOG_DEBUG(" rest bw  " <<bw << "init bw" <<init_bw);

    }

    return edge_usage_v;//percentage form
}


//return a vector that contains all the bw.
std::vector<int> MyGraph::get_graph_bw(){

    int bw = 0;

    std::vector<int> bw_v;

    //Iterate through all the edges
    std::pair<edge_iterator_t, edge_iterator_t> ei = boost::edges(m_graph);

     for(edge_iterator_t edge_iter = ei.first; edge_iter != ei.second; ++edge_iter) 
    {
        bw = m_graph[*edge_iter].bw;

        bw_v.push_back(bw);
    }

    return bw_v;
}

int MyGraph::get_nodes_num(){

    return (int)boost::num_vertices(m_graph);
}

int MyGraph::get_edges_num(){

    return (int)boost::num_edges(m_graph);
}





}   //   end mygraph
