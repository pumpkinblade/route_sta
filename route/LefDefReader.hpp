#ifndef __LEF_DEF_READER_HPP__
#define __LEF_DEF_READER_HPP__

#include "LefDefDatabase.hpp"
#include <sta/Network.hh>

class LefDefReader {
public:
  explicit LefDefReader(sta::NetworkReader *network);
  ~LefDefReader() = default;

  bool read(const char *lef_file, const char *def_file);

  void makeLibcell(const LefLibcell *libcell);

private:
  sta::NetworkReader *m_network;
  sta::Library *m_library;
  std::string m_lef_file;
  std::string m_def_file;
};

bool readLefDefFile(const char *lef_file, const char *def_file,
                    sta::NetworkReader *network);

sta::Instance *linkLefDefNetwork(const char *top_cell_name,
                                 bool make_black_boxes, sta::Report *report,
                                 sta::NetworkReader *network);

#endif
