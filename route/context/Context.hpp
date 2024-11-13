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
  Context() = default;
  static Context *context() { return s_ctx.get(); }

  bool readLef(const std::string &lef_file);
  bool readDef(const std::string &def_file, bool use_routing);
  bool runCugr2();

  bool writeGuide(const std::string &file);
  bool writeSlack(const std::string &file);

  bool test();

  sta::Instance *linkNetwork(const std::string &top_cell_name);

  GRNetwork *network() { return m_network.get(); }
  GRTechnology *technology() { return m_tech.get(); }

private:
  static std::unique_ptr<Context> s_ctx;

  std::unique_ptr<GRNetwork> m_network;
  std::unique_ptr<GRTechnology> m_tech;
  std::unique_ptr<MakeWireParasitics> m_parasitics_builder;
  sta::Report *m_sta_report;
  sta::NetworkReader *m_sta_network;
  sta::Library *m_sta_library;
  sta::Sta *m_sta;

  std::unique_ptr<LefDatabase> m_lef_db;
  // std::unique_ptr<DefDatabase> m_def_db;
};

#endif
