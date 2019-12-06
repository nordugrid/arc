// -*- indent-tabs-mode: nil -*-

#include <arc/StringConv.h>
#include <arc/Base64.h>

#include "ClientVOMS.h"

namespace Arc {

ClientVOMS::~ClientVOMS() {
}

ClientVOMS::ClientVOMS(const BaseConfig& cfg, const std::string& host, int port, TCPSec sec, int timeout):
  ClientTCP(cfg, host, port, sec, timeout)
{
}

VOMSCommand& VOMSCommand::GetGroup(const std::string& groupname) {
  str = "G"; str.append(groupname);
  return *this;
}

VOMSCommand& VOMSCommand::GetRole(const std::string& rolename) {
  str = "R"; str.append(rolename);
  return *this;
}

VOMSCommand& VOMSCommand::GetRoleInGroup(const std::string& groupname, const std::string& rolename) {
  str = "B"; str.append(groupname); str.append(":"); str.append(rolename);
  return *this;
}

VOMSCommand& VOMSCommand::GetEverything(void) {
  str = "A";
  return *this;
}

VOMSCommand& VOMSCommand::GetFQANs(void) {
  str = "M";
  return *this;
}

VOMSCommand& VOMSCommand::GetFQAN(const std::string& fqan) {
  str = fqan;
  return *this;
}

// EBCDIC is not supported
static bool VOMSDecodeChar(unsigned char& c) {
  if((c >= (unsigned char)'a') && (c <= (unsigned char)'z')) {
    return (c - (unsigned char)'a');
  }
  if((c >= (unsigned char)'A') && (c <= (unsigned char)'Z')) {
    c =    (c - (unsigned char)'A') +
           ((unsigned char)'z' - (unsigned char)'a' + 1);
    return true;
  }
  if((c >= (unsigned char)'0') && (c <= (unsigned char)'9')) {
    c =    (c - (unsigned char)'0') +
           ((unsigned char)'z' - (unsigned char)'a' + 1) +
           ((unsigned char)'Z' - (unsigned char)'A' + 1);
    return true;
  }
  if(c == (unsigned char)'[') {
    c =    0 +
           ((unsigned char)'z' - (unsigned char)'a' + 1) +
           ((unsigned char)'Z' - (unsigned char)'A' + 1) +
           ((unsigned char)'9' - (unsigned char)'0' + 1);
    return true;
  }
  if(c == (unsigned char)']') {
    c =    1 +
           ((unsigned char)'z' - (unsigned char)'a' + 1) +
           ((unsigned char)'Z' - (unsigned char)'A' + 1) +
           ((unsigned char)'9' - (unsigned char)'0' + 1);
    return true;
  }
  return false;
}

static bool VOMSDecodeData(const std::string& in, std::string& out) {
  std::string::size_type p_in = 0;
  std::string::size_type p_out = 0;
  // 4 bytes -> 3 bytes
  out.resize((in.length()+3)/4*3, '\0');
  if(out.empty()) return true;
  for(;;) {
    unsigned char c;
    if(p_in >= in.length()) break;
    c = in[p_in++]; if(!VOMSDecodeChar(c)) return false;
    out[p_out] = c<<2;
    if(p_in >= in.length()) { ++p_out; break; }
    c = in[p_in++]; if(!VOMSDecodeChar(c)) return false;
    out[p_out] |= c>>4; ++p_out; out[p_out] = c<<4;
    if(p_in >= in.length()) { ++p_out; break; }
    c = in[p_in++]; if(!VOMSDecodeChar(c)) return false;
    out[p_out] |= c>>2; ++p_out; out[p_out] = c<<6;
    if(p_in >= in.length()) { ++p_out; break; }
    c = in[p_in++]; if(!VOMSDecodeChar(c)) return false;
    out[p_out] |= c; ++p_out;
  }
  out.resize(p_out);
  return true;
} 

MCC_Status ClientVOMS::Load() {
  MCC_Status r(STATUS_OK);
  if(!(r=ClientTCP::Load())) return r;
  return r;
}

MCC_Status ClientVOMS::process(const VOMSCommand& command,
                               const Period& lifetime,
                               std::string& result) {
  std::list<VOMSCommand> commands;
  const std::list<std::pair<std::string,std::string> > order;
  commands.push_back(command);
  return process(commands, order, lifetime, result);
}

MCC_Status ClientVOMS::process(const VOMSCommand& command,
                               const std::list<std::pair<std::string,std::string> >& order,
                               const Period& lifetime,
                               std::string& result) {
  std::list<VOMSCommand> commands;
  commands.push_back(command);
  return process(commands, order, lifetime, result);
}

MCC_Status ClientVOMS::process(const std::list<VOMSCommand>& commands,
                               const Period& lifetime,
                               std::string& result) {
  const std::list<std::pair<std::string,std::string> > order;
  return process(commands, order, lifetime, result);
}

MCC_Status ClientVOMS::process(const std::list<VOMSCommand>& commands,
                               const std::list<std::pair<std::string,std::string> >& order,
                               const Period& lifetime,
                               std::string& result) {
  if(commands.empty()) return MCC_Status(STATUS_OK); // No request - no response
  //voms
  // command +
  // order ?
  // targets ?
  // lifetime ?
  // base64 ?
  // version ?
  XMLNode msg(NS(),"voms");
  for(std::list<VOMSCommand>::const_iterator cmd = commands.begin(); cmd != commands.end(); ++cmd) {
    msg.NewChild("command") = cmd->Str();
  }
  std::string ord_str;
  for(std::list<std::pair<std::string,std::string> >::const_iterator ord = order.begin(); ord != order.end(); ++ord) {
    if(!ord_str.empty()) ord_str += ",";
    ord_str += ord->first;
    if(!ord->second.empty()) ord_str += (":"+ord->second);
  }
  if(!ord_str.empty()) msg.NewChild("order") = ord_str;
  if(lifetime.GetPeriod() > 0) msg.NewChild("lifetime") = tostring(lifetime.GetPeriod());
  // Leaving base64 and version default to avoid dealing with various versions of service
  
  Arc::PayloadRaw request;
  {
    std::string msg_str;
    msg.GetXML(msg_str,"US-ASCII");
    msg_str.insert(0,"<?xml version=\"1.0\" encoding=\"US-ASCII\"?>");
    request.Insert(msg_str.c_str(), 0, msg_str.length());
  }
  Arc::PayloadStreamInterface *response = NULL;
  Arc::MCC_Status status = ClientTCP::process(&request, &response, true);
  if(!status) {
    if(response) delete response;
    return status;
  }
  if(!response) {
    return MCC_Status(GENERIC_ERROR,"VOMS","Response is empty");
  }

  // vomsans
  //  error ?
  //   item *
  //    number
  //    message
  //  bitstr
  //  ac
  //  version

  // It is not clear how VOMS combines answers to different commands.
  // So we are assuming it is always one answer
  std::string resp_str;
  // TODO: something more effective is needed
  do {
    char buf[1024];
    int len = sizeof(buf);
    if(!response->Get(buf,len)) break;
    resp_str.append(buf,len);
    if(resp_str.length() > 4*1024*1024) break; // Some sanity check
  } while(true);
  delete response;
  XMLNode resp(resp_str);
  if(!resp) {
    return MCC_Status(GENERIC_ERROR,"VOMS","Response is not recognized as XML");
  }
  if(resp.Name() != "vomsans") {
    return MCC_Status(GENERIC_ERROR,"VOMS","Response is missing required 'vomsans' element");
  }
  if(resp["ac"]) {
    result = "-----BEGIN VOMS AC-----\n"+(std::string)resp["ac"]+"\n-----END VOMS AC-----";
  } else if(resp["bitstr"]) {
    std::string bitstr = (std::string)resp["bitstr"];
    if(!VOMSDecodeData(bitstr,result)) {
      result = Base64::decode(bitstr);
    }
  } else {
    result.resize(0);
  }
  if(resp["error"]) {
    std::string err_str;
    for(XMLNode err = resp["error"]["item"]; (bool)err; ++err) {
      if(!err_str.empty()) err_str += "\n";
      err_str += (std::string)(err["message"]);
    }
    return MCC_Status(GENERIC_ERROR,"VOMS",err_str);
  }
  return MCC_Status(STATUS_OK);
}

} // namespace Arc

