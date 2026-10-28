#include "spotify.h"

#include <cctype>
#include <vector>

namespace discplayer {
namespace {

struct KindInfo {
  SpotifyKind kind;
  const char* name;
  const char* uriPrefix;  // prepended to the encoded spotify URI
  const char* idKey;      // prepended to form the DIDL item id
  const char* upnpClass;
};

const KindInfo kKinds[] = {
    {SpotifyKind::Track, "track", "", "00032020", "object.item.audioItem.musicTrack"},
    {SpotifyKind::Album, "album", "x-rincon-cpcontainer:1004206c", "00040000",
     "object.container.album.musicAlbum"},
    {SpotifyKind::Playlist, "playlist", "x-rincon-cpcontainer:1006206c", "1006206c",
     "object.container.playlistContainer"},
    {SpotifyKind::Episode, "episode", "", "00032020", "object.item.audioItem.musicTrack"},
    {SpotifyKind::Show, "show", "x-rincon-cpcontainer:1006206c", "1006206c",
     "object.container.playlistContainer"},
};

const KindInfo* findKind(const std::string& name) {
  for (const auto& k : kKinds)
    if (name == k.name) return &k;
  return nullptr;
}

const KindInfo& infoFor(SpotifyKind kind) {
  for (const auto& k : kKinds)
    if (k.kind == kind) return k;
  return kKinds[0];
}

bool isValidId(const std::string& id) {
  if (id.size() != 22) return false;
  for (char c : id)
    if (!std::isalnum(static_cast<unsigned char>(c))) return false;
  return true;
}

std::vector<std::string> split(const std::string& s, char sep) {
  std::vector<std::string> parts;
  size_t start = 0;
  for (;;) {
    size_t pos = s.find(sep, start);
    parts.push_back(s.substr(start, pos - start));
    if (pos == std::string::npos) return parts;
    start = pos + 1;
  }
}

std::string trim(const std::string& s) {
  size_t b = 0, e = s.size();
  while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
  while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
  return s.substr(b, e - b);
}

bool startsWith(const std::string& s, const char* prefix) {
  return s.rfind(prefix, 0) == 0;
}

bool accept(const std::string& kindName, const std::string& id, SpotifyRef& out) {
  const KindInfo* k = findKind(kindName);
  if (!k || !isValidId(id)) return false;
  out.kind = k->kind;
  out.id = id;
  return true;
}

}  // namespace

const char* kindName(SpotifyKind kind) { return infoFor(kind).name; }

bool parseSpotifyLink(const std::string& raw, SpotifyRef& out) {
  std::string link = trim(raw);

  if (startsWith(link, "spotify:")) {
    auto parts = split(link, ':');
    // spotify:<kind>:<id>, or legacy spotify:user:<name>:playlist:<id>
    if (parts.size() < 3) return false;
    return accept(parts[parts.size() - 2], parts.back(), out);
  }

  for (const char* scheme : {"https://", "http://"}) {
    if (startsWith(link, scheme)) {
      link = link.substr(std::string(scheme).size());
      break;
    }
  }
  if (!startsWith(link, "open.spotify.com/")) return false;
  link = link.substr(link.find('/') + 1);
  link = link.substr(0, link.find_first_of("?#"));

  std::vector<std::string> segs;
  for (auto& s : split(link, '/'))
    if (!s.empty() && !startsWith(s, "intl-")) segs.push_back(s);
  if (segs.size() < 2) return false;
  // Legacy: open.spotify.com/user/<name>/playlist/<id>
  return accept(segs[segs.size() - 2], segs.back(), out);
}

SonosEnqueue buildSpotifyEnqueue(const SpotifyRef& ref, unsigned serviceType) {
  const KindInfo& k = infoFor(ref.kind);
  const std::string encoded = std::string("spotify%3a") + k.name + "%3a" + ref.id;
  const std::string svc = std::to_string(serviceType);

  SonosEnqueue e;
  e.uri = k.uriPrefix + encoded;
  e.metadata =
      "<DIDL-Lite xmlns:dc=\"http://purl.org/dc/elements/1.1/\" "
      "xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\" "
      "xmlns:r=\"urn:schemas-rinconnetworks-com:metadata-1-0/\" "
      "xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\">"
      "<item id=\"" + std::string(k.idKey) + encoded + "\" parentID=\"-1\" restricted=\"true\">"
      "<dc:title></dc:title><upnp:class>" + k.upnpClass + "</upnp:class>"
      "<desc id=\"cdudn\" nameSpace=\"urn:schemas-rinconnetworks-com:metadata-1-0/\">"
      "SA_RINCON" + svc + "_X_#Svc" + svc + "-0-Token</desc></item></DIDL-Lite>";
  return e;
}

}  // namespace discplayer
