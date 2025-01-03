#include "Technology.hpp"
#include "./util/log.hpp"

namespace sca {

void Port::addShape(Layer *layer, const BoxT<double> &box) {
  m_shapes.emplace_back(layer, box);
}

Port *Libcell::makePort(const std::string &port_name) {
  return makeHelper(port_name, m_port_name_map, m_ports, port_name);
}

Port *Libcell::findPort(const std::string &port_name) const {
  return findHelper(port_name, m_port_name_map);
}

void Technology::reset() {
  m_layers.clear();
  m_layer_name_map.clear();
  m_cut_layers.clear();
  m_cut_layer_name_map.clear();
  m_libcells.clear();
  m_libcell_name_map.clear();
}

Layer *Technology::makeLayer(const std::string &layer_name) {
  return makeHelper(layer_name, m_layer_name_map, m_layers, layer_name,
                    numLayers());
}

CutLayer *Technology::makeCutLayer(const std::string &cut_layer_name) {
  return makeHelper(cut_layer_name, m_cut_layer_name_map, m_cut_layers,
                    cut_layer_name, numCutLayers());
}

Libcell *Technology::makeLibcell(const std::string &libcell_name) {
  return makeHelper(libcell_name, m_libcell_name_map, m_libcells, libcell_name);
}

Layer *Technology::findLayer(const std::string &layer_name) const {
  return findHelper(layer_name, m_layer_name_map);
}

CutLayer *Technology::findCutLayer(const std::string &cut_layer_name) const {
  return findHelper(cut_layer_name, m_cut_layer_name_map);
}

Libcell *Technology::findLibcell(const std::string &libcell_name) const {
  return findHelper(libcell_name, m_libcell_name_map);
}

double Technology::layerRes(int idx) const {
  Layer *layer = m_layers[idx].get();
  auto result = findLayerRC(layer->name());
  double res;
  if(result){
      //exist set_layer_rc 
    res = result->first * 1e9;
  }else{
    res = 1e+6 * layer->sqRes() / layer->wireWidth();
  }
  return res;
}

double Technology::layerCap(int idx) const {
  Layer *layer = m_layers[idx].get();
  auto result = findLayerRC(layer->name().c_str());
  double cap;
  if(result){
    //exist set_layer_rc 
    cap  = result->second * 1e-9;  
  }
  else{
    cap = 
        1e-6 * (layer->sqCap() * layer->wireWidth() + layer->edgeCap() * 2.);
  }
  return cap;
}

double Technology::cutLayerRes(int idx) const {
  CutLayer *cut_layer = m_cut_layers[idx].get();
  return cut_layer->res();
}

bool Technology::addLayerRC(const std::string& layer_name, double resistance, double capacitance) {
  auto result = m_layer_rc.insert({layer_name, new std::pair(resistance, capacitance)});
  if (!result.second) {
      LOG_ERROR("Error: Layer %s set_layer_rc already exists and was not inserted.");
      return false; 
  }
  return true; 
}

std::pair<double, double> *Technology::findLayerRC(const std::string& layer_name) const {
  return findHelper(layer_name, m_layer_rc);
}


}; // namespace sca
