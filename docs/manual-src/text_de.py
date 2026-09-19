# -*- coding: utf-8 -*-
DE = {}
def t(k, v): DE[k] = v

t('title_sub', 'SPATIAL INTELLIGENCE')
t('title_sub2', 'SMART IMAGING')
t('manual', 'Handbuch')
t('cover_meta', 'Version 1.0 - VST3 - macOS / Windows')

t('h_what', 'Wofür das hier gut ist')
t('p_what1',
  "SpaceX ist kein Korrekturwerkzeug. Es ist ein Weg, einem Signal eine eigene Ebene zu geben - "
  "in einem Mix, der eigentlich schon voll ist.")
t('p_what2',
  "Die meisten Stereo-Plugins sind darauf gebaut, sicher zu sein. Dieses hier ist darauf gebaut, "
  "interessant zu sein. Es schiebt Dinge zur Seite, nach vorne, nach hinten und ein Stück aus der "
  "Phase - also genau dorthin, wo die meisten nicht hingehen: entweder, weil es ihnen nie jemand "
  "gezeigt hat, oder weil sie Angst davor haben, was das mit Mono macht. Bewusst eingesetzt liegt "
  "genau da der Platz.")
t('p_what2b',
  "Das kommt aus der Arbeit. Ich mische seit 2019 professionell als Produzent, Mix- und "
  "Mastering-Engineer, für Platten und fürs Fernsehen, unter anderem The Voice. Daneben habe ich "
  "für andere Engineers - Jeff Ellis und Mosty zum Beispiel - deren Recording- und Mixing-Templates "
  "neu gebaut und für andere Systeme optimiert. In der Session von jemand anderem zu sitzen ist der "
  "schnellste Weg zu sehen, wo die Entscheidungen wirklich liegen.")
t('p_what3',
  "Zwei Begriffe aus Jeff Ellis' Mixer-Brain-Kurs sind hängen geblieben, weil sie etwas benannt "
  "haben, das ich seit Jahren mache, ohne ein Wort dafür zu haben: die Forbidden Zone und die "
  "vierte Dimension. Wenn der Mix dicht ist und jede Frequenz vergeben, bleiben nur noch Breite und "
  "Tiefe. SpaceX ist für diesen Raum gebaut.")
t('p_what4',
  "Verstehe es als Effekt-Erzeuger, nicht als Mastering-Werkzeug. Es nimmt etwas Gewöhnliches - "
  "einen Hall-Return, einen Delay-Throw, einen Stapel Backings - und macht daraus etwas, dem man "
  "zuhört.")

t('h_instead', 'Was es ersetzt')
t('p_instead',
  "Für alles hier unten braucht man sonst vier bis fünf Plugins und rund 500 Euro:")
t('instead', [
  ['Center-Extraktion (L / C / R)', 'ein eigenes Center-Plugin'],
  ['Haas-Delay und Micro-Pitch', 'ein Pitch-/Delay-Werkzeug'],
  ['Polarity, Side Gain, Side Width', 'ein Utility-Plugin - meistens ein hässliches'],
  ['Mono-Check und Korrelation', 'ein Metering-Plugin'],
  ['Distanz und Klangneigung', 'ein EQ'],
  ['Auto-Pan und Stereobewegung', 'ein Panner'],
])
t('p_instead2',
  "Ein Fenster, eine Latenz, ein Preset. Das ist der halbe Sinn der Sache.")

t('h_where', 'Wohin damit')
t('p_where1',
  "Am besten auf alles, was ohnehin schon wet ist: Backings, Ad-libs, ein Stapel Harmonien, ein "
  "Hall- oder Delay-Return. Throws gehören genauso dazu - das sind meist einzelne Spuren und keine "
  "Busse. Trockene Quellen sind nicht verboten: eine Lead-Stimme, die gedoppelt werden soll, "
  "funktioniert, Drums auch. Die Faustregel ist einfach - je mehr Raumeffekte vor SpaceX liegen, "
  "desto weiter kannst du gehen.")
t('p_where2',
  "Die Position in der Kette ist Geschmackssache. Ganz hinten ist der Normalfall: Du formst, was "
  "aus der Kette herauskommt. Ganz vorne funktioniert auch, wenn das Signal schon in Hall oder "
  "Echo ertrinkt - dann formst du den Effekt selbst statt der Quelle.")
t('p_where3',
  "Throw-Spuren sind der einfachste Gewinn. Die sind per Definition 100 % wet, niemand erwartet "
  "davon Mono-Kompatibilität, und ein bisschen Timewarp plus Vision macht aus einem Delay einen "
  "Hook.")
