#pragma once

#include "../object/Design.hpp"
#include "../object/Technology.hpp"
#include "../timing/MakeWireParasitics.hpp"
#include <memory>

namespace sta {
class Report;
class NetworkReader;
class Instance;
class Library;
} // namespace sta

namespace sca {

class Context {
public:
  static Context *ctx();

  Context();

  int readLef(const char *lef_file);
  int readDef(const char *def_file);
  int linkDesign(const char *design_name);
  static sta::Instance *linkFunc(const char *top_cell_name, bool, sta::Report *,
                                 sta::NetworkReader *);

  int readGuide(const char *guide_file);
  bool writeGuide(const char *guide_file);
  int writeSlack(const char *slack_file);
  bool setLayerRc(const std::string &layer_name, double res, double cap);

  /*IrisLin*/
  std::vector<int> getNetOrder();
  /*IrisLin*/

  int runCugr2();
  int estimateParasitcs();

  Technology *technology() const { return m_tech.get(); }
  Design *design() const { return m_design.get(); }
  MakeWireParasitics *parasiticsBuilder() const {
    return m_parasitics_builder.get();
  }

private:
  static std::unique_ptr<Context> s_ctx;

  std::unique_ptr<Technology> m_tech;
  std::unique_ptr<Design> m_design;
  std::unique_ptr<MakeWireParasitics> m_parasitics_builder;
};
} // namespace sca
