#pragma once

#include "../object/Design.hpp"
#include <map>
#include <sta/Parasitics.hh>
#include <sta/Sta.hh>

namespace sca {

class MakeWireParasitics {
public:
  MakeWireParasitics(Design *design);
  ~MakeWireParasitics();

  void estimateParasitcs(Net *net);
  void clearParasitics();
  // Return the Slack of a given net
  float getNetSlack(Net *net);

  sta::Instance *staTopInstance() const { return m_sta_top_inst; }

private:
  void makeRouteParasitics(Net *net, sta::Net *sta_net,
                           sta::Parasitic *parasitic);
  void makeParasiticsToPins(Net *net, sta::Net *sta_net,
                            sta::Parasitic *parasitic);

  sta::ParasiticNode *ensureParasiticNode(const PointOnLayerT<int> &pt,
                                          sta::Parasitic *parasitic,
                                          sta::Net *net);

  Design *m_design;
  sta::Sta *m_sta;
  sta::NetworkReader *m_sta_network;
  sta::Library *m_sta_library;
  sta::Cell *m_sta_top_cell;
  sta::Instance *m_sta_top_inst;
  sta::Parasitics *m_parasitics;
  sta::ArcDelayCalc *m_arc_delay_calc;

  using NodeRoutePtMap = std::map<PointOnLayerT<int>, sta::ParasiticNode *>;
  NodeRoutePtMap m_node_map;
  size_t m_resistor_id;
};

} // namespace sca
