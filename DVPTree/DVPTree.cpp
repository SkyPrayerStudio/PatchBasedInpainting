#include <iostream>

#include "metric_space_search.hpp"

namespace boost {
  enum vertex_data_t { vertex_data };

  BOOST_INSTALL_PROPERTY(vertex, data);
};

int main(int argc, char *argv[])
{
  typedef boost::hypercube_topology<6,boost::minstd_rand> TopologyType;

  typedef boost::adjacency_list<boost::vecS,
                                boost::vecS,
                                boost::undirectedS,
                                boost::property< boost::vertex_data_t, TopologyType::point_type >
                                > Graph;

  Graph g;

  typedef boost::graph_traits<Graph>::vertex_descriptor VertexType;

  //typedef TopologyType::point_type PointType;

  typedef ReaK::pp::dvp_tree<VertexType, TopologyType, boost::property_map<Graph, boost::vertex_data_t>::type > TreeType;

  TopologyType myTopology;
  boost::property_map<Graph, boost::vertex_data_t>::type positionMap;
  TreeType tree(g, myTopology, positionMap);

  return 0;
}
