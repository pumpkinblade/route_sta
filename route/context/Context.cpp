#include "Context.hpp"
#include "../cugr2/GlobalRouter.h"
#include "../lefdef/DefDatabase.hpp"
#include "../lefdef/LefDatabase.hpp"
#include "../util/log.hpp"
#include <fstream>
#include <iomanip>
#include <sta/Corner.hh>
#include <sta/Network.hh>
#include <sta/PortDirection.hh>
#include <sta/Report.hh>
#include <sta/Sta.hh>
#include <sta/Units.hh>
#include <unordered_map>

std::unique_ptr<Context> Context::s_ctx(new Context);

static sta::Instance *link(const char *top_cell_name, bool, sta::Report *,
                           sta::NetworkReader *) {
  return Context::context()->linkNetwork(top_cell_name);
}

bool Context::test() {
  std::vector<std::string> net_names = {"clk",
                                        "clknet_0_clk",
                                        "clknet_2_0__leaf_clk",
                                        "clknet_2_1__leaf_clk",
                                        "clknet_2_2__leaf_clk",
                                        "clknet_2_3__leaf_clk"};
  for (const auto &name : net_names) {
    GRNet *net = *std::find_if(
        m_network->nets().begin(), m_network->nets().end(),
        [&name](const GRNet *net) { return net->name() == name; });
    std::cout << "pin of " << name << "\n";
    for (const GRPin *pin : net->pins()) {
      if (pin->instance() == nullptr)
        std::cout << "( PIN " << pin->name() << " )\n";
      else
        std::cout << "( " << pin->instance()->name() << " " << pin->name()
                  << " )\n";
    }
    std::cout << "\n";
    // std::cout << "routing tree of " << name << "\n";
    // GRTreeNode::preorder(
    //     net->routingTree(), [](std::shared_ptr<GRTreeNode> node) {
    //       for (auto child : node->children) {
    //         std::printf("(%d %d %d) -> (%d %d %d)\n", node->x, node->y,
    //                     node->layerIdx, child->x, child->y, child->layerIdx);
    //       }
    //     });
    // std::cout << "\n";
  }
  return true;
}

bool Context::readLef(const std::string &lef_file) {
  if (m_lef_db == nullptr) {
    m_lef_db = std::make_unique<LefDatabase>();
  }
  return m_lef_db->read(lef_file);
}

bool Context::readDef(const std::string &def_file, bool use_routing) {
  m_sta_network = sta::Sta::sta()->networkReader();
  m_sta_report = sta::Sta::sta()->report();
  sta::Sta::sta()->readNetlistBefore();
  m_sta_network->setLinkFunc(link);

  LOG_INFO("read def");
  auto def_db = std::make_unique<DefDatabase>();
  bool success = def_db->read(def_file, use_routing);
  if (!success)
    return false;

  LOG_INFO("init tech");
  m_tech = std::make_unique<GRTechnology>(m_lef_db.get(), def_db.get());
  LOG_INFO("init network");
  m_network =
      std::make_unique<GRNetwork>(m_lef_db.get(), def_db.get(), m_tech.get());
  LOG_INFO("init parasitics_builder");
  m_parasitics_builder =
      std::make_unique<MakeWireParasitics>(m_network.get(), m_tech.get());
  m_sta = sta::Sta::sta();

  m_sta_library = m_sta_network->findLibrary("lefdef");
  if (m_sta_library == nullptr)
    m_sta_library = m_sta_network->makeLibrary("lefdef", nullptr);

  // topcell in def file
  sta::Cell *sta_top_cell =
      m_sta_network->findCell(m_sta_library, m_network->designName().c_str());
  if (sta_top_cell) {
    m_sta_network->deleteCell(sta_top_cell);
  }
  sta_top_cell = m_sta_network->makeCell(
      m_sta_library, def_db->design_name.c_str(), false, def_file.c_str());
  for (const DefIoPin &iopin : def_db->iopins) {
    sta::Port *port = m_sta_network->makePort(sta_top_cell, iopin.name.c_str());
    switch (iopin.direction) {
    case PortDirection::Inout:
      m_sta_network->setDirection(port, sta::PortDirection::bidirect());
      break;
    case PortDirection::Input:
      m_sta_network->setDirection(port, sta::PortDirection::input());
      break;
    case PortDirection::Output:
      m_sta_network->setDirection(port, sta::PortDirection::output());
      break;
    default:
      LOG_WARN("Unknown direction of io PIN.%s", iopin.name.c_str());
      break;
    }
  }

  return true;
}

