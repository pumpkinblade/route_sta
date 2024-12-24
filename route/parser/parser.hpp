#pragma once

#include <string>

namespace sca {

class Technology;
class Design;

int readLefImpl(const std::string &lef_file_path, Technology *tech);
int readDefImpl(const std::string &def_file_path, Design *design);
int readGuideImpl(const std::string &guide_file_path, Design *design);


} // namespace sca
