#ifndef __CONTEXT_HPP__
#define __CONTEXT_HPP__

#include "../object/GRNetwork.hpp"
#include "../object/GRTechnology.hpp"
#include "../timing/MakeWireParasitics.hpp"
#include <memory>

namespace sta {
class Report;
class NetworkReader;
class Instance;
class Library;
} // namespace sta

class Context {
public:
  static Context *ctx();

  Context();

  bool test();

  bool readLef(const char *lef_file);
  bool readDef(const char *def_file);
  bool readGuide(const char *guide_file);
  bool writeGuide(const char *guide_file);
  bool writeSlack(const char *slack_file);

  bool linkDesign(const char *design_name);
  sta::Instance *linkNetwork(const char *top_cell_name);

  bool runCugr2();
  bool estimateParasitcs();

  GRNetwork *network() { return m_network.get(); }
  GRTechnology *technology() { return m_tech.get(); }

private:
  static std::unique_ptr<Context> s_ctx;

  std::unique_ptr<GRNetwork> m_network;
  std::unique_ptr<GRTechnology> m_tech;
  std::unique_ptr<MakeWireParasitics> m_parasitics_builder;

  std::unique_ptr<LefDatabase> m_lef_db;
  // std::unique_ptr<DefDatabase> m_def_db;
};

#endif
