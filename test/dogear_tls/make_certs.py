"""Generate the DOGEAR TLS test certificates into OUT_DIR.

CMake runs this when the test is configured, so no key is committed: each run
makes new keys, and they exist only in the build tree. The validity dates are
fixed, so the outcome does not depend on the day the tests run (until 2060).
Needs the hash-pinned `cryptography` in requirements.txt (no system OpenSSL).

    python3 make_certs.py OUT_DIR

    root.pem          trusted test root, ECDSA P-384 (as GTS Root R4 is)
    good.pem/.key     dogear.test, signed by the root, valid 2020-2060
    wronghost.pem     other.test, signed by the root
    expired.pem       dogear.test, signed by the root, expired in 2021
    selfsigned.pem    dogear.test, signed by itself
"""
import datetime as dt
import sys
from pathlib import Path

from cryptography import x509
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import ec
from cryptography.x509.oid import ExtendedKeyUsageOID, NameOID

UTC = dt.timezone.utc
VALID = (dt.datetime(2020, 1, 1, tzinfo=UTC), dt.datetime(2060, 1, 1, tzinfo=UTC))
EXPIRED = (dt.datetime(2020, 1, 1, tzinfo=UTC), dt.datetime(2021, 1, 1, tzinfo=UTC))


def name(cn: str) -> x509.Name:
    return x509.Name([x509.NameAttribute(NameOID.COMMON_NAME, cn)])


def write_key(out: Path, stem: str, key) -> None:
    (out / f"{stem}.key").write_bytes(key.private_bytes(
        serialization.Encoding.PEM, serialization.PrivateFormat.TraditionalOpenSSL, serialization.NoEncryption()))


def write_cert(out: Path, stem: str, cert: x509.Certificate) -> None:
    (out / f"{stem}.pem").write_bytes(cert.public_bytes(serialization.Encoding.PEM))


def root(out: Path):
    key = ec.generate_private_key(ec.SECP384R1())
    ski = x509.SubjectKeyIdentifier.from_public_key(key.public_key())
    cert = (x509.CertificateBuilder()
            .subject_name(name("DOGEAR Test Root")).issuer_name(name("DOGEAR Test Root"))
            .public_key(key.public_key()).serial_number(x509.random_serial_number())
            .not_valid_before(VALID[0]).not_valid_after(VALID[1])
            .add_extension(x509.BasicConstraints(ca=True, path_length=None), critical=True)
            .add_extension(x509.KeyUsage(digital_signature=False, content_commitment=False, key_encipherment=False,
                                         data_encipherment=False, key_agreement=False, key_cert_sign=True,
                                         crl_sign=True, encipher_only=False, decipher_only=False), critical=True)
            .add_extension(ski, critical=False)
            .add_extension(x509.AuthorityKeyIdentifier.from_issuer_subject_key_identifier(ski), critical=False)
            .sign(key, hashes.SHA384()))
    write_cert(out, "root", cert)
    return key, cert  # the root key is never written


def leaf(out: Path, stem: str, cn: str, validity, signer) -> None:
    key = ec.generate_private_key(ec.SECP256R1())
    builder = (x509.CertificateBuilder()
               .subject_name(name(cn))
               .public_key(key.public_key()).serial_number(x509.random_serial_number())
               .not_valid_before(validity[0]).not_valid_after(validity[1])
               .add_extension(x509.SubjectAlternativeName([x509.DNSName(cn)]), critical=False)
               .add_extension(x509.BasicConstraints(ca=False, path_length=None), critical=False)
               .add_extension(x509.ExtendedKeyUsage([ExtendedKeyUsageOID.SERVER_AUTH]), critical=False)
               .add_extension(x509.SubjectKeyIdentifier.from_public_key(key.public_key()), critical=False))
    if signer is None:  # self-signed
        cert = builder.issuer_name(name(cn)).sign(key, hashes.SHA256())
    else:
        ca_key, ca_cert = signer
        cert = (builder.issuer_name(ca_cert.subject)
                .add_extension(x509.AuthorityKeyIdentifier.from_issuer_public_key(ca_key.public_key()), critical=False)
                .sign(ca_key, hashes.SHA384()))
    write_key(out, stem, key)
    write_cert(out, stem, cert)


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: make_certs.py OUT_DIR", file=sys.stderr)
        return 2
    out = Path(sys.argv[1])
    out.mkdir(parents=True, exist_ok=True)
    for old in list(out.glob("*.pem")) + list(out.glob("*.key")):
        old.unlink()
    ca = root(out)
    leaf(out, "good", "dogear.test", VALID, ca)
    leaf(out, "wronghost", "other.test", VALID, ca)
    leaf(out, "expired", "dogear.test", EXPIRED, ca)
    leaf(out, "selfsigned", "dogear.test", VALID, None)
    return 0


if __name__ == "__main__":
    sys.exit(main())
