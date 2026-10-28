#pragma once

#include <string>

namespace discplayer {

// Text content of the first <tag>...</tag> in a SOAP response, XML-unescaped.
bool soapValue(const std::string& response, const char* tag, std::string& out);

// RenderingControl#GetVolume response -> 0..100.
bool parseVolume(const std::string& response, int& volume);

// Identifies what is in a player's queue, from a ContentDirectory#Browse of
// "Q:0" with RequestedCount=1: the total track count plus the first track's
// URI. Unchanged while the queue plays through; different once someone
// replaces the queue from the Sonos app. Used to tell "still our disc" apart
// from "a parent took over the speaker".
bool queueFingerprint(const std::string& browseResponse, std::string& out);

}  // namespace discplayer
