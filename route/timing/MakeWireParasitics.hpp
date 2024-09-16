#ifndef __MAKE_WIRE_PARASITICS_HPP__
#define __MAKE_WIRE_PARASITICS_HPP__

#include "../object/GRContext.hpp"
#include "../object/GRNet.hpp"
#include <map>
#include <sta/Parasitics.hh>
#include <sta/Sta.hh>

class MakeWireParasitics {
public:
  // void estimateParasitcs(GRNet *net);

  // void clearParasitics();
  // Return the Slack of a given net
  float getNetSlack(GRNet *net);

private:
  // using NodeRoutePtMap = std::map<GRPoint, sta::ParasiticNode *>;

  // void makeRouteParasitics(GRNet *net, sta::Net *sta_net, sta::Corner
  // *corner,
  //                          sta::ParasiticAnalysisPt *analysis_point,
  //                          sta::Parasitic *parasitic, NodeRoutePtMap
  //                          &node_map);
  // sta::ParasiticNode *ensureParasiticNode(int x, int y, int layer,
  //                                         NodeRoutePtMap &node_map,
  //                                         sta::Parasitic *parasitic,
  //                                         sta::Net *net) const;
  // void makeParasiticsToPins(std::vector<GRPin> &pins, NodeRoutePtMap
  // &node_map,
  //                           sta::Corner *corner,
  //                           sta::ParasiticAnalysisPt *analysis_point,
  //                           sta::Parasitic *parasitic);
  // void makeParasiticsToPin(GRPin *pin, NodeRoutePtMap &node_map,
  //                          sta::Corner *corner,
  //                          sta::ParasiticAnalysisPt *analysis_point,
  //                          sta::Parasitic *parasitic);
  // void makePartialParasiticsToPins(std::vector<GRPin> &pins,
  //                                  NodeRoutePtMap &node_map,
  //                                  sta::Corner *corner,
  //                                  sta::ParasiticAnalysisPt *analysis_point,
  //                                  sta::Parasitic *parasitic, GRNet *net);
  // void makePartialParasiticsToPin(GRPin *pin, NodeRoutePtMap &node_map,
  //                                 sta::Corner *corner,
  //                                 sta::ParasiticAnalysisPt *analysis_point,
  //                                 sta::Parasitic *parasitic, GRNet *net);
  // std::pair<float, float> layerRC(int wire_length_dbu, int layer,
  //                                 sta::Corner *corner) const;
  // float getCutLayerRes(int layer, sta::Corner *corner, int num_cuts = 1)
  // const; double dbuToMeters(int dbu) const;

  // // Variables common to all nets.
  // GlobalRouter *grouter_;
  // odb::dbTech *tech_;
  // odb::dbBlock *block_;
  // utl::Logger *logger_;
  // rsz::Resizer *resizer_;
  // sta::dbSta *sta_;
  // sta::dbNetwork *network_;

  // sta::Parasitics *m_parasitics;
  // sta::ArcDelayCalc *m_arc_delay_calc;
  // sta::MinMax *m_min_max;
  // sta::Sta *m_sta;
  // sta::Network *m_network;
  // GRContext *m_context;
  // std::map<GRNet *, sta::Net *> m_sta_net_map;
  // std::map<GRPin *, sta::Pin *> m_sta_pin_map;
  // size_t m_resistor_id;
};

#endif
