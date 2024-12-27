#include "Context.hpp"
#include "../cugr2/GlobalRouter.h"
#include "../parser/parser.hpp"
#include "../util/log.hpp"
#include "../object/Route.hpp"
#include <sta/Network.hh>
#include <fstream>
#include <iomanip>

namespace sca {
std::unique_ptr<Context> Context::s_ctx;

Context *Context::ctx() {
  if (s_ctx == nullptr) {
    s_ctx = std::make_unique<Context>();
  }
  return s_ctx.get();
}

Context::Context() = default;

int Context::readLef(const char *lef_file) {
  if (m_tech == nullptr) {
    m_tech = std::make_unique<Technology>();
  }
  return readLefImpl(lef_file, m_tech.get());
}

int Context::readDef(const char *def_file) {
  m_design = std::make_unique<Design>();
  m_design->setTechnology(m_tech.get());
  int res = readDefImpl(def_file, m_design.get());
  if (!res) {
    m_design->makeGrid();
  }
  return res;
}

sta::Instance *Context::linkFunc(const char *, bool, sta::Report *,
                                 sta::NetworkReader *) {
  Context *ctx = Context::ctx();
  ctx->m_parasitics_builder =
      std::make_unique<MakeWireParasitics>(ctx->m_design.get());
  return ctx->m_parasitics_builder->staTopInstance();
}

int Context::linkDesign(const char *design_name __attribute_maybe_unused__) {
  sta::Sta *sta = sta::Sta::sta();
  sta::Report *sta_report = sta->report();
  sta::NetworkReader *sta_network = sta->networkReader();
  sta_network->setLinkFunc(linkFunc);
  sta_network->linkNetwork(design_name, false, sta_report);
  return 0;
}

int Context::readGuide(const char *guide_file) {
  return readGuideImpl(guide_file, m_design.get());
}

bool Context::writeGuide(const char *guide_file) {
  std::ofstream fout(guide_file);
  for (int i = 0; i < m_design->numNets(); i++) {
    sca::Net* net = m_design->net(i);
    fout << net->name() << std::endl;
    if (net->routingTree() == nullptr) {
      fout << "(\n)\n";
    } else if (net->routingTree()->children.size() == 0) {
      fout << "(\n";
      const auto &tree = net->routingTree();
      PointOnLayerT<int> p = m_design->grid()->gcellToDbu(*tree);
      std::string layer_name = m_tech->layer(p.layerIdx)->name();
      fout << p.x << " " << p.y << " " << layer_name << " " << p.x << " " << p.y
           << " " << layer_name << "\n";
      fout << ")\n";
    } else {
      fout << "(\n";
      GRTreeNode::preorder(
          net->routingTree(), [&](std::shared_ptr<GRTreeNode> node) {
            for (const auto &child : node->children) {
              auto [p_x, q_x] = std::minmax(node->x, child->x);
              auto [p_y, q_y] = std::minmax(node->y, child->y);
              auto [p_z, q_z] = std::minmax(node->layerIdx, child->layerIdx);
              PointOnLayerT<int> p = m_design->grid()->gcellToDbu(PointOnLayerT<int>(p_z, p_x, p_y));
              PointOnLayerT<int> q = m_design->grid()->gcellToDbu(PointOnLayerT<int>(q_z, q_x, q_y));
              std::string p_layer_name = m_tech->layer(p.layerIdx)->name();
              std::string q_layer_name = m_tech->layer(q.layerIdx)->name();
              fout << p.x << " " << p.y << " " << p_layer_name << " " << q.x
                   << " " << q.y << " " << q_layer_name << "\n";
            }
          });
      fout << ")\n";
    }
  }
  return 0;
}

int Context::writeSlack(const char *slack_file) {
  std::ofstream fout(slack_file);
  if (!fout) {
    LOG_ERROR("can not open file %s", slack_file);
    return false;
  }
  for (int i = 0; i < m_design->numNets(); i++) {
    Net *net = m_design->net(i);
    float slack = m_parasitics_builder->getNetSlack(net);
    fout << net->name() << " " << std::setprecision(5) << slack << '\n';
  }
  return 0;
}

int Context::runCugr2() {
  cugr2::Parameters params;
  params.threads = 1;
  params.unit_length_wire_cost = 0.00131579;
  params.unit_via_cost = 4.;
  params.unit_overflow_costs =
      std::vector<double>(static_cast<size_t>(m_tech->numLayers()), 5.);
  params.min_routing_layer = 1;
  params.cost_logistic_slope = 1.;
  params.maze_logistic_slope = .5;
  params.via_multiplier = 2.;
  params.target_detour_count = 20;
  params.max_detour_ratio = 0.25;
  cugr2::GlobalRouter globalRouter(m_design.get(), params);
  globalRouter.route();
  return 0;
}
bool Context::setLayerRc(const std::string &layer_name, double res, double cap){
  return ctx()->technology()->addLayerRC(layer_name, res, cap);
}


int Context::estimateParasitcs() {
  m_parasitics_builder->clearParasitics();
  for (int i = 0; i < m_design->numNets(); i++) {
    m_parasitics_builder->estimateParasitcs(m_design->net(i));
  }
  sta::Sta::sta()->delaysInvalid();
  return 0;
}
} // namespace sca
