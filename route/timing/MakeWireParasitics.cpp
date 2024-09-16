/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, The Regents of the University of California
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////////

#include "MakeWireParasitics.hpp"
#include <sta/Corner.hh>
#include <sta/Network.hh>
#include <sta/Sta.hh>

// void MakeWireParasitics::estimateParasitcs(GRNet *net) {
//   sta::Net *sta_net = m_sta_net_map.at(net);

//   for (sta::Corner *corner : *m_sta->corners()) {
//     NodeRoutePtMap node_map;

//     sta::ParasiticAnalysisPt *analysis_point =
//         corner->findParasiticAnalysisPt(m_min_max);
//     sta::Parasitic *parasitic =
//         m_parasitics->makeParasiticNetwork(sta_net, false, analysis_point);
//     makeRouteParasitics(net, sta_net, corner, analysis_point, parasitic,
//                         node_map);
//     // makePartialParasiticsToPins(net->getPins(), node_map, corner,
//     //                             analysis_point, parasitic, net);
//     m_arc_delay_calc->reduceParasitic(parasitic, sta_net, corner,
//                                       sta::MinMaxAll::all());
//   }

//   m_parasitics->deleteParasiticNetworks(sta_net);
// }

// void MakeWireParasitics::clearParasitics() {
//   // Remove any existing parasitics.
//   sta::Sta::sta()->deleteParasitics();
//   // Make separate parasitics for each corner.
//   sta::Sta::sta()->setParasiticAnalysisPts(true);
// }

float MakeWireParasitics::getNetSlack(GRNet *net) {
  auto *sta = sta::Sta::sta();
  auto *network = sta->network();
  auto *sta_net =
      network->findNet(network->topInstance(), net->getName().c_str());
  float slack = sta::Sta::sta()->netSlack(sta_net, sta::MinMax::max());
  return slack;
}

// void MakeWireParasitics::makeRouteParasitics(
//     GRNet *net, sta::Net *sta_net, sta::Corner *corner,
//     sta::ParasiticAnalysisPt *analysis_point, sta::Parasitic *parasitic,
//     NodeRoutePtMap &node_map) {
//   // const int min_routing_layer = grouter_->getMinRoutingLayer();
//   const int min_routing_layer = 1;

//   size_t resistor_id_ = 1;
//   auto visit = [&](std::shared_ptr<GRTreeNode> node) {
//     for (auto child : node->children) {
//       GRSegment segment(node->x, node->y, node->layerIdx, child->x, child->y,
//                         child->layerIdx);
//       const int wire_length_dbu = segment.length();

//       const int init_layer = segment.init_layer;
//       sta::ParasiticNode *n1 =
//           (init_layer >= min_routing_layer)
//               ? ensureParasiticNode(segment.init_x, segment.init_y,
//               init_layer,
//                                     node_map, parasitic, sta_net)
//               : nullptr;

//       const int final_layer = segment.final_layer;
//       sta::ParasiticNode *n2 =
//           (final_layer >= min_routing_layer)
//               ? ensureParasiticNode(segment.final_x, segment.final_y,
//                                     final_layer, node_map, parasitic,
//                                     sta_net)
//               : nullptr;
//       if (!n1 || !n2) {
//         continue;
//       }

//       sta::Units *units = m_sta->units();
//       float res = 0.0;
//       float cap = 0.0;
//       if (wire_length_dbu == 0) {
//         res = getCutLayerRes(init_layer, corner);
//       } else if (segment.init_layer == segment.final_layer) {
//         std::tie(res, cap) =
//             layerRC(wire_length_dbu, segment.init_layer, corner);
//       }
//       m_parasitics->incrCap(n1, cap / 2.0);
//       m_parasitics->makeResistor(parasitic, m_resistor_id++, res, n1, n2);
//       m_parasitics->incrCap(n2, cap / 2.0);
//     }
//   };
//   GRTreeNode::preorder(net->getRoutingTree(), visit);
// }

// void MakeWireParasitics::makeParasiticsToPins(
//     std::vector<Pin *> &pins, NodeRoutePtMap &node_map, sta::Corner *corner,
//     sta::ParasiticAnalysisPt *analysis_point, sta::Parasitic *parasitic) {
//   for (Pin *pin : pins) {
//     makeParasiticsToPin(pin, node_map, corner, analysis_point, parasitic);
//   }
// }

// // Make parasitics for the wire from the pin to the grid location of the pin.
// void MakeWireParasitics::makeParasiticsToPin(
//     Pin *pin, NodeRoutePtMap &node_map, sta::Corner *corner,
//     sta::ParasiticAnalysisPt *analysis_point, sta::Parasitic *parasitic) {
//   sta::Pin *sta_pin = m_sta_pin_map.at(pin);
//   sta::ParasiticNode *pin_node =
//       m_parasitics->ensureParasiticNode(parasitic, sta_pin, m_network);

//   Point pt = pin->getPosition();
//   Point grid_pt = pin->getOnGridPosition();

//   // Use the route layer above the pin layer if there is a via
//   // to the pin.
//   int layer = pin->getConnectionLayer() + 1;
//   RoutePt grid_route(grid_pt.getX(), grid_pt.getY(), layer);
//   sta::ParasiticNode *grid_node = node_map[grid_route];
//   float via_res = 0;

