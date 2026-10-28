#pragma once

#include <string>

namespace discplayer {

struct Version {
  unsigned major = 0, minor = 0, patch = 0;
};

// Accepts "v1.2.3" or "1.2.3", with surrounding whitespace (e.g. a trailing
// newline from version.txt). Pre-release suffixes are not supported.
bool parseVersion(const std::string& s, Version& out);

// True only if both parse and `candidate` is strictly newer. Anything that
// doesn't parse (including the "dev" builds flashed over USB) never updates.
bool isNewerVersion(const std::string& candidate, const std::string& current);

}  // namespace discplayer
