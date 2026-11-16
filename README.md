# Madame Rosa – Halloween-Wahrsagerin mit Sound und Bewegungsmelder

Umbau einer gekauften Halloween-Wahrsagerin (nur Licht, 3× AAA): Mit **Arduino Nano**, **DFPlayer Mini** und **PIR-Bewegungsmelder** flackert die Kristallkugel ruhig vor sich hin. Kommt jemand vorbei, wird das Licht hell und die Figur spricht einen zufälligen Spruch von der SD-Karte.

Video-Anleitung (Deutsch): *Link folgt*

## Funktion
1. Start: 40 s Aufwärmzeit für den PIR, das Licht flackert schon ruhig.
2. Bewegung erkannt: zufällige MP3 von der SD-Karte, Licht wird hell und flackert stark.
3. Nach dem Sound 4 s Sperre, dann wieder bereit.

## Bauteile
| Teil | Zweck |
|---|---|
| Arduino Nano V3 (CH340), optional Schraubklemmen-Board | Steuerung |
| DFPlayer Mini + microSD-Karte (FAT32/FAT16) | spielt die MP3s |
| PIR HC-SR501 | Bewegungsmelder |
| Lautsprecher 3 W / 4 Ω, ca. 40 mm | Ton |
| NPN-Transistor BC337 oder 2N2222 | schaltet die Lichterkette |
| Widerstände: 2× 1 kΩ, 47 Ω, 330 Ω | Schutz und Helligkeit |
| 2 WAGO-Steckklemmen (4- und 5-fach) | sammeln 5 V und GND |
| Steckkabel (Dupont) | DFPlayer und PIR ohne Löten |
| USB-Netzteil 5 V / 1 A | Strom (keine Powerbank, die schaltet bei wenig Strom ab) |

## Verdrahtung
Die Original-LEDs der Figur: **gelb = Plus, weiß = Minus**. Batteriefach, Schalter und Try-me-Modul werden entfernt. Alle 5-V-Leitungen laufen in eine WAGO-Klemme, alle GND-Leitungen in die andere.

| Bauteil | Anschluss |
|---|---|
| PIR HC-SR501 | VCC → 5V, GND → GND, OUT → **D2** |
| DFPlayer | VCC → 5V, GND → GND, TX → **D10**, RX → 1 kΩ → **D11**, SPK_1/SPK_2 → Lautsprecher |
| Lichterkette (10 LEDs) | gelb → 47 Ω → 5V, weiß → Kollektor BC337 |
| BC337 | Basis → 1 kΩ → **D5**, Emitter → GND |
| Einzel-LED (oben im Bogen) | gelb → 330 Ω → **D6**, weiß → GND |

PIR-Einstellung: Delay-Poti auf Minimum, Jumper auf H (Dauerauslösung), Empfindlichkeit nach Bedarf. Bilder: [`docs/`](docs).

Tipp: BC337 (TO-92) mit der flachen Seite zu dir und den Beinen nach unten: von links C – B – E.

## SD-Karte
- Mit dem **SD Card Formatter** der SD Association formatieren, nicht mit Windows (sonst spielt der DFPlayer nur kurz Rauschen).
- MP3s im Hauptverzeichnis, lückenlos nummeriert: `0001.mp3`, `0002.mp3`, …
- Gut funktioniert: 44,1 kHz, 128 kbit/s CBR, ohne Tags, kurze Clips (3–6 s).
- **Die Sprüche aus dem Video sind nicht enthalten.** Nimm eigene oder lizenzfreie Sounds.

## Software
1. Arduino IDE, Bibliothek **DFRobotDFPlayerMini** installieren.
2. `wahrsagerin/wahrsagerin.ino` öffnen (Board: Arduino Nano, Prozessor ATmega328P, bei Upload-Problemen „Old Bootloader“ probieren).
3. Die Anzahl der Dateien liest der Sketch selbst von der SD-Karte.
4. Helligkeit und Zeiten oben im Sketch (`KUGEL_*`, `AUGE_*`, `PIR_WARMUP_MS`, `SPERRE_MS`). Ist es zu hell: Widerstand erhöhen, zu dunkel: 47 Ω → 33 Ω.

## 3D-Druck (Sockel)
Die Figur steht auf einem kleinen Sockel, in dem Nano, DFPlayer, Lautsprecher und PIR sitzen (so musste ich nichts in die Figur bohren).
- `stl/rahmen_30mm.stl`: Rahmen 135 × 130 mm, 30 mm hoch, 3 mm Wand, PIR-Fenster und Schallöcher vorne
- `stl/deckel.stl`: Deckel 155 × 150 mm mit Steck-Lippe
- `stl/make_sockel.py`: erzeugt beide Teile (Python, benötigt numpy, shapely, trimesh, manifold3d, mapbox_earcut)

**Ehrlicher Hinweis:** Bei mir war der Deckel an der Kante zu niedrig geplant, und der Steg über dem PIR-Fenster war zu schmal. Ich habe dort ein Stück nachgefräst, und für das USB-Kabel einen Schlitz. Ihr könnt die Maße oben in `make_sockel.py` anpassen.

## Hinweise
- Nachbau auf eigene Gefahr. Das Projekt läuft nur mit 5 V (USB), nichts davon an Netzspannung anschließen.
- Der Nano darf nicht mit den 3 AAA betrieben werden (zu wenig Spannung). Ein Nano aus der Charge kann defekt sein: Wenn Upload oder Serial-Ausgabe spinnen, einen anderen testen.

## Lizenz
MIT, siehe [LICENSE](LICENSE).
