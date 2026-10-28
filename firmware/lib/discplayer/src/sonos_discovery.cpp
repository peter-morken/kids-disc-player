#include "sonos_discovery.h"

#include <cctype>

namespace discplayer {
namespace {

std::string lower(std::string s) {
  for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return s;
}

std::string trim(const std::string& s) {
  size_t b = 0, e = s.size();
  while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
  while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
  return s.substr(b, e - b);
}

// Value of attr="..." inside a single start tag.
bool attribute(const std::string& tag, const char* name, std::string& out) {
  const std::string key = std::string(" ") + name + "=\"";
  size_t start = tag.find(key);
  if (start == std::string::npos) return false;
  start += key.size();
  size_t end = tag.find('"', start);
  if (end == std::string::npos) return false;
  out = xmlUnescape(tag.substr(start, end - start));
  return true;
}

}  // namespace

const char kSonosSsdpSearch[] =
    "M-SEARCH * HTTP/1.1\r\n"
    "HOST: 239.255.255.250:1900\r\n"
    "MAN: \"ssdp:discover\"\r\n"
    "MX: 1\r\n"
    "ST: urn:schemas-upnp-org:device:ZonePlayer:1\r\n"
    "\r\n";

bool urlHost(const std::string& url, std::string& host) {
  const std::string scheme = "http://";
  if (lower(url.substr(0, scheme.size())) != scheme) return false;
  size_t start = scheme.size();
  size_t end = url.find_first_of(":/", start);
  host = url.substr(start, end == std::string::npos ? std::string::npos : end - start);
  return !host.empty();
}

bool parseSsdpSonosHost(const std::string& response, std::string& host) {
  bool isSonos = false;
  std::string location;
  size_t pos = 0;
  while (pos < response.size()) {
    size_t eol = response.find('\n', pos);
    std::string line = response.substr(pos, eol == std::string::npos ? std::string::npos : eol - pos);
    pos = eol == std::string::npos ? response.size() : eol + 1;

    size_t colon = line.find(':');
    if (colon == std::string::npos) continue;
    const std::string name = lower(trim(line.substr(0, colon)));
    const std::string value = trim(line.substr(colon + 1));
    if (name == "location") location = value;
    if (name == "st" && value.find("ZonePlayer") != std::string::npos) isSonos = true;
    if (name == "server" && lower(value).find("sonos") != std::string::npos) isSonos = true;
  }
  return isSonos && urlHost(location, host);
}

std::string xmlUnescape(const std::string& s) {
  static const struct {
    const char* entity;
    char c;
  } kEntities[] = {{"&lt;", '<'}, {"&gt;", '>'}, {"&quot;", '"'}, {"&apos;", '\''}, {"&amp;", '&'}};

  std::string out;
  out.reserve(s.size());
  for (size_t i = 0; i < s.size();) {
    bool replaced = false;
    if (s[i] == '&') {
      for (const auto& e : kEntities) {
        const std::string ent = e.entity;
        if (s.compare(i, ent.size(), ent) == 0) {
          out += e.c;
          i += ent.size();
          replaced = true;
          break;
        }
      }
    }
    if (!replaced) out += s[i++];
  }
  return out;
}

bool findRoomInZoneGroupState(const std::string& soapResponse, const std::string& room,
                              SonosSpeaker& out) {
  const std::string open = "<ZoneGroupState>";
  size_t start = soapResponse.find(open);
  if (start == std::string::npos) return false;
  start += open.size();
  size_t end = soapResponse.find("</ZoneGroupState>", start);
  if (end == std::string::npos) return false;
  // The state is an XML document escaped inside the SOAP response.
  const std::string state = xmlUnescape(soapResponse.substr(start, end - start));
  const std::string wanted = lower(trim(room));

  const std::string memberTag = "<ZoneGroupMember ";
  for (size_t pos = state.find(memberTag); pos != std::string::npos;
       pos = state.find(memberTag, pos + 1)) {
    size_t tagEnd = state.find('>', pos);
    if (tagEnd == std::string::npos) break;
    const std::string tag = state.substr(pos, tagEnd - pos);

    std::string name, invisible, uuid, location, host;
    if (!attribute(tag, "ZoneName", name) || lower(trim(name)) != wanted) continue;
    if (attribute(tag, "Invisible", invisible) && invisible == "1") continue;
    if (!attribute(tag, "UUID", uuid) || !attribute(tag, "Location", location)) continue;
    if (!urlHost(location, host)) continue;
    out.host = host;
    out.uuid = uuid;
    return true;
  }
  return false;
}

}  // namespace discplayer
