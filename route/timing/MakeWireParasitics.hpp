#pragma once


#include "../object/GRNetwork.hpp"
#include "../object/GRTechnology.hpp"
#include <map>
#include <sta/Parasitics.hh>
#include <sta/Sta.hh>

class MakeWireParasitics {
public:
  MakeWireParasitics(const GRNetwork *network, const GRTechnology *tech);

  void estimateParasitcs(GRNet *net);
  void clearParasitics();
  // Return the Slack of a given net
  float getNetSlack(GRNet *net);

private:
  using NodeRoutePtMap = std::map<GRPoint, sta::ParasiticNode *>;

  void makeRouteParasitics(GRNet *net, sta::Net *sta_net,
                           sta::Parasitic *parasitic);
  void makeParasiticsToPins(GRNet *net, sta::Net *sta_net,
                            sta::Parasitic *parasitic);

  sta::ParasiticNode *ensureParasiticNode(const GRPoint &pt,
                                          sta::Parasitic *parasitic,
                                          sta::Net *net);

  const GRNetwork *m_network;
  const GRTechnology *m_tech;
  sta::Sta *m_sta;
  sta::Network *m_sta_network;
  sta::Parasitics *m_parasitics;
  sta::ArcDelayCalc *m_arc_delay_calc;

  NodeRoutePtMap m_node_map;
  size_t m_resistor_id;
};