t('p_where4',
  "Es ist kein Mastering-Werkzeug. Pack es nicht auf den Mixbus und erwarte, dass es sich benimmt.")

t('h_quick', 'In einer Minute zum Ergebnis')
t('quick', [
  "Auf einen Backing-Bus oder eine Throw-Spur legen.",
  "Oben eine Smart-Kategorie anklicken - Backings, Vocals, Pads, was eben passt.",
  "Den Smart-Knopf drücken. Hören. Nochmal drücken. So lange, bis dich etwas überrascht.",
  "Zu viel? Mix auf 70 %. Immer noch zu viel? Eine Sektion ausschalten.",
  "Unten auf Mono prüfen. Fällt es zusammen, Dimension Size etwas zurück.",
])
t('p_quick',
  "Das ist der ganze Ablauf. Die Regler darunter sind für den Moment, in dem du schon weißt, was "
  "du willst.")

t('h_smart', 'Die Smart-Engine')
t('p_smart1',
  "Die beiden Knöpfe oben bauen dir eine neue Einstellung. Der linke lässt jede Sektion spielen, "
  "der rechte entscheidet zusätzlich, welche Sektionen überhaupt dazugehören - das gibt meistens "
  "das aufgeräumtere Ergebnis.")
t('p_smart2',
  "Die sechs Kategorien sagen ihm, woran er arbeitet. Einmal ausgewählt übernimmt die Kategorie "
  "die Führung - ab da reicht ein Knopf.")
t('p_smart3',
  "Drücken, hören, nochmal drücken. So lange, bis dich etwas überrascht - und das wird es.")
t('p_smart4',
  "Mach dir keine Sorgen, was das mit deiner Spur anstellt. Das ist nicht der Zufallsknopf, den "
  "andere Plugins haben. Alles, was er erreichen kann, ist nach Gehör eingestellt, und was nicht "
  "funktioniert hat, ist gar nicht erst drin - du bekommst also nichts, das du rückgängig machen "
  "musst.")
t('smart', [
  ['Schloss', 'diese Sektion lassen Smart und Breathe in Ruhe'],
  ['Breathe', 'schaltet in jeder Sektion die Modulation an, mit neuen Zufallstiefen'],
  ['Undo / Redo', 'gilt für alles, auch für Smart'],
  ['A / B', 'zwei Versionen nebeneinander; der Pfeil kopiert die eine in die andere'],
])

t('h_layout', 'Der Signalweg')
t('p_layout',
  "Von links nach rechts, von oben nach unten. Polarity kann an vier Stellen dieser Kette sitzen - "
  "dafür sind die Knöpfe 1-4 da.")
t('chain', ['IN', 'GALAXY', 'POLARITY', 'TIMEWARP', 'DIMENSION', 'HYPERDRIVE', 'VISION', 'RAYE', 'MIX', 'OUT'])
t('p_chain_pol',
  "Die Latenz ist null, solange Galaxy nicht scharf ist. Galaxy braucht ein Analysefenster, um die "
  "Mitte herauszulösen - das kostet etwas Verzögerung. Alles andere läuft in Echtzeit.")

t('p_chain_filter',
  "Der Focus-Bereich liegt quer zur Kette, nicht darin. Galaxy, Dimension und Vision arbeiten nur "
  "innerhalb seines Bereichs - ganz offen macht er gar nichts. Das kleine Focus-Symbol in den "
  "Köpfen von Galaxy und Dimension nimmt die jeweilige Sektion wieder heraus.")

t('h_common', 'In jeder Sektion')
t('common', [
  ['S', 'Solo - nur diese Sektion hören'],
  ['Schloss', 'Smart lässt diese Sektion in Ruhe'],
  ['Sektionsname', 'anklicken schaltet die Sektion an oder aus'],
  ['Wellen-Icon', 'Modulation an / aus für diese Sektion'],
  ['Kleiner Regler', 'Modulationstiefe'],
])

t('h_galaxy', 'GALAXY')
t('p_galaxy',
  "Löst die Mitte aus dem Stereobild heraus und behandelt Mitte und Seiten getrennt. Das ist der "
  "Teil, der Latenz kostet - und der Teil, der alles dahinter anders klingen lässt.")
t('galaxy', [
  ['Gravity', 'wie stark die Mitte von den Seiten getrennt wird'],
  ['Orbit', 'unten nur die Mitte, oben nur die Seiten, Mitte ist das Original'],
])
t('tip_galaxy',
  "Auf einem Backing-Stapel Orbit nach oben. Die Lead bleibt, wo sie ist, und die Backings gehen "
  "drumherum auf, ohne lauter zu werden.")

