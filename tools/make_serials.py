#!/usr/bin/env python3
"""Erzeugt gueltige SpaceX-Seriennummern.

    python3 tools/make_serials.py 20            -> 20 zufaellige Nummern
    python3 tools/make_serials.py 1 --name "Jeff Ellis"
                                                -> eine Nummer, aus dem Namen
                                                   abgeleitet (gleicher Name =
                                                   gleiche Nummer). Das Plugin
                                                   prueft bei der Aktivierung,
                                                   ob Name und Nummer zusammen-
                                                   passen, und schreibt den
                                                   Namen dann aufs Back Panel.

Das Verfahren muss identisch zu Source/Licence.h bleiben.
"""
import sys, secrets

ALPHABET = "23456789ABCDEFGHJKLMNPQRSTUVWXYZ"
SALT     = "SpaceX/PaulMisty/v1:"

def fnv1a64(s: str) -> int:
    h = 1469598103934665603
    for b in s.encode("utf-8"):
        h ^= b
        h = (h * 1099511628211) & 0xFFFFFFFFFFFFFFFF
    return h

def checksum(payload: str) -> str:
    h = fnv1a64(SALT + payload)
    return "".join(ALPHABET[(h >> (i * 5)) & 31] for i in range(4))

def serial(payload: str) -> str:
    assert len(payload) == 8 and all(c in ALPHABET for c in payload)
    full = "SPX1" + payload + checksum(payload)
    return "-".join(full[i:i+4] for i in range(0, 16, 4))

def payload_random() -> str:
    return "".join(secrets.choice(ALPHABET) for _ in range(8))

def name_key(name: str) -> str:
    """Nur a-z und 0-9 - identisch zu spacex::nameKey() in Source/Licence.h.
    Dadurch ist es egal, wie der Kunde seinen Namen spaeter eintippt."""
    return "".join(c for c in name.lower() if c.isascii() and c.isalnum())

def payload_from_name(name: str) -> str:
    """Muss Zeichen fuer Zeichen zu spacex::payloadFromName() passen."""
    h = fnv1a64(SALT + "name:" + name_key(name))
    return "".join(ALPHABET[(h >> (i * 5)) & 31] for i in range(8))

if __name__ == "__main__":
    args = sys.argv[1:]
    n = int(args[0]) if args and args[0].isdigit() else 1
    name = None
    if "--name" in args:
        name = args[args.index("--name") + 1]
    for _ in range(n):
        print(serial(payload_from_name(name) if name else payload_random()))
