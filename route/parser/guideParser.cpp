#include "parser.hpp"
#include "../object/Design.hpp"
#include "../util/log.hpp"
#include <fstream>

namespace sca {

int readGuideImpl(const std::string &guide_file_path, Design *design) {
  Technology *tech = design->technology();

  std::ifstream fin(guide_file_path);
  ASSERT(!!fin, "Cound not open GUIDE file `%s`", guide_file_path.c_str());
  std::string line;
  Net *net = nullptr;
  std::vector<RouteSegment<int>> net_route;
  while (fin.good()) {
    std::getline(fin, line);
    std::vector<std::string> words;
    {
      size_t last = 0;
      last = line.find_first_not_of(' ');
      size_t next = last;
      while ((next = line.find_first_of(' ', last)) != std::string::npos) {
        words.push_back(line.substr(last, next - last));
        last = line.find_first_not_of(' ', next);
      }
      if (last < line.size()) {
        words.push_back(line.substr(last, std::string::npos));
      }
    }

    if (words.size() == 0 || (words.size() == 1 && words[0] == "("))
      continue;
    else if (words.size() == 1 && words[0] == ")") {
      auto tree = buildTree(net_route, tech);
      net->setRoutingTree(tree);
      // precompute access points
      std::vector<std::vector<sca::PointOnLayerT<int>>> access_points_per_pin(net->numPins());
      for (int j = 0; j < net->numPins(); j++) {
        Pin *pin = net->pin(j);
        design->grid()->computeAccessPoints(pin, access_points_per_pin[j]);
      }
      // check end points
      for (int j = 0; j < net->numPins(); j++) {
        Pin *pin = net->pin(j);
        const auto &access_points = access_points_per_pin[j];
        for (size_t k = 0; k < access_points.size(); k++) {
          const auto &ap = access_points[k];
          if (ap.x == tree->x && ap.y == tree->y && ap.layerIdx == tree->layerIdx)
            pin->setPosition(ap);
        }
      }
      // check itermediate points
      GRTreeNode::preorder(tree, [&](std::shared_ptr<GRTreeNode> node) {
        for (const auto &child : node->children) {
          for (int j = 0; j < net->numPins(); j++) {
            Pin *pin = net->pin(j);
            // compute local access_points
            const auto &access_points = access_points_per_pin[j];
            for (size_t k = 0; k < access_points.size(); k++) {
              const auto &ap = access_points[k];
              auto [init_x, final_x] = std::minmax(node->x, child->x);
              auto [init_y, final_y] = std::minmax(node->y, child->y);
              auto [init_z, final_z] =
                std::minmax(node->layerIdx, child->layerIdx);
              if (init_x <= ap.x && ap.x <= final_x && init_y <= ap.y &&
                ap.y <= final_y && init_z <= ap.layerIdx &&
                ap.layerIdx <= final_z)
                pin->setPosition(ap);
            }
          }
        }
      });
    } else if (words.size() == 1) {
      net = design->findNet(words[0]);
      ASSERT(net, "Cound file NET `%s`", words[0].c_str());
      net_route.clear();
    } else {
      PointOnLayerT<int> p(tech->findLayer(words[2])->idx(), std::stoi(words[0]), std::stoi(words[1]));
      PointOnLayerT<int> q(tech->findLayer(words[5])->idx(), std::stoi(words[3]), std::stoi(words[4]));
      p = design->grid()->dbuToGcell(p);
      q = design->grid()->dbuToGcell(q);
      net_route.emplace_back(p, q);
    }
  }
  return 0;
}

}
