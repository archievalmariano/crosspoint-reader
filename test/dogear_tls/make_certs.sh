#!/usr/bin/env bash
# Generates the DOGEAR TLS test certificates into OUT_DIR. CMake runs it when the
# test is configured, so no key is committed: each run makes new keys, and they
# exist only in the build tree. Needs OpenSSL 3.4+ for -not_before / -not_after.
#   make_certs.sh OUT_DIR
#   root.pem          trusted test root, ECDSA P-384 (as GTS Root R4 is)
#   good.pem/.key     dogear.test, signed by the root, valid 2020-2060
#   wronghost.pem     other.test, signed by the root
#   expired.pem       dogear.test, signed by the root, expired in 2021
#   selfsigned.pem    dogear.test, signed by itself
set -euo pipefail
[ $# -eq 1 ] || { echo "usage: make_certs.sh OUT_DIR" >&2; exit 2; }
mkdir -p "$1"
cd "$1"
rm -f ./*.pem ./*.key ./*.srl
openssl ecparam -name secp384r1 -genkey -noout -out root.key
openssl req -x509 -new -key root.key -sha384 -subj "/CN=DOGEAR Test Root" \
  -not_before 20200101000000Z -not_after 20600101000000Z \
  -addext "basicConstraints=critical,CA:TRUE" -addext "keyUsage=critical,keyCertSign,cRLSign" -out root.pem
leaf() {  # name cn not_before not_after signer(root|self)
  openssl ecparam -name prime256v1 -genkey -noout -out "$1.key"
  openssl req -new -key "$1.key" -subj "/CN=$2" -out "$1.csr"
  ext=$(mktemp); printf "subjectAltName=DNS:%s\nbasicConstraints=CA:FALSE\nextendedKeyUsage=serverAuth\n" "$2" > "$ext"
  if [ "$5" = self ]; then
    openssl x509 -req -in "$1.csr" -key "$1.key" -sha256 -not_before "$3" -not_after "$4" -extfile "$ext" -out "$1.pem"
  else
    openssl x509 -req -in "$1.csr" -CA root.pem -CAkey root.key -CAcreateserial -sha384 \
      -not_before "$3" -not_after "$4" -extfile "$ext" -out "$1.pem"
  fi
  rm -f "$1.csr" "$ext"
}
leaf good dogear.test 20200101000000Z 20600101000000Z root
leaf wronghost other.test 20200101000000Z 20600101000000Z root
leaf expired dogear.test 20200101000000Z 20210101000000Z root
leaf selfsigned dogear.test 20200101000000Z 20600101000000Z self
rm -f root.key ./*.srl
