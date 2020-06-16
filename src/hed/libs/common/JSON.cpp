// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>

#include "XMLNode.h"
#include "JSON.h"

namespace Arc {

  static char const * SkipWS(char const * input) {
    while(*input) {
      if(!std::isspace(*input))
        break;
      ++input;
    }
    return input;
  }

  static char const * SkipTo(char const * input, char tag) {
    while(*input) {
      if(*input == tag)
        break;
      ++input;
    }
    return input;
  }

  char const * JSON::ParseInternal(Arc::XMLNode& xml, char const * input, int depth) {
    input = SkipWS(input);
    if(!*input) return input;
    if(*input == '{') {
        // complex item
        ++input;
        char const * nameStart = SkipWS(input);
        if(*nameStart != '}') while(true) {
            if(*nameStart != '"') return NULL;
            ++nameStart;
            char const * nameEnd = SkipTo(nameStart, '"');
            if(*nameEnd != '"') return NULL;
            char const * sep = SkipWS(nameEnd+1);
            if(*sep != ':') return NULL;
            XMLNode item = xml.NewChild(std::string(nameStart, nameEnd-nameStart));
            input = sep+1;
            input = ParseInternal(item,input,depth+1);
            if(!input) return NULL;
            input = SkipWS(input);
            if(*input == ',') {
                // next element
                ++input;

            } else if(*input == '}') {
                // last element
                break;
            } else {
                return NULL;
            };
        };
        ++input;
    } else if(*input == '[') {
        ++input;
        // array
        char const * nameStart = SkipWS(input);
        XMLNode item = xml;
        if(*nameStart == ']') while(true) {
            input = ParseInternal(item,input,depth+1);
            if(!input) return NULL;
            input = SkipWS(input);
            if(*input == ',') {
                // next element
                ++input;
                item = xml.NewChild(item.Name());
            } else if(*input == ']') {
                // last element
                item = xml.NewChild(item.Name()); // It will be deleted outside loop
                break;
            } else {
                return NULL;
            };
        };
        item.Destroy();
        ++input;
    } else if(*input == '"') {
        ++input;
        // string
        char const * strStart = input;
        input = SkipTo(strStart, '"');
        if(*input != '"') return NULL;
        xml = std::string(strStart, input-strStart);
        ++input;
    // } else if((*input >= '0') && (*input <= '9')) {
    // } else if(*input == 't') {
    // } else if(*input == 'f') {
    // } else if(*input == 'n') {
    } else {
        ++input;
        // true, false, null, number
        char const * strStart = input;
        while(*input) {
            if((*input == ',') || (*input == '}') || (*input == ']') || (std::isspace(*input)))
                break;
            ++input;
        }
        xml = std::string(strStart, input-strStart);
    };
    return input;
  }

  bool JSON::Parse(Arc::XMLNode& xml, char const * input) {
    if(ParseInternal(xml, input, 0) == NULL)
      return false;
    return true;
  }

} // namespace Arc