bool Context::readGuide(const std::string &guide_file) {
  std::ifstream fin(guide_file);
  if (!fin) {
    LOG_ERROR("can not open file %s", guide_file.c_str());
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

bool Context::runCugr2() {
  cugr2::Parameters params;
  params.threads = 1;
  params.unit_length_wire_cost = 0.00131579;
  params.unit_via_cost = 4.;
  params.unit_overflow_costs =
      std::vector<double>(static_cast<size_t>(m_tech->numLayers()), 5.);
  params.min_routing_layer = m_tech->minRoutingLayer();
  params.cost_logistic_slope = 1.;
  params.maze_logistic_slope = .5;
  params.via_multiplier = 2.;
  params.target_detour_count = 20;
  params.max_detour_ratio = 0.25;
  cugr2::GlobalRouter globalRouter(m_network.get(), m_tech.get(), params);
  globalRouter.route();
  return true;
}

bool Context::estimateParasitcs() {
  m_parasitics_builder->clearParasitics();
  for (GRNet *net : m_network->nets()) {
    m_parasitics_builder->estimateParasitcs(net);
  }
  sta::Sta::sta()->delaysInvalid();
  return true;
}

bool Context::writeGuide(const std::string &guide_file) {
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
}

bool Context::writeSlack(const std::string &slack_file) {
  std::ofstream fout(slack_file);
  if (!fout) {
    LOG_ERROR("can not open file %s", slack_file.c_str());
    return false;
  }
  for (GRNet *net : m_network->nets()) {
    float slack = m_parasitics_builder->getNetSlack(net);
    fout << net->name() << " " << std::setprecision(3) << slack << '\n';
  }
  return true;
}

sta::Instance *Context::linkNetwork(const std::string &top_cell_name) {
  if (m_sta_library == nullptr) {
    LOG_ERROR("libray has not been initialized");
    return nullptr;
  }
  sta::Cell *sta_top_cell =
      m_sta_network->findCell(m_sta_library, top_cell_name.c_str());
  if (sta_top_cell == nullptr) {
    LOG_ERROR("%s is not a def design.", top_cell_name.c_str());
    return nullptr;
  }

  sta::Instance *sta_top_inst =
      m_sta_network->makeInstance(sta_top_cell, "", nullptr);

  std::unordered_map<sta::Instance *, sta::LibertyCell *> sta_inst_cell_map;
  for (const GRNet *net : m_network->nets()) {
    sta::Net *sta_net =
        m_sta_network->makeNet(net->name().c_str(), sta_top_inst);
    for (const GRPin *pin : net->pins()) {
      if (pin->instance() == nullptr) { // IO pin
        sta::Port *sta_port =
            m_sta_network->findPort(sta_top_cell, pin->name().c_str());
        if (sta_port == nullptr) {
          LOG_ERROR("io port name PIN.%s not found", pin->name().c_str());
          m_sta_network->deleteInstance(sta_top_inst);
          return nullptr;
        }
        if (m_sta_network->findPin(sta_top_inst, sta_port) == nullptr) {
          sta::Pin *sta_pin =
              m_sta_network->makePin(sta_top_inst, sta_port, nullptr);
          m_sta_network->makeTerm(sta_pin, sta_net);
        }
        // m_sta_network->connect(sta_top_inst, sta_port, sta_net);
      } else { // internal pin
        GRInstance *inst = pin->instance();
        sta::Instance *sta_inst = m_sta_network->findInstanceRelative(
            sta_top_inst, inst->name().c_str());
        // if the instance not found, then create it
        if (sta_inst == nullptr) {
          sta::LibertyCell *sta_liberty_cell =
              m_sta_network->findLibertyCell(inst->libcellName().c_str());
          if (sta_liberty_cell == nullptr) {
            LOG_ERROR("%s is not a liberty libcell",
                      inst->libcellName().c_str());
            m_sta_network->deleteInstance(sta_top_inst);
            return nullptr;
          }
          sta_inst = m_sta_network->makeInstance(
              sta_liberty_cell, inst->name().c_str(), sta_top_inst);
          sta_inst_cell_map.emplace(sta_inst, sta_liberty_cell);
        }

        // connect
        sta::LibertyCell *sta_liberty_cell = sta_inst_cell_map[sta_inst];
        sta::Cell *sta_cell = reinterpret_cast<sta::Cell *>(sta_liberty_cell);
        sta::Port *sta_port =
            m_sta_network->findPort(sta_cell, pin->name().c_str());
        if (sta_port == nullptr) {
          LOG_ERROR("port name %s.%s not found", m_sta_network->name(sta_cell),
                    pin->name().c_str());
          m_sta_network->deleteInstance(sta_top_inst);
          return nullptr;
        }
        m_sta_network->makePin(sta_inst, sta_port, sta_net);
        // m_sta_network->connect(sta_inst, sta_port, sta_net);
      }
    }
  }
  return sta_top_inst;
}
