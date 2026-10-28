#include "ndef.h"

namespace discplayer {
namespace {

// NFC Forum URI Record Type Definition, table 3.
const char* const kUriPrefixes[] = {
    "",
    "http://www.",
    "https://www.",
    "http://",
    "https://",
    "tel:",
    "mailto:",
    "ftp://anonymous:anonymous@",
    "ftp://ftp.",
    "ftps://",
    "sftp://",
    "smb://",
    "nfs://",
    "ftp://",
    "dav://",
    "news:",
    "telnet://",
    "imap:",
    "rtsp://",
    "urn:",
    "pop:",
    "sip:",
    "sips:",
    "tftp:",
    "btspp://",
    "btl2cap://",
    "btgoep://",
    "tcpobex://",
    "irdaobex://",
    "file://",
    "urn:epc:id:",
    "urn:epc:tag:",
    "urn:epc:pat:",
    "urn:epc:raw:",
    "urn:epc:",
    "urn:nfc:",
};
constexpr size_t kNumUriPrefixes = sizeof(kUriPrefixes) / sizeof(kUriPrefixes[0]);

constexpr uint8_t kTlvNull = 0x00;
constexpr uint8_t kTlvNdef = 0x03;
constexpr uint8_t kTlvTerminator = 0xFE;

constexpr uint8_t kTnfWellKnown = 0x01;

}  // namespace

bool parseNdefMessage(const uint8_t* m, size_t len, std::string& out) {
  size_t i = 0;
  while (i < len) {
    const uint8_t hdr = m[i++];
    const bool messageEnd = hdr & 0x40;
    const bool shortRecord = hdr & 0x10;
    const bool hasId = hdr & 0x08;
    const uint8_t tnf = hdr & 0x07;

    if (i >= len) return false;
    const size_t typeLen = m[i++];

    size_t payloadLen;
    if (shortRecord) {
      if (i >= len) return false;
      payloadLen = m[i++];
    } else {
      if (len - i < 4) return false;
      payloadLen = (static_cast<size_t>(m[i]) << 24) | (static_cast<size_t>(m[i + 1]) << 16) |
                   (static_cast<size_t>(m[i + 2]) << 8) | m[i + 3];
      i += 4;
    }

    size_t idLen = 0;
    if (hasId) {
      if (i >= len) return false;
      idLen = m[i++];
    }

    if (typeLen + idLen > len - i) return false;
    const uint8_t* type = m + i;
    i += typeLen + idLen;
    if (payloadLen > len - i) return false;
    const uint8_t* payload = m + i;
    i += payloadLen;

    if (tnf == kTnfWellKnown && typeLen == 1 && payloadLen > 0) {
      if (type[0] == 'U') {
        const uint8_t code = payload[0];
        out = code < kNumUriPrefixes ? kUriPrefixes[code] : "";
        out.append(reinterpret_cast<const char*>(payload + 1), payloadLen - 1);
        return true;
      }
      if (type[0] == 'T') {
        const size_t langLen = payload[0] & 0x3F;
        if (1 + langLen > payloadLen) return false;
        out.assign(reinterpret_cast<const char*>(payload + 1 + langLen), payloadLen - 1 - langLen);
        return true;
      }
    }

    if (messageEnd) break;
  }
  return false;
}

bool readFirstNdefString(const uint8_t* data, size_t len, std::string& out) {
  size_t i = 0;
  while (i < len) {
    const uint8_t tag = data[i++];
    if (tag == kTlvNull) continue;
    if (tag == kTlvTerminator) return false;

    if (i >= len) return false;
    size_t valueLen = data[i++];
    if (valueLen == 0xFF) {
      if (len - i < 2) return false;
      valueLen = (static_cast<size_t>(data[i]) << 8) | data[i + 1];
      i += 2;
    }
    if (valueLen > len - i) return false;

    if (tag == kTlvNdef) return parseNdefMessage(data + i, valueLen, out);
    i += valueLen;  // skip lock/memory control TLVs
  }
  return false;
}

}  // namespace discplayer
