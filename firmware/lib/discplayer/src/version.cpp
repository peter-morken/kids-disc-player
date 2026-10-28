#include "version.h"

#include <cctype>

namespace discplayer {
namespace {

bool parseNumber(const std::string& s, size_t& i, unsigned& out) {
  const size_t start = i;
  out = 0;
  while (i < s.size() && std::isdigit(static_cast<unsigned char>(s[i]))) {
    if (i - start >= 6) return false;  // absurdly long component
    out = out * 10 + static_cast<unsigned>(s[i++] - '0');
  }
  return i > start;
}

}  // namespace

bool parseVersion(const std::string& raw, Version& out) {
  size_t b = 0, e = raw.size();
  while (b < e && std::isspace(static_cast<unsigned char>(raw[b]))) ++b;
  while (e > b && std::isspace(static_cast<unsigned char>(raw[e - 1]))) --e;
  const std::string s = raw.substr(b, e - b);

  size_t i = (!s.empty() && (s[0] == 'v' || s[0] == 'V')) ? 1 : 0;
  Version v;
  if (!parseNumber(s, i, v.major) || i >= s.size() || s[i++] != '.') return false;
  if (!parseNumber(s, i, v.minor) || i >= s.size() || s[i++] != '.') return false;
  if (!parseNumber(s, i, v.patch) || i != s.size()) return false;
  out = v;
  return true;
}

bool isNewerVersion(const std::string& candidate, const std::string& current) {
  Version a, b;
  if (!parseVersion(candidate, a) || !parseVersion(current, b)) return false;
  if (a.major != b.major) return a.major > b.major;
  if (a.minor != b.minor) return a.minor > b.minor;
  return a.patch > b.patch;
}

}  // namespace discplayer
