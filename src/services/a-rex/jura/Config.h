#ifndef _JURA_CONFIG_H
#define _JURA_CONFIG_H

namespace ArcJura
{
  class Config
  {
  public:
    Config(char const * configFile);
    operator bool() { return processed; }
    bool operator!() { return !processed; }
  private:
    bool processed;
  };

}

#endif
