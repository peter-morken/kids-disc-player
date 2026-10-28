#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace discplayer {

// Parses the raw user memory of an NFC Forum Type 2 tag (NTAG213/215/216),
// starting at page 4, and returns the text of the first NDEF URI ("U") or
// Text ("T") record. Returns false if there is no such record or the data is
// malformed/truncated.
bool readFirstNdefString(const uint8_t* data, size_t len, std::string& out);

// Parses a bare NDEF message (no TLV wrapper).
bool parseNdefMessage(const uint8_t* msg, size_t len, std::string& out);

}  // namespace discplayer