//   // Use the pin layer for the connection.
//   if (grid_node == nullptr) {
//     layer--;
//     grid_route = RoutePt(grid_pt.getX(), grid_pt.getY(), layer);
//     grid_node = node_map[grid_route];
//   } else {
//     via_res = getCutLayerRes(layer, corner);
//   }

//   if (grid_node) {
//     // Make wire from pin to gcell center on pin layer.
//     int wire_length_dbu =
//         abs(pt.getX() - grid_pt.getX()) + abs(pt.getY() - grid_pt.getY());
//     auto [res, cap] = layerRC(wire_length_dbu, layer, corner);
//     // We could added the via resistor before the segment pi-model
//     // but that would require an extra node and the accuracy of all
//     // this is not that high.  Instead we just lump them together.
//     m_parasitics->incrCap(pin_node, cap / 2.0);
//     m_parasitics->makeResistor(parasitic, m_resistor_id++, res + via_res,
//                                pin_node, grid_node);
//     m_parasitics->incrCap(grid_node, cap / 2.0);
//   }
// }

// void MakeWireParasitics::makePartialParasiticsToPins(
//     std::vector<Pin *> pins, NodeRoutePtMap &node_map, sta::Corner *corner,
//     sta::ParasiticAnalysisPt *analysis_point, sta::Parasitic *parasitic,
//     Net *net) {
//   for (Pin *pin : pins) {
//     makePartialParasiticsToPin(pin, node_map, corner, analysis_point,
//     parasitic,
//                                net);
//   }
// }

// // Make parasitics for the wire from the pin to the grid location of the pin.
// void MakeWireParasitics::makePartialParasiticsToPin(
//     Pin *pin, NodeRoutePtMap &node_map, sta::Corner *corner,
//     sta::ParasiticAnalysisPt *analysis_point, sta::Parasitic *parasitic,
//     Net *net) {
//   sta::Pin *sta_pin = m_sta_pin_map.at(pin);
//   sta::ParasiticNode *pin_node =
//       m_parasitics->ensureParasiticNode(parasitic, sta_pin, m_network);

//   Point pt = pin->getPosition();
//   Point grid_pt = pin->getOnGridPosition();

//   // Use the route layer above the pin layer if there is a via
//   // to the pin.
//   int net_max_layer;
//   int net_min_layer;
//   grouter_->getNetLayerRange(net, net_min_layer, net_max_layer);
//   int layer = net_min_layer + 1;
//   RoutePt grid_route(grid_pt.getX(), grid_pt.getY(), layer);
//   sta::ParasiticNode *grid_node = node_map[grid_route];
//   float via_res = 0;

//   // Use the pin layer for the connection.
//   if (grid_node == nullptr) {
//     layer--;
//     grid_route = RoutePt(grid_pt.getX(), grid_pt.getY(), layer);
//     grid_node = node_map[grid_route];
//   } else {
//     odb::dbTechLayer *cut_layer =
//         tech_->findRoutingLayer(layer)->getLowerLayer();
//     via_res = getCutLayerRes(cut_layer, corner);
//   }

//   if (grid_node) {
//     // Make wire from pin to gcell center on pin layer.
//     int wire_length_dbu =
//         abs(pt.getX() - grid_pt.getX()) + abs(pt.getY() - grid_pt.getY());
//     auto [res, cap] = layerRC(wire_length_dbu, layer, corner);
//     // We could added the via resistor before the segment pi-model
//     // but that would require an extra node and the accuracy of all
//     // this is not that high.  Instead we just lump them together.
//     m_parasitics->incrCap(pin_node, cap / 2.0);
//     m_parasitics->makeResistor(parasitic, m_resistor_id++, res + via_res,
//                                pin_node, grid_node);
//     m_parasitics->incrCap(grid_node, cap / 2.0);
//   }
// }

// std::pair<float, float> MakeWireParasitics::layerRC(int wire_length_dbu,
//                                                     int layer_id,
//                                                     sta::Corner *corner)
//                                                     const {
//   auto [r_per_meter, cap_per_meter] =
//       m_context->layerRC(layer_id, corner->index());
//   float wire_length = m_context->dbuToMeters(wire_length_dbu);
//   float res = r_per_meter * wire_length;
//   float cap = cap_per_meter * wire_length;
//   return {res, cap};
// }

// sta::ParasiticNode *MakeWireParasitics::ensureParasiticNode(
//     int x, int y, int layer, NodeRoutePtMap &node_map,
//     sta::Parasitic *parasitic, sta::Net *net) const {
//   GRPoint pin_loc(x, y, layer);
//   sta::ParasiticNode *node = node_map[pin_loc];
//   if (node == nullptr) {
//     node = m_parasitics->ensureParasiticNode(parasitic, net, node_map.size(),
//                                              m_network);
//     node_map[pin_loc] = node;
//   }
//   return node;
// }

// float MakeWireParasitics::getCutLayerRes(int layer, sta::Corner *corner,
//                                          int num_cuts) const {
//   float res = m_context->cutLayerRes(layer, corner->index());
//   return res / num_cuts;
// }
