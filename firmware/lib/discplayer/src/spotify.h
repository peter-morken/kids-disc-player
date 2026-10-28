#pragma once

#include <string>

namespace discplayer {

enum class SpotifyKind { Track, Album, Playlist, Episode, Show };

struct SpotifyRef {
  SpotifyKind kind;
  std::string id;
};

// Accepts "spotify:<kind>:<id>" or "https://open.spotify.com/[intl-xx/]<kind>/<id>[?si=...]".
// Short links (spotify.link/...) need an HTTP redirect to resolve and are rejected.
bool parseSpotifyLink(const std::string& link, SpotifyRef& out);

const char* kindName(SpotifyKind kind);

// Sonos music-service "service type" numbers for Spotify. Which one a household
// uses depends on the account region, so callers try one, then the other.
constexpr unsigned kSpotifyServiceEU = 2311;
constexpr unsigned kSpotifyServiceUS = 3079;

struct SonosEnqueue {
  std::string uri;       // EnqueuedURI
  std::string metadata;  // EnqueuedURIMetaData (raw DIDL-Lite, not yet XML-escaped)
};

// Same URI/metadata scheme as SoCo's ShareLinkPlugin (used by Home Assistant).
SonosEnqueue buildSpotifyEnqueue(const SpotifyRef& ref, unsigned serviceType);

}  // namespace discplayer