t('h_pol', 'POLARITY')
t('p_pol',
  "Dreht die Phase eines Kanals. Der drastischste Eingriff im Plugin - und der mit dem größten "
  "Unterschied.")
t('pol', [
  ['L / R', 'linken oder rechten Kanal drehen'],
  ['Link', 'beide gleichzeitig'],
  ['1 - 4', 'an welcher Stelle der Kette gedreht wird'],
])
t('warn_pol',
  "Ein Flip zerstört fast immer die Mono-Kompatibilität. Auf einem Throw oder einem Effekt-Return "
  "ist das egal - die summiert niemand nach Mono und erwartet, dass sie überleben. Bei allem, was "
  "den Song trägt: vorher Mono prüfen.")
t('p_pol2',
  "Die vier Positionen sind wichtiger, als sie aussehen. Vor Galaxy gedreht ändert sich, was "
  "Galaxy für die Mitte hält. Nach Vision gedreht betrifft es nur noch das fertige Bild. Probier "
  "1 und 3 bei derselben Einstellung - das sind zwei verschiedene Sounds.")

t('h_tw', 'TIMEWARP')
t('p_tw',
  "Winzige Unterschiede zwischen links und rechts, in der Zeit und in der Tonhöhe. Der klassische "
  "Breiten-Trick - und der Grund, warum man dafür bisher zwei Plugins brauchte.")
t('tw', [
  ['Drift', 'verzögert eine Seite um wenige Millisekunden (Haas). Links oder rechts'],
  ['Shift', 'Micro-Pitch-Versatz zwischen den Kanälen, in Cent'],
  ['Balance', 'gleicht die Lautheitsverschiebung aus, die Drift verursacht'],
])

t('h_dim', 'DIMENSION')
t('p_dim',
  "Klassische Mid/Side-Breite. Die Sektion, zu der du greifst, wenn das Ergebnis fast stimmt und "
  "nur noch breiter oder schmaler werden soll.")
t('dim', [
  ['Size', 'Stereobreite. Unter 100 % schmaler, über 100 % breiter'],
  ['Boost', 'hebt den Pegel der Seiten an'],
])
t('tip_dim',
  "Size über etwa 150 % zusammen mit Drift ist die Stelle, an der Mono zu leiden beginnt. Das "
  "Plugin begrenzt die schlimmsten Kombinationen schon selbst, entscheiden tun deine Ohren.")

t('h_hyp', 'HYPERDRIVE')
t('p_hyp',
  "Langsame automatische Bewegung durch das Stereofeld. Kein Tremolo - der Pegel bleibt, nur die "
  "Position wandert.")
t('hyp', [
  ['Flow', 'wie weit der Klang nach links und rechts wandert'],
  ['Pulse', 'weichere, pulsartige Bewegung statt einer Sinuswelle'],
  ['Speed', 'Bewegungsrate in Hz'],
  ['Sync', 'Rate ans Host-Tempo koppeln'],
  ['Bars', 'die Notenlänge, solange Sync an ist'],
])

t('h_vis', 'VISION')
t('p_vis',
  "Wo das fertige Bild sitzt und wie weit weg es ist. Die Sektion, die einen EQ-Griff ersetzt, den "
  "du sonst von Hand machen würdest.")
t('vis', [
  ['Tilt', 'verschiebt das ganze Bild nach links oder rechts'],
  ['Width', 'endgültige Breite des Bildes'],
  ['Distance', 'weiter weg heißt weichere Höhen und etwas weniger Pegel'],
  ['Elevate', 'sanfte Klangneigung. Oben heller, unten dunkler'],
])

t('h_raye', 'RAYE')
t('p_raye',
  "Ein weicher Phaser, der das Stereobild bewegt statt den Klang. Absichtlich subtil - es ist "
  "Bewegung, kein Effekt, den man benennen können sollte.")
t('raye', [
  ['Strength', 'klicken schaltet durch Light, Medium, Strong und Off'],
  ['Speed', 'wie schnell die Bewegung durchläuft'],
  ['Pair', 'folgt Hyperdrive mit halber Geschwindigkeit'],
])

t('h_footer', 'Ausgang')
t('footer', [
  ['IN / OUT', 'Eingangs- und Ausgangspegel'],
  ['Mono', 'das Ergebnis in Mono hören'],
  ['Dry', 'in Mono mit dem unbearbeiteten Eingang vergleichen'],
  ['Mix', 'bearbeitetes Signal mit dem Original mischen'],
  ['Vol', 'Ausgangstrimm, plus/minus 6 dB'],
  ['Korrelation', 'rechts der Mitte ist mono-sicher, links davon löscht sich in Mono aus'],
])

