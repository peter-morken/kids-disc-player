#pragma once

#include <string>
#include <utility>
#include <vector>

namespace discplayer {

using SoapArgs = std::vector<std::pair<std::string, std::string>>;

std::string xmlEscape(const std::string& s);

// Builds a UPnP SOAP request body. Argument values are XML-escaped.
std::string buildSoapEnvelope(const std::string& serviceUrn, const std::string& action,
                              const SoapArgs& args);

// Extracts "RINCON_xxxxxxxxxxxx01400" from a Sonos device_description.xml.
bool extractRinconId(const std::string& deviceDescriptionXml, std::string& out);

}  // namespace discplayer
