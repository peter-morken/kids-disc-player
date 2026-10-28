#include <unity.h>

#include <string>
#include <vector>

#include "disc_session.h"
#include "ndef.h"
#include "soap.h"
#include "sonos_discovery.h"
#include "spotify.h"
#include "version.h"

using namespace discplayer;

#define ASSERT_ACTION(expected, actual) TEST_ASSERT_EQUAL_INT(static_cast<int>(expected), static_cast<int>(actual))

void setUp() {}
void tearDown() {}

// ---------- NDEF ----------

// Tag user memory as written by e.g. the "NFC Tools" phone app: a URI record
// for https://open.spotify.com/album/... wrapped in an NDEF TLV.
std::vector<uint8_t> uriTag(uint8_t prefixCode, const std::string& rest) {
  std::vector<uint8_t> rec = {0xD1, 0x01, static_cast<uint8_t>(rest.size() + 1), 'U', prefixCode};
  rec.insert(rec.end(), rest.begin(), rest.end());
  std::vector<uint8_t> tlv = {0x03, static_cast<uint8_t>(rec.size())};
  tlv.insert(tlv.end(), rec.begin(), rec.end());
  tlv.push_back(0xFE);
  return tlv;
}

void test_ndef_uri_record_with_https_prefix() {
  auto tag = uriTag(0x04, "open.spotify.com/album/1ATL5GLyefJaxhQzSPVrLX");
  std::string out;
  TEST_ASSERT_TRUE(readFirstNdefString(tag.data(), tag.size(), out));
  TEST_ASSERT_EQUAL_STRING("https://open.spotify.com/album/1ATL5GLyefJaxhQzSPVrLX", out.c_str());
}

void test_ndef_text_record() {
  const std::string text = "spotify:track:4uLU6hMCjMI75M1A2tKUQC";
  std::vector<uint8_t> rec = {0xD1, 0x01, static_cast<uint8_t>(text.size() + 3), 'T', 0x02, 'e', 'n'};
  rec.insert(rec.end(), text.begin(), text.end());
  std::vector<uint8_t> tag = {0x03, static_cast<uint8_t>(rec.size())};
  tag.insert(tag.end(), rec.begin(), rec.end());
  std::string out;
  TEST_ASSERT_TRUE(readFirstNdefString(tag.data(), tag.size(), out));
  TEST_ASSERT_EQUAL_STRING(text.c_str(), out.c_str());
}

void test_ndef_skips_lock_control_tlv_and_nulls() {
  auto inner = uriTag(0x00, "spotify:playlist:37i9dQZF1DXcBWIGoYBM5M");
  std::vector<uint8_t> tag = {0x00, 0x01, 0x03, 0xA0, 0x10, 0x44, 0x00};
  tag.insert(tag.end(), inner.begin(), inner.end());
  std::string out;
  TEST_ASSERT_TRUE(readFirstNdefString(tag.data(), tag.size(), out));
  TEST_ASSERT_EQUAL_STRING("spotify:playlist:37i9dQZF1DXcBWIGoYBM5M", out.c_str());
}

void test_ndef_truncated_data_fails_until_complete() {
  auto tag = uriTag(0x04, "open.spotify.com/track/4uLU6hMCjMI75M1A2tKUQC");
  std::string out;
  for (size_t n = 0; n < tag.size() - 1; ++n)  // everything but the terminator is needed
    TEST_ASSERT_FALSE_MESSAGE(readFirstNdefString(tag.data(), n, out), "parsed a truncated tag");
  TEST_ASSERT_TRUE(readFirstNdefString(tag.data(), tag.size() - 1, out));
}

void test_ndef_blank_tag() {
  const uint8_t blank[] = {0x03, 0x00, 0xFE, 0x00};
  std::string out;
  TEST_ASSERT_FALSE(readFirstNdefString(blank, sizeof(blank), out));
}

void test_ndef_long_tlv_length() {
  const std::string rest(300, 'a');
  std::vector<uint8_t> rec = {0xC1, 0x01, 0x00, 0x00, 0x01, static_cast<uint8_t>(301 - 256), 'U', 0x00};
  rec.insert(rec.end(), rest.begin(), rest.end());
  std::vector<uint8_t> tag = {0x03, 0xFF, static_cast<uint8_t>(rec.size() >> 8), static_cast<uint8_t>(rec.size())};
  tag.insert(tag.end(), rec.begin(), rec.end());
  std::string out;
  TEST_ASSERT_TRUE(readFirstNdefString(tag.data(), tag.size(), out));
  TEST_ASSERT_EQUAL(300, out.size());
}

