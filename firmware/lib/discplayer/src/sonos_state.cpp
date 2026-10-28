#include "sonos_state.h"

#include <cctype>
#include <cstdlib>

#include "sonos_discovery.h"

namespace discplayer {

bool soapValue(const std::string& response, const char* tag, std::string& out) {
  const std::string open = std::string("<") + tag + ">";
  const std::string close = std::string("</") + tag + ">";
  size_t start = response.find(open);
  if (start == std::string::npos) {
    // Empty element, e.g. <CurrentURI/>
    if (response.find(std::string("<") + tag + "/>") == std::string::npos) return false;
    out.clear();
    return true;
  }
  start += open.size();
  size_t end = response.find(close, start);
  if (end == std::string::npos) return false;
  out = xmlUnescape(response.substr(start, end - start));
  return true;
}

bool parseVolume(const std::string& response, int& volume) {
  std::string text;
  if (!soapValue(response, "CurrentVolume", text) || text.empty()) return false;
  for (char c : text)
    if (!std::isdigit(static_cast<unsigned char>(c))) return false;
  const int v = std::atoi(text.c_str());
  if (v < 0 || v > 100) return false;
  volume = v;
  return true;
}

bool queueFingerprint(const std::string& browseResponse, std::string& out) {
  std::string total, didl;
  if (!soapValue(browseResponse, "TotalMatches", total) || total.empty()) return false;
  if (!soapValue(browseResponse, "Result", didl)) return false;

  // didl is now the DIDL-Lite document; the <res> text inside is escaped again.
  std::string firstUri;
  size_t res = didl.find("<res");
  if (res != std::string::npos) {
    size_t start = didl.find('>', res);
    size_t end = start == std::string::npos ? std::string::npos : didl.find("</res>", start);
    if (end == std::string::npos) return false;
    firstUri = xmlUnescape(didl.substr(start + 1, end - start - 1));
  }
  out = total + "|" + firstUri;
  return true;
}

}  // namespace discplayer
