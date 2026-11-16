#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>

SoftwareSerial dfSerial(10, 11); // RX, TX
DFRobotDFPlayerMini myDFPlayer;

const int PIR_PIN = 2;
const int KUGEL_PIN = 5;  // PWM -> 1k -> Transistor-Basis (Lichterkette)
const int AUGE_PIN = 6;   // PWM -> Vorwiderstand -> Einzel-LED
const int NUM_TRACKS = 7; // Vorgabe, falls die Dateizahl der SD-Karte nicht gelesen werden kann
int anzahlTracks = NUM_TRACKS;
const int LAUTSTAERKE = 25; // 0-30

// Helligkeit (PWM 0-255): falls zu hell/dunkel, hier anpassen
const int KUGEL_RUHE_MIN = 10;
const int KUGEL_RUHE_MAX = 40;
const int KUGEL_AKTIV_MIN = 120;
const int KUGEL_AKTIV_MAX = 255;
const int AUGE_RUHE_MIN = 0;
const int AUGE_RUHE_MAX = 20;
const int AUGE_AKTIV_MIN = 100;
const int AUGE_AKTIV_MAX = 255;

const unsigned long PIR_WARMUP_MS = 40000;
const unsigned long AKTIV_MAX_MS = 20000; // Sicherheits-Timeout, falls "Track fertig" nicht kommt
const unsigned long SPERRE_MS = 4000;
const unsigned long FLACKER_TICK_MS = 15;
const unsigned long HEARTBEAT_MS = 1000; // Statuszeile im Serial Monitor

bool dfBereit = false;
int letzterPir = LOW;
unsigned long letzterHeartbeat = 0;

enum Status { WARMUP, RUHE, AKTIV, SPERRE };
Status status = WARMUP;
unsigned long statusStart = 0;

struct Flacker {
  int pin;
  float helligkeit;
  int ziel;
  unsigned long naechsterWechsel;
};
Flacker kugel = {KUGEL_PIN, 0, 0, 0};
Flacker auge = {AUGE_PIN, 0, 0, 0};
unsigned long letzterTick = 0;

void flackerUpdate(Flacker &f, int minHell, int maxHell, float glaettung) {
  unsigned long jetzt = millis();
  if (jetzt >= f.naechsterWechsel) {
    f.ziel = random(minHell, maxHell + 1);
    f.naechsterWechsel = jetzt + random(40, 160);
  }
  f.helligkeit += (f.ziel - f.helligkeit) * glaettung;
  analogWrite(f.pin, (int)f.helligkeit);
}

const char *statusName(Status st) {
  switch (st) {
    case WARMUP: return "WARMUP";
    case RUHE:   return "RUHE";
    case AKTIV:  return "AKTIV";
    case SPERRE: return "SPERRE";
  }
  return "?";
}

void setStatus(Status neu) {
  Serial.print(F("Status: "));
  Serial.print(statusName(status));
  Serial.print(F(" -> "));
  Serial.println(statusName(neu));
  status = neu;
  statusStart = millis();
}

void lichtTest() {
  Serial.println(F("Lichttest: Kugel (D5) 1,5 s voll an..."));
  analogWrite(KUGEL_PIN, 255);
  delay(1500);
  analogWrite(KUGEL_PIN, 0);
  Serial.println(F("Lichttest: Einzel-LED (D6) 1,5 s voll an..."));
  analogWrite(AUGE_PIN, 255);
  delay(1500);
  analogWrite(AUGE_PIN, 0);
  Serial.println(F("Lichttest fertig."));
}

