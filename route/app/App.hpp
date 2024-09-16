#ifndef __GLOBAL_ROUTER_HPP__
#define __GLOBAL_ROUTER_HPP__

#include "../object/GRContext.hpp"
// #include "../timing/MakeWireParasitics.hpp"
#include <sta/Network.hh>
#include <sta/Report.hh>
#include <sta/Sta.hh>

class App {
public:
  static const auto &app() { return s_app; }

  bool writeSlack(const std::string &file);
  bool readLefDef(const std::string &lef_file, const std::string &def_file);
  sta::Instance *linkNetwork(const std::string &top_cell_name);
  void setStaNetwork(sta::NetworkReader *sta_newtwork);

private:
  static std::shared_ptr<App> s_app;

  sta::Report *m_sta_report;
  sta::NetworkReader *m_sta_network;
  sta::Library *m_sta_library;
  std::string m_lef_file;
  std::string m_def_file;

  std::shared_ptr<GRContext> m_context;
  // std::shared_ptr<MakeWireParasitics> m_parasitics_builder;
};

#endif