// ---------- Spotify links ----------

void test_spotify_parses_supported_forms() {
  struct Case {
    const char* link;
    SpotifyKind kind;
  } cases[] = {
      {"spotify:track:4uLU6hMCjMI75M1A2tKUQC", SpotifyKind::Track},
      {"https://open.spotify.com/album/1ATL5GLyefJaxhQzSPVrLX?si=abc123", SpotifyKind::Album},
      {"open.spotify.com/playlist/37i9dQZF1DXcBWIGoYBM5M", SpotifyKind::Playlist},
      {"https://open.spotify.com/intl-de/track/4uLU6hMCjMI75M1A2tKUQC", SpotifyKind::Track},
      {"  spotify:user:someone:playlist:37i9dQZF1DXcBWIGoYBM5M\n", SpotifyKind::Playlist},
      {"https://open.spotify.com/show/4rOoJ6Egrf8K2IrywzwOMk#x", SpotifyKind::Show},
  };
  for (const auto& c : cases) {
    SpotifyRef ref;
    TEST_ASSERT_TRUE_MESSAGE(parseSpotifyLink(c.link, ref), c.link);
    TEST_ASSERT_EQUAL_MESSAGE(static_cast<int>(c.kind), static_cast<int>(ref.kind), c.link);
    TEST_ASSERT_EQUAL_MESSAGE(22, ref.id.size(), c.link);
  }
}

void test_spotify_rejects_unsupported() {
  const char* bad[] = {
      "https://spotify.link/abcdef",                       // short link, needs redirect
      "spotify:artist:0OdUWJ0sBjDrqHygGUXeCF",             // artists aren't queueable
      "https://open.spotify.com/track/tooShort",
      "https://example.com/track/4uLU6hMCjMI75M1A2tKUQC",
      "",
  };
  for (const char* link : bad) {
    SpotifyRef ref;
    TEST_ASSERT_FALSE_MESSAGE(parseSpotifyLink(link, ref), link);
  }
}

// Expected values are what SoCo's ShareLinkPlugin sends for the same links.
void test_sonos_enqueue_album_matches_soco() {
  SpotifyRef ref{SpotifyKind::Album, "1ATL5GLyefJaxhQzSPVrLX"};
  auto e = buildSpotifyEnqueue(ref, kSpotifyServiceEU);
  TEST_ASSERT_EQUAL_STRING("x-rincon-cpcontainer:1004206cspotify%3aalbum%3a1ATL5GLyefJaxhQzSPVrLX",
                           e.uri.c_str());
  TEST_ASSERT_NOT_EQUAL(std::string::npos,
                        e.metadata.find("<item id=\"00040000spotify%3aalbum%3a1ATL5GLyefJaxhQzSPVrLX\""));
  TEST_ASSERT_NOT_EQUAL(std::string::npos,
                        e.metadata.find("<upnp:class>object.container.album.musicAlbum</upnp:class>"));
  TEST_ASSERT_NOT_EQUAL(std::string::npos, e.metadata.find(">SA_RINCON2311_X_#Svc2311-0-Token</desc>"));
}

void test_sonos_enqueue_track_us() {
  SpotifyRef ref{SpotifyKind::Track, "4uLU6hMCjMI75M1A2tKUQC"};
  auto e = buildSpotifyEnqueue(ref, kSpotifyServiceUS);
  TEST_ASSERT_EQUAL_STRING("spotify%3atrack%3a4uLU6hMCjMI75M1A2tKUQC", e.uri.c_str());
  TEST_ASSERT_NOT_EQUAL(std::string::npos, e.metadata.find("item id=\"00032020spotify%3atrack%3a"));
  TEST_ASSERT_NOT_EQUAL(std::string::npos, e.metadata.find("SA_RINCON3079_X_#Svc3079-0-Token"));
}

// ---------- SOAP ----------

