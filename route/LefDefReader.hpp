#ifndef __LEF_DEF_READER_HPP__
#define __LEF_DEF_READER_HPP__

#include "LefDefDatabase.hpp"
#include <sta/Network.hh>

class LefDefReader {
public:
  explicit LefDefReader(sta::NetworkReader *network);
  ~LefDefReader() = default;

  bool read(const char *lef_file, const char *def_file);
  sta::Instance *linkNetwork();

  void makeLibcell(const LefLibcell *libcell);

private:
  void warn(int id, const char *fmt, ...);

  sta::Report *m_report;
  sta::Debug *m_debug;
  sta::NetworkReader *m_network;
  sta::Library *m_library;
  std::string m_lef_file;
  std::string m_def_file;

  LefDefDatabase m_db;
};

bool readLefDefFile(const char *lef_file, const char *def_file,
                    sta::NetworkReader *network);

sta::Instance *linkLefDefNetwork(const char *top_cell_name,
                                 bool make_black_boxes, sta::Report *report,
                                 sta::NetworkReader *network);

#endif
