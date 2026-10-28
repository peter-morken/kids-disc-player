#!/usr/bin/env bash
# Regenerates firmware/src/github_root_cas.h from the system's Mozilla root
# store (Debian/Ubuntu: the ca-certificates package).
set -euo pipefail

CERT_DIR=${CERT_DIR:-/etc/ssl/certs}
OUT=$(dirname "$0")/../firmware/src/github_root_cas.h
ROOTS=(
  USERTrust_ECC_Certification_Authority
  USERTrust_RSA_Certification_Authority
  Sectigo_Public_Server_Authentication_Root_E46
  Sectigo_Public_Server_Authentication_Root_R46
  DigiCert_Global_Root_G2
  DigiCert_Global_Root_G3
  ISRG_Root_X1
  ISRG_Root_X2
  GTS_Root_R1
  GTS_Root_R4
  Microsoft_RSA_Root_Certificate_Authority_2017
  Microsoft_ECC_Root_Certificate_Authority_2017
)

{
  echo '#pragma once'
  echo '// Root CAs trusted for firmware downloads from GitHub Releases.'
  echo '// github.com and its release-asset CDN currently chain to Sectigo/USERTrust'
  echo '// or DigiCert. The others are there so a change of CA by GitHub does not cut'
  echo '// a device in the field off from updates. Source: Mozilla root store'
  echo '// (Debian ca-certificates). Regenerate with tools/gen_root_cas.sh.'
  echo
  echo 'static const char kGithubRootCAs[] ='
  for f in "${ROOTS[@]}"; do
    echo "    // $f"
    openssl x509 -in "$CERT_DIR/$f.pem" | sed 's/^/    "/; s/$/\\n"/'
  done
  echo '    ;'
} > "$OUT"
echo "Wrote $OUT"