void test_soap_envelope_escapes_args() {
  auto body = buildSoapEnvelope("urn:schemas-upnp-org:service:AVTransport:1", "AddURIToQueue",
                                {{"InstanceID", "0"}, {"EnqueuedURIMetaData", "<a b=\"c\">&</a>"}});
  TEST_ASSERT_NOT_EQUAL(std::string::npos,
                        body.find("<u:AddURIToQueue xmlns:u=\"urn:schemas-upnp-org:service:AVTransport:1\">"));
  TEST_ASSERT_NOT_EQUAL(std::string::npos,
                        body.find("<EnqueuedURIMetaData>&lt;a b=&quot;c&quot;&gt;&amp;&lt;/a&gt;</EnqueuedURIMetaData>"));
  TEST_ASSERT_NOT_EQUAL(std::string::npos, body.find("</u:AddURIToQueue></s:Body></s:Envelope>"));
}

void test_extract_rincon_id() {
  const std::string xml =
      "<root><device><deviceType>urn:schemas-upnp-org:device:ZonePlayer:1</deviceType>"
      "<UDN>uuid:RINCON_48A6B8C0FFEE01400</UDN><deviceList><device>"
      "<UDN>uuid:RINCON_48A6B8C0FFEE01400_MR</UDN></device></deviceList></device></root>";
  std::string id;
  TEST_ASSERT_TRUE(extractRinconId(xml, id));
  TEST_ASSERT_EQUAL_STRING("RINCON_48A6B8C0FFEE01400", id.c_str());
  TEST_ASSERT_FALSE(extractRinconId("<html>not sonos</html>", id));
}

// ---------- Sonos discovery ----------

void test_ssdp_response_from_sonos() {
  const std::string resp =
      "HTTP/1.1 200 OK\r\n"
      "CACHE-CONTROL: max-age = 1800\r\n"
      "EXT:\r\n"
      "LOCATION: http://192.168.1.20:1400/xml/device_description.xml\r\n"
      "SERVER: Linux UPnP/1.0 Sonos/80.1-55014 (ZPS40)\r\n"
      "ST: urn:schemas-upnp-org:device:ZonePlayer:1\r\n"
      "USN: uuid:RINCON_48A6B8C0FFEE01400::urn:schemas-upnp-org:device:ZonePlayer:1\r\n\r\n";
  std::string host;
  TEST_ASSERT_TRUE(parseSsdpSonosHost(resp, host));
  TEST_ASSERT_EQUAL_STRING("192.168.1.20", host.c_str());
}

void test_ssdp_response_from_other_device_ignored() {
  const std::string resp =
      "HTTP/1.1 200 OK\r\nLOCATION: http://192.168.1.1:49000/igd.xml\r\n"
      "SERVER: FRITZ!Box UPnP/1.0\r\nST: upnp:rootdevice\r\n\r\n";
  std::string host;
  TEST_ASSERT_FALSE(parseSsdpSonosHost(resp, host));
}

// Trimmed GetZoneGroupState response: a stereo pair in the kids' room (the
// right speaker is invisible), a grouped living room, and an apostrophe in a
// room name, which arrives escaped twice.
const char kZoneGroupStateResponse[] =
    "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\"><s:Body>"
    "<u:GetZoneGroupStateResponse xmlns:u=\"urn:schemas-upnp-org:service:ZoneGroupTopology:1\">"
    "<ZoneGroupState>&lt;ZoneGroupState&gt;&lt;ZoneGroups&gt;"
    "&lt;ZoneGroup Coordinator=&quot;RINCON_KIDSR01400&quot; ID=&quot;RINCON_KIDSL01400:1&quot;&gt;"
    "&lt;ZoneGroupMember UUID=&quot;RINCON_KIDSR01400&quot; "
    "Location=&quot;http://192.168.1.31:1400/xml/device_description.xml&quot; "
    "ZoneName=&quot;Kids Room&quot; Invisible=&quot;1&quot; SoftwareVersion=&quot;80.1&quot;/&gt;"
    "&lt;ZoneGroupMember UUID=&quot;RINCON_KIDSL01400&quot; "
    "Location=&quot;http://192.168.1.30:1400/xml/device_description.xml&quot; "
    "ZoneName=&quot;Kids Room&quot; SoftwareVersion=&quot;80.1&quot;/&gt;"
    "&lt;/ZoneGroup&gt;"
    "&lt;ZoneGroup Coordinator=&quot;RINCON_LIVING01400&quot; ID=&quot;RINCON_LIVING01400:7&quot;&gt;"
    "&lt;ZoneGroupMember UUID=&quot;RINCON_LIVING01400&quot; "
    "Location=&quot;http://192.168.1.40:1400/xml/device_description.xml&quot; ZoneName=&quot;Living Room&quot;&gt;"
    "&lt;Satellite UUID=&quot;RINCON_SUB01400&quot; "
    "Location=&quot;http://192.168.1.41:1400/xml/device_description.xml&quot; ZoneName=&quot;Living Room&quot;/&gt;"
    "&lt;/ZoneGroupMember&gt;"
    "&lt;ZoneGroupMember UUID=&quot;RINCON_EMMA01400&quot; "
    "Location=&quot;http://192.168.1.50:1400/xml/device_description.xml&quot; "
    "ZoneName=&quot;Emma&amp;apos;s Room&quot;/&gt;"
    "&lt;/ZoneGroup&gt;&lt;/ZoneGroups&gt;&lt;/ZoneGroupState&gt;</ZoneGroupState>"
    "</u:GetZoneGroupStateResponse></s:Body></s:Envelope>";

