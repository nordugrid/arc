#include <arc/ArcConfigIni.h>

#include "Config.h"

namespace ArcJura
{
  Config::Config(char const * configFile): processed(false)
  {
    std::string name;
    Arc::ConfigIni file(configFile);
    if(!file) return;
    file.SetSectionIndicator(".");
    const int sgas_secnum = 0;
    file.AddSection("grid-manager/jura/sgas");
    const int apel_secnum = 0;
    file.AddSection("grid-manager/jura/apel");
    const int jura_secnum = 2;
    file.AddSection("grid-manager/jura");
    while(true) {
      std::string command;
      std::string rest;
      if(!file.ReadNext(command,rest)) return;
      if(command.empty()) break;
      if (file.SectionNum() == jura_secnum) {
        if (file.SubSection()[0] == '\0') {


        }
      } else if (file.SectionNum() == sgas_secnum) {
        if (file.SectionNew()) {
          name = file.SubSection();
          if(name.empty()) return;
        } else {


        }
      } else if (file.SectionNum() == apel_secnum) {
        if (file.SectionNew()) {
          name = file.SubSection();
          if(name.empty()) return;
        } else {


        }
      }
    }
    processed = true;
  }
}

