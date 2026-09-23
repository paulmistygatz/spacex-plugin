#pragma once
#include <JuceHeader.h>

// ===== SERIENNUMMER =====
//
// Bewusst offline und bewusst einfach. Eine Seriennummer ist KEIN echter
// Kopierschutz - wer sie umgehen will, schafft das. Sie hält den ehrlichen
// Nutzer davon ab, das Plugin einfach weiterzureichen, und genau das ist der
// Anteil, der bei Plugins dieser Preisklasse den Umsatz macht. Alles andere
// (iLok, Online-Aktivierung) kostet Geld, nervt Käufer und lohnt sich hier
// nicht.
//
// Format:
//     SPX1-XXXX-XXXX-CCCC
//
// "SPX1" ist ein festes Präfix (die 1 ist die Schema-Version, falls je ein
// zweites Format nötig wird). Die beiden mittleren Gruppen sind die frei
// wählbare Nutzlast, die letzte eine Prüfsumme aus Nutzlast + geheimem Salz.
// Das Alphabet lässt 0/O/1/I weg, damit beim Abtippen aus einer Mail nichts
// schiefgeht; Bindestriche, Leerzeichen und Kleinschreibung werden beim
// Prüfen ohnehin ignoriert.
//
// tools/make_serials.py erzeugt gültige Nummern mit demselben Verfahren -
// wird hier etwas geändert, muss es dort mitgeändert werden.
namespace spacex
{
    inline const char* serialAlphabet() { return "23456789ABCDEFGHJKLMNPQRSTUVWXYZ"; }   // 32 Zeichen

    // Wird dieses Salz getauscht, werden alle bisher ausgegebenen Nummern
    // ungültig. Also: nicht anfassen, sobald die erste verkauft ist.
    inline const char* serialSalt() { return "SpaceX/PaulMisty/v1:"; }

    inline juce::uint64 serialHash (const juce::String& payload)
    {
        // FNV-1a, 64 Bit. Kein Kryptoanspruch - es geht nur darum, dass man
        // sich eine gültige Nummer nicht ausdenken kann.
        juce::uint64 h = 1469598103934665603ULL;
        const juce::String salted = juce::String (serialSalt()) + payload;
        for (const char* p = salted.toRawUTF8(); *p != 0; ++p)
        {
            h ^= (juce::uint64) (unsigned char) *p;
            h *= 1099511628211ULL;
        }
        return h;
    }

    // Alles ausser A-Z und 0-9 fliegt raus, Rest in Grossbuchstaben.
    inline juce::String normaliseSerial (const juce::String& in)
    {
        juce::String out;
        for (int i = 0; i < in.length(); ++i)
        {
            const auto c = juce::CharacterFunctions::toUpperCase (in[i]);
            if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))
                out += juce::String::charToString (c);
        }
        return out;
    }

    inline juce::String checksumFor (const juce::String& payload8)
    {
        const juce::uint64 h = serialHash (payload8);
        const juce::String alpha (serialAlphabet());
        juce::String cs;
        for (int i = 0; i < 4; ++i)
            cs += juce::String::charToString (alpha[(int) ((h >> (i * 5)) & 31ULL)]);
        return cs;
    }

    // ===== NAME <-> SERIENNUMMER =====
    // tools/make_serials.py leitet die Nutzlast aus dem Namen ab. Das ist eine
    // Einbahnstrasse - aus der Nummer laesst sich der Name NICHT zurueck-
    // rechnen. Man kann ihn aber PRUEFEN: wer bei der Aktivierung seinen Namen
    // eintippt, bekommt genau dann "Registered to <Name>", wenn Name und Nummer
    // zusammengehoeren. Damit steht auf dem Back Panel nie ein erfundener Name.
    //
    // Der Namensschluessel ist bewusst hart normalisiert: nur a-z und 0-9,
    // alles andere faellt weg. "Jeff Ellis", "jeff ellis" und "Jeff  Ellis"
    // ergeben denselben Schluessel - der Kunde muss also nicht auf
    // Gross-/Kleinschreibung oder Leerzeichen achten. Umlaute und Akzente
    // fallen auf beiden Seiten gleichermassen heraus (Python: dieselbe Regel
    // in make_serials.py).
    inline juce::String nameKey (const juce::String& name)
    {
        const auto lower = name.toLowerCase();
        juce::String out;
        for (int i = 0; i < lower.length(); ++i)
        {
            const auto c = lower[i];
            if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))
                out += juce::String::charToString (c);
        }
        return out;
    }

    inline juce::String payloadFromName (const juce::String& name)
    {
        // Dieselbe FNV-1a-Funktion wie fuer die Pruefsumme, nur mit eigenem
        // Praefix, damit Nutzlast und Pruefsumme nicht aus demselben Wert
        // fallen. 8 Zeichen a 5 Bit = 40 der 64 Bit.
        const juce::uint64 h = serialHash ("name:" + nameKey (name));
        const juce::String alpha (serialAlphabet());
        juce::String out;
        for (int i = 0; i < 8; ++i)
            out += juce::String::charToString (alpha[(int) ((h >> (i * 5)) & 31ULL)]);
        return out;
    }

    inline bool isValidSerial (const juce::String& raw)
    {
        const auto s = normaliseSerial (raw);
        if (s.length() != 16 || ! s.startsWith ("SPX1"))
            return false;
        const auto payload = s.substring (4, 12);
        const juce::String alpha (serialAlphabet());
        for (int i = 0; i < payload.length(); ++i)
            if (! alpha.containsChar (payload[i]))
                return false;
        return s.substring (12, 16) == checksumFor (payload);
    }

    // Gehoert diese Nummer zu diesem Namen? (Nur sinnvoll fuer Nummern, die
    // mit --name erzeugt wurden; zufaellige Nummern passen zu keinem Namen.)
    inline bool serialMatchesName (const juce::String& rawSerial, const juce::String& name)
    {
        const auto s = normaliseSerial (rawSerial);
        if (s.length() != 16 || nameKey (name).isEmpty())
            return false;
        return s.substring (4, 12) == payloadFromName (name);
    }

    // Zur Anzeige: SPX1-XXXX-XXXX-CCCC
    inline juce::String formatSerial (const juce::String& raw)
    {
        const auto s = normaliseSerial (raw);
        if (s.length() != 16)
            return raw.toUpperCase();
        return s.substring (0, 4) + "-" + s.substring (4, 8) + "-"
             + s.substring (8, 12) + "-" + s.substring (12, 16);
    }
}
