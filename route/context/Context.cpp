#include "Context.hpp"
#include "../cugr2/GlobalRouter.h"
#include "../parser/parser.hpp"
#include "../util/log.hpp"
#include <sta/Network.hh>

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

/* int Context::readGuide(const char *guide_file) {
  std::ifstream fin(guide_file);
  if (!fin) {
    LOG_ERROR("can not open file %s", guide_file);
    return false;
  }

  std::string line;
  std::vector<GRNet *> nets;
  std::vector<std::vector<std::pair<GRPoint, GRPoint>>> net_routes;
  while (fin.good()) {
    std::getline(fin, line);
    if (line == "(" || line.empty() || line == ")")
      continue;
    std::istringstream iss(line);
    std::vector<std::string> words;
    while (!iss.eof()) {
      std::string word;
      iss >> word;
      words.push_back(word);
    }
    if (words.size() == 1) {
      // new net
      auto it =
          std::find_if(m_network->nets().begin(), m_network->nets().end(),
                       [&](const GRNet *n) { return n->name() == words[0]; });
      if (it == m_network->nets().end()) {
        LOG_ERROR("Could find net %s", words[0].c_str());
        return false;
      }
      nets.push_back(*it);
      net_routes.emplace_back();
    } else {
      // segment
      GRPoint p(m_tech->findLayer(words[2]), std::stoi(words[0]),
                std::stoi(words[1]));
      GRPoint q(m_tech->findLayer(words[5]), std::stoi(words[3]),
                std::stoi(words[4]));
      p = m_tech->dbuToGcell(p);
      q = m_tech->dbuToGcell(q);
      net_routes.back().emplace_back(p, q);
    }
  }

  for (size_t i = 0; i < nets.size(); i++) {
    auto tree = buildTree(net_routes[i], m_tech.get());

    // ensure access point
    std::vector<int> ap_indices(nets[i]->pins().size(), -1);
    // check end points
    for (size_t j = 0; j < nets[i]->pins().size(); j++) {
      GRPin *pin = nets[i]->pins()[j];
      for (size_t k = 0; k < pin->accessPoints().size(); k++) {
        const auto &ap = pin->accessPoints()[k];
        if (ap.x == tree->x && ap.y == tree->y && ap.layerIdx == tree->layerIdx)
          ap_indices[j] = static_cast<int>(k);
      }
    }
    // check itermediate points
    GRTreeNode::preorder(tree, [&](std::shared_ptr<GRTreeNode> node) {
      for (const auto &child : node->children) {
        for (size_t j = 0; j < nets[i]->pins().size(); j++) {
          GRPin *pin = nets[i]->pins()[j];
          for (size_t k = 0; k < pin->accessPoints().size(); k++) {
            const auto &ap = pin->accessPoints()[k];
            auto [init_x, final_x] = std::minmax(node->x, child->x);
            auto [init_y, final_y] = std::minmax(node->y, child->y);
            auto [init_z, final_z] =
                std::minmax(node->layerIdx, child->layerIdx);
            if (init_x <= ap.x && ap.x <= final_x && init_y <= ap.y &&
                ap.y <= final_y && init_z <= ap.layerIdx &&
                ap.layerIdx <= final_z)
              ap_indices[j] = static_cast<int>(k);
          }
        }
      }
    });

    for (size_t j = 0; j < nets[i]->pins().size(); j++) {
      if (ap_indices[j] < 0) {
        LOG_ERROR("guide of net %s doesn't cover", nets[i]->name().c_str());
        return false;
      }
    }

    for (size_t j = 0; j < nets[i]->pins().size(); j++) {
      nets[i]->pins()[j]->setAccessIndex(ap_indices[j]);
    }
    nets[i]->setRoutingTree(tree);
  }

  return true;
}

bool Context::writeGuide(const char *guide_file) {
  std::ofstream fout(guide_file);
  for (GRNet *net : m_network->nets()) {
    fout << net->name() << std::endl;
    if (net->routingTree() == nullptr) {
      fout << "(\n)\n";
    } else if (net->routingTree()->children.size() == 0) {
      fout << "(\n";
      const auto &tree = net->routingTree();
      GRPoint p = m_tech->gcellToDbu(*tree);
      std::string layer_name = m_tech->layerName(p.layerIdx);
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
              GRPoint p = m_tech->gcellToDbu(GRPoint(p_z, p_x, p_y));
              GRPoint q = m_tech->gcellToDbu(GRPoint(q_z, q_x, q_y));
              std::string p_layer_name = m_tech->layerName(p.layerIdx);
              std::string q_layer_name = m_tech->layerName(q.layerIdx);
              fout << p.x << " " << p.y << " " << p_layer_name << " " << q.x
                   << " " << q.y << " " << q_layer_name << "\n";
            }
          });
      fout << ")\n";
    }
  }
  return true;
} */

/* int Context::writeSlack(const char *slack_file) {
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
} */

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

int Context::estimateParasitcs() {
  m_parasitics_builder->clearParasitics();
  for (int i = 0; i < m_design->numNets(); i++) {
    m_parasitics_builder->estimateParasitcs(m_design->net(i));
  }
  sta::Sta::sta()->delaysInvalid();
  return 0;
}
} // namespace sca