void setup() {
  pinMode(PIR_PIN, INPUT);
  pinMode(KUGEL_PIN, OUTPUT);
  pinMode(AUGE_PIN, OUTPUT);
  analogWrite(KUGEL_PIN, 0);
  analogWrite(AUGE_PIN, 0);

  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);
  delay(2000); // Monitor Zeit geben, sich nach dem Reset zu verbinden
  Serial.println(F("=== Setup gestartet ==="));

  lichtTest();

  Serial.println(F("Starte SoftwareSerial..."));
  dfSerial.begin(9600);
  Serial.println(F("Initialisiere DFPlayer..."));
  if (myDFPlayer.begin(dfSerial)) {
    dfBereit = true;
    myDFPlayer.volume(LAUTSTAERKE);
    int dateien = myDFPlayer.readFileCounts();
    Serial.print(F("DFPlayer bereit. Dateien auf SD-Karte: "));
    Serial.println(dateien);
    if (dateien > 0) {
      anzahlTracks = dateien;
    } else {
      Serial.print(F("Dateizahl nicht lesbar, nehme Vorgabe: "));
      Serial.println(anzahlTracks);
    }
  } else {
    Serial.println(F("FEHLER: DFPlayer nicht gefunden! Verkabelung (D10/D11, VCC, GND) und SD-Karte prüfen. Licht und PIR laufen trotzdem."));
  }

  randomSeed(analogRead(A0));
  setStatus(WARMUP);
  Serial.println(F("PIR wärmt auf (40 s), Licht schon im Ruhe-Flackern..."));
}

void loop() {
  unsigned long jetzt = millis();

  int pir = digitalRead(PIR_PIN);
  if (pir != letzterPir) {
    letzterPir = pir;
    Serial.print(F("PIR-Pin D2 -> "));
    Serial.println(pir == HIGH ? F("HIGH (Bewegung)") : F("LOW"));
  }

  if (jetzt - letzterHeartbeat >= HEARTBEAT_MS) {
    letzterHeartbeat = jetzt;
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN)); // blinkt, solange loop() läuft
    Serial.print(F("[")); Serial.print(jetzt / 1000); Serial.print(F("s] "));
    Serial.print(statusName(status));
    Serial.print(F(" PIR=")); Serial.print(pir);
    Serial.print(F(" Kugel=")); Serial.print((int)kugel.helligkeit);
    Serial.print(F(" Auge=")); Serial.println((int)auge.helligkeit);
  }

  if (jetzt - letzterTick >= FLACKER_TICK_MS) {
    letzterTick = jetzt;
    if (status == AKTIV) {
      flackerUpdate(kugel, KUGEL_AKTIV_MIN, KUGEL_AKTIV_MAX, 0.35);
      flackerUpdate(auge, AUGE_AKTIV_MIN, AUGE_AKTIV_MAX, 0.35);
    } else {
      flackerUpdate(kugel, KUGEL_RUHE_MIN, KUGEL_RUHE_MAX, 0.08);
      flackerUpdate(auge, AUGE_RUHE_MIN, AUGE_RUHE_MAX, 0.08);
    }
  }

  switch (status) {
    case WARMUP:
      if (jetzt - statusStart >= PIR_WARMUP_MS) {
        Serial.println(F("PIR bereit."));
        setStatus(RUHE);
      }
      break;

    case RUHE:
      if (pir == HIGH) {
        int track = random(1, anzahlTracks + 1);
        Serial.print(F(">>> Bewegung! Spiele Track: "));
        Serial.println(track);
        if (dfBereit) {
          myDFPlayer.play(track);
        } else {
          Serial.println(F("(kein Sound: DFPlayer nicht bereit)"));
        }
        setStatus(AKTIV);
      }
      break;

    case AKTIV:
      if (jetzt - statusStart >= AKTIV_MAX_MS) {
        Serial.println(F("Timeout, beende Aktiv-Phase."));
        setStatus(SPERRE);
      }
      break;

    case SPERRE:
      if (jetzt - statusStart >= SPERRE_MS) {
        setStatus(RUHE);
      }
      break;
  }

  if (dfBereit && myDFPlayer.available()) {
    uint8_t type = myDFPlayer.readType();
    int value = myDFPlayer.read();
    if (type == DFPlayerPlayFinished && status == AKTIV) {
      Serial.print(F("Track fertig: "));
      Serial.println(value);
      setStatus(SPERRE);
    } else {
      Serial.print(F("[DFPlayer] Meldung Typ "));
      Serial.print(type);
      Serial.print(F(", Wert "));
      Serial.println(value);
    }
  }
}