void test_zone_group_state_skips_invisible_pair_member() {
  SonosSpeaker sp;
  TEST_ASSERT_TRUE(findRoomInZoneGroupState(kZoneGroupStateResponse, "kids room", sp));
  TEST_ASSERT_EQUAL_STRING("192.168.1.30", sp.host.c_str());
  TEST_ASSERT_EQUAL_STRING("RINCON_KIDSL01400", sp.uuid.c_str());
}

void test_zone_group_state_room_in_group_and_escaped_name() {
  SonosSpeaker sp;
  TEST_ASSERT_TRUE(findRoomInZoneGroupState(kZoneGroupStateResponse, "  Emma's Room ", sp));
  TEST_ASSERT_EQUAL_STRING("192.168.1.50", sp.host.c_str());
  TEST_ASSERT_TRUE(findRoomInZoneGroupState(kZoneGroupStateResponse, "Living Room", sp));
  TEST_ASSERT_EQUAL_STRING("RINCON_LIVING01400", sp.uuid.c_str());
}

void test_zone_group_state_unknown_room() {
  SonosSpeaker sp;
  TEST_ASSERT_FALSE(findRoomInZoneGroupState(kZoneGroupStateResponse, "Kitchen", sp));
  TEST_ASSERT_FALSE(findRoomInZoneGroupState(kZoneGroupStateResponse, "", sp));
  TEST_ASSERT_FALSE(findRoomInZoneGroupState("<html>500</html>", "Kids Room", sp));
}

// ---------- Versions ----------

void test_version_parsing() {
  Version v;
  TEST_ASSERT_TRUE(parseVersion("v1.12.3\n", v));
  TEST_ASSERT_EQUAL(1, v.major);
  TEST_ASSERT_EQUAL(12, v.minor);
  TEST_ASSERT_EQUAL(3, v.patch);
  TEST_ASSERT_TRUE(parseVersion("0.1.0", v));
  const char* bad[] = {"dev", "", "v1.2", "v1.2.3-rc1", "1.2.3.4", "<!DOCTYPE html>", "v1..3"};
  for (const char* s : bad) TEST_ASSERT_FALSE_MESSAGE(parseVersion(s, v), s);
}

void test_version_ordering() {
  TEST_ASSERT_TRUE(isNewerVersion("v1.0.1", "v1.0.0"));
  TEST_ASSERT_TRUE(isNewerVersion("v1.10.0", "v1.9.9"));  // numeric, not lexical
  TEST_ASSERT_TRUE(isNewerVersion("v2.0.0\n", "v1.99.99"));
  TEST_ASSERT_FALSE(isNewerVersion("v1.0.0", "v1.0.0"));
  TEST_ASSERT_FALSE(isNewerVersion("v0.9.0", "v1.0.0"));  // never downgrade
  TEST_ASSERT_FALSE(isNewerVersion("v9.9.9", "dev"));     // USB dev builds stay put
  TEST_ASSERT_FALSE(isNewerVersion("Not Found", "v1.0.0"));
}

