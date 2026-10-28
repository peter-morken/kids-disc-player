#include "soap.h"

namespace discplayer {

std::string xmlEscape(const std::string& s) {
  std::string out;
  out.reserve(s.size() + s.size() / 4);
  for (char c : s) {
    switch (c) {
      case '&': out += "&amp;"; break;
      case '<': out += "&lt;"; break;
      case '>': out += "&gt;"; break;
      case '"': out += "&quot;"; break;
      case '\'': out += "&apos;"; break;
      default: out += c;
    }
  }
  return out;
}

std::string buildSoapEnvelope(const std::string& serviceUrn, const std::string& action,
                              const SoapArgs& args) {
  std::string body =
      "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
      "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" "
      "s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\"><s:Body>"
      "<u:" + action + " xmlns:u=\"" + serviceUrn + "\">";
  for (const auto& a : args) body += "<" + a.first + ">" + xmlEscape(a.second) + "</" + a.first + ">";
  body += "</u:" + action + "></s:Body></s:Envelope>";
  return body;
}

bool extractRinconId(const std::string& xml, std::string& out) {
  const std::string open = "<UDN>uuid:";
  size_t start = xml.find(open);
  if (start == std::string::npos) return false;
  start += open.size();
  size_t end = xml.find("</UDN>", start);
  if (end == std::string::npos || end == start) return false;
  out = xml.substr(start, end - start);
  return out.rfind("RINCON_", 0) == 0;
}

}  // namespace discplayer