t('h_prism', 'Frequenzband')
t('p_prism',
  "Der Bereich, in dem SpaceX arbeitet. Hier wird nichts aus dem Klang herausgenommen - ausserhalb "
  "laeuft das Signal unveraendert durch. Kante ziehen zum Aendern, Mitte ziehen zum Verschieben, "
  "hoch/runter oder scrollen macht ihn breiter.")
t('prism', [
  ['Power', 'Band an / aus'],
  ['Band', 'der Bereich, in dem Dimension und Vision arbeiten dürfen'],
])
t('tip_prism',
  "Untere Kante auf etwa 200 Hz, und der Bassbereich bleibt mono und fest, während alles darüber "
  "aufgeht. Das ist die nützlichste Einstellung im ganzen Plugin.")

t('h_star', 'Das Sternenfeld')
t('p_star1',
  "Das Bild links ist nicht nur Deko. Jedes Objekt reagiert darauf, was das Plugin gerade tut - "
  "der Planet steigt mit Gravity, die Sternlinien neigen sich mit Tilt, die goldenen Linien sind "
  "RAYE.")
t('p_star2',
  "Klick ins Feld schaltet die Spur an und aus. Klick auf Sonne oder Mond wechselt zwischen voller "
  "Ansicht und ruhiger Messansicht. Das Zahnrad öffnet die Darstellungseinstellungen.")
t('star', [
  ['Goniometer', 'das klassische Vektorskop - senkrechte Linie ist mono, Kreis ist breit'],
  ['Ansicht', 'Space zeigt alles, Stars ist die ruhige Messansicht'],
  ['Speed / Density', 'wie schnell und wie belebt das Feld ist'],
  ['Stars', 'die funkelnden Sterne abschalten'],
])

t('h_look', 'Themes und Layout')
t('p_look',
  "Im Menü liegen sieben Themes und drei Layouts. Layout ändert, wie viel Rahmenwerk die Sektionen "
  "bekommen: 3D mit Kasten und Füllung, Flat ohne beides, Outline mit dünnem Rahmen ohne Füllung. "
  "Am Klang ändert das nichts.")

t('h_recipes', 'Fünf Einstellungen, die funktionieren')
t('recipes', [
  ['Backing-Stapel, der nicht reinpasst',
   "Kategorie Backings, dann Smart. Dann Orbit hoch, Dimension Size auf etwa 120 %, "
   "Frequenzband ab 200 Hz. Die Lead behält die Mitte, die Backings nehmen sich alles drumherum."],
  ['Delay-Throw mit eigenem Charakter',
   "Timewarp Drift auf 8-12 ms, Shift um 6 Cent, RAYE auf Light. Der Throw klingt nicht mehr nach "
   "Delay, sondern nach Entscheidung."],
  ['Hall-Return mit Tiefe',
   "Vision Distance hoch, Elevate etwas runter, Dimension Size auf etwa 130 %. Der Hall wandert "
   "hinter das trockene Signal statt darüber."],
  ['Ad-libs, die kommen und gehen',
   "Hyperdrive Flow auf etwa 40 %, Sync an, 4 Takte. Langsam genug, dass niemand die Bewegung "
   "bemerkt, schnell genug, dass die Spur nie stillsteht."],
  ['Die verbotene',
   "Polarity L an, Position 1, Mix auf etwa 60 %. Mono überlebt das nicht. Auf einem Throw im "
   "dichten Refrain ist es das Breiteste, was du hast."],
])

t('h_short', 'Klicks, die man kennen sollte')
t('short', [
  ['Doppelklick auf einen Regler', 'zurück auf Standard'],
  ['Cmd / Strg + ziehen', 'Feinjustierung'],
  ['Klick auf den Sektionsnamen', 'Sektion an / aus'],
  ['Klick aufs Logo', 'Bypass'],
  ['? unten links', 'zeigt, worüber die Maus gerade steht'],
])

t('h_thanks', 'Dank')
t('p_thanks1',
  "SpaceX ist entstanden, weil ich aufhören wollte, für eine Idee fünf Plugins zu öffnen. Nichts "
  "daran ist zufällig passiert: die Reihenfolge der Kette, die vier Polarity-Positionen, wie weit "
  "jede Sektion gehen darf - das sind alles Entscheidungen, die durch Hören entstanden sind und "
  "danach noch einmal geprüft wurden.")
t('p_thanks2',
  "Dank an Jeff Ellis, dessen Begriffe für die Forbidden Zone und die vierte Dimension der Idee "
  "einen Namen gegeben haben.")
t('p_thanks3',
  "Paul Misty - Mistycatstudios &middot; info@paulmisty.com &middot; paulmisty.com &middot; Germany")