// ---------- Disc session ----------

void test_session_insert_remove_resume() {
  DiscSession s(1500, 60000);
  ASSERT_ACTION(DiscAction::None, s.update(0, ""));
  ASSERT_ACTION(DiscAction::PlayNew, s.update(100, "A"));
  ASSERT_ACTION(DiscAction::None, s.update(250, "A"));
  // A missed read shorter than the debounce does nothing.
  ASSERT_ACTION(DiscAction::None, s.update(400, ""));
  ASSERT_ACTION(DiscAction::None, s.update(550, "A"));
  // Removed for good.
  ASSERT_ACTION(DiscAction::None, s.update(1000, ""));
  ASSERT_ACTION(DiscAction::Pause, s.update(2100, ""));
  ASSERT_ACTION(DiscAction::None, s.update(3000, ""));
  // Same disc back within the window resumes.
  ASSERT_ACTION(DiscAction::Resume, s.update(10000, "A"));
}

void test_session_resume_window_expires() {
  DiscSession s(1500, 60000);
  s.update(0, "A");
  ASSERT_ACTION(DiscAction::Pause, s.update(2000, ""));
  ASSERT_ACTION(DiscAction::PlayNew, s.update(2000 + 60001, "A"));
}

void test_session_different_disc_and_swap() {
  DiscSession s(1500, 60000);
  s.update(0, "A");
  s.update(2000, "");
  ASSERT_ACTION(DiscAction::PlayNew, s.update(3000, "B"));
  // Swapped faster than the debounce: still a new disc.
  ASSERT_ACTION(DiscAction::PlayNew, s.update(3150, "C"));
}

void test_session_invalid_disc_does_not_pause() {
  DiscSession s(1500, 60000);
  ASSERT_ACTION(DiscAction::PlayNew, s.update(0, "BLANK"));
  s.markActiveInvalid();
  ASSERT_ACTION(DiscAction::None, s.update(150, "BLANK"));  // no retry loop
  ASSERT_ACTION(DiscAction::None, s.update(2000, ""));      // no pause on removal
  ASSERT_ACTION(DiscAction::PlayNew, s.update(3000, "BLANK"));  // re-insert retries
}

void test_session_survives_millis_wraparound() {
  DiscSession s(1500, 60000);
  const uint32_t t0 = 0xFFFFFF00u;
  s.update(t0, "A");
  ASSERT_ACTION(DiscAction::None, s.update(t0 + 1000, ""));
  ASSERT_ACTION(DiscAction::Pause, s.update(t0 + 1600, ""));
  ASSERT_ACTION(DiscAction::Resume, s.update(t0 + 5000, "A"));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_ndef_uri_record_with_https_prefix);
  RUN_TEST(test_ndef_text_record);
  RUN_TEST(test_ndef_skips_lock_control_tlv_and_nulls);
  RUN_TEST(test_ndef_truncated_data_fails_until_complete);
  RUN_TEST(test_ndef_blank_tag);
  RUN_TEST(test_ndef_long_tlv_length);
  RUN_TEST(test_spotify_parses_supported_forms);
  RUN_TEST(test_spotify_rejects_unsupported);
  RUN_TEST(test_sonos_enqueue_album_matches_soco);
  RUN_TEST(test_sonos_enqueue_track_us);
  RUN_TEST(test_soap_envelope_escapes_args);
  RUN_TEST(test_extract_rincon_id);
  RUN_TEST(test_session_insert_remove_resume);
  RUN_TEST(test_session_resume_window_expires);
  RUN_TEST(test_session_different_disc_and_swap);
  RUN_TEST(test_session_invalid_disc_does_not_pause);
  RUN_TEST(test_session_survives_millis_wraparound);
  RUN_TEST(test_ssdp_response_from_sonos);
  RUN_TEST(test_ssdp_response_from_other_device_ignored);
  RUN_TEST(test_zone_group_state_skips_invisible_pair_member);
  RUN_TEST(test_zone_group_state_room_in_group_and_escaped_name);
  RUN_TEST(test_zone_group_state_unknown_room);
  RUN_TEST(test_version_parsing);
  RUN_TEST(test_version_ordering);
  return UNITY_END();
}
