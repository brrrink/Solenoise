//---------------------- Solenoise Firmware Testing v9 -------------------------//
//----------------------------- John Harding -----------------------------------//
//---------------------------- Steal This Code ---------------------------------//


// Teensy 4 Pins:
// - OLED display on I2C address 0x3C
// - For Teensy 4: SDA on pin 18, SCL on pin 19
// - Encoder pins: Enc-L (A) on pin 16 (A2), Enc-R (B) on pin 17 (A3)
// - Encoder switch on pin 22
// - MIDI RX on pin 21 (Serial 5)
// - CV input 0-5V is on pin 14(A0)
// - Gate input 0-5V is on pin 15 (A1)
// - Transistor outputs T1 to T12 are on pins 2 - 13

// - MIDI TRS input using TLP2361 opto-isolator connected to pin 21 (RX5 for Serial5)
//   (TX5 on pin 20 is unused)
// - On the PCB Silkscreen, Pin 4 (bottom left-hand side of pcb) is the Midi input pin, Pin: 5 is the GND pin.
// - For TRS MIDI Type A (Tip: Pin 5, Ring: Pin 4, Sleeve: GND)

// - Testing:
// - Testing CV/Gate mapping strategies
// - Added 400 uS delay to the gate reading to allow analog CV input to settle
// - Testing non-blocking routine with micros(); instead of delayMicroSeconds();
// - Testing encoder to alter gate settle and on time for solenoids

// - Adding variable (INVERT_ENCODER) for the encoder direction (invert CW/CCW rotation)
// - Moving GUI output indicators lower
// - Altering the switch behaviour to avoid repeated triggers

// Teensy 4 Pins:
// - OLED display on I2C address 0x3C
// - For Teensy 4: SDA on pin 18, SCL on pin 19
// - Encoder pins: Enc-L (A) on pin 16 (A2), Enc-R (B) on pin 17 (A3)
// - Encoder switch on pin 22
// - MIDI RX on pin 21 (Serial 5)
// - CV input 0-5V is on pin 14(A0)
// - Gate input 0-5V is on pin 15 (A1)
// - Transistor outputs T1 to T12 on pins 2 - 13

//----------------------INCLUDES----------------------------//
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Encoder.h>
#include <MIDI.h>
//---------------------------------------------------------//

//-----------------------CONSTANTS-------------------------//
#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32

#if (SSD1306_LCDHEIGHT != 32)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

#define ENCODER_SWITCH_PIN 22
#define CV_PIN 14
#define GATE_PIN 15
#define PPR 24
#define CPR (PPR * 4)

const bool INVERT_ENCODER = true;

const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

#define MIDI_NOTE_START 36
#define MIDI_NOTE_END 47
#define OUTPUT_PIN_START 2

#define DISPLAY_UPDATE_INTERVAL 50
#define SWITCH_DEBOUNCE_MS 50

#define SETTLE_MIN_US 50
#define SETTLE_MAX_US 500
#define PULSE_MIN_MS 20
#define PULSE_MAX_MS 50

//---------------------------------------------------------//

//-----------------------VARIABLES-------------------------//
Encoder enc(16, 17);

bool switchState = false;
int lastButtonState = HIGH;
int debouncedButtonState = HIGH;
unsigned long lastDebounceTime = 0;

int cv_value = 0;
int gate_state = HIGH;
int last_gate_state = HIGH;

int last_note = -1;
int last_vel = 0;

unsigned long lastDisplayUpdate = 0;
bool displayNeedsUpdate = true;

unsigned long pulseStartTimes[12] = {0};

int settleDelayUS = 200;
int pulseDuration = 30;

int editMode = 0;            // 0 = normal, 1 = settle, 2 = pulse
long lastEncoderPosition = 0;

//---------------------------------------------------------//

MIDI_CREATE_INSTANCE(HardwareSerial, Serial5, midiIn);
//---------------------------------------------------------//

void setup() {
  Serial.begin(9600);
  Wire.begin();

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setRotation(0);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  pinMode(ENCODER_SWITCH_PIN, INPUT_PULLUP);
  pinMode(GATE_PIN, INPUT_PULLUP);

  for (int pin = OUTPUT_PIN_START; pin < OUTPUT_PIN_START + 12; pin++) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
  }

  Serial5.begin(31250);
  midiIn.begin(MIDI_CHANNEL_OMNI);
  midiIn.turnThruOff();

  LPUART5_CTRL &= ~(1UL << 19);

  lastEncoderPosition = enc.read();
}

void handleMIDI() {
  while (midiIn.read()) {
    byte type = midiIn.getType();
    int note = midiIn.getData1();
    int vel = midiIn.getData2();

    if (type == midi::NoteOn || type == midi::NoteOff) {
      bool isOn = (type == midi::NoteOn && vel > 0);
      if (!isOn) vel = 0;

      last_note = note;
      last_vel = vel;
      displayNeedsUpdate = true;

      if (note >= MIDI_NOTE_START && note <= MIDI_NOTE_END && isOn) {
        int index = note - MIDI_NOTE_START;
        digitalWrite(OUTPUT_PIN_START + index, HIGH);
        pulseStartTimes[index] = millis();
      }
    }
  }
}

void checkPulseTimers() {
  unsigned long now = millis();
  for (int i = 0; i < 12; i++) {
    if (pulseStartTimes[i] != 0 && now - pulseStartTimes[i] >= pulseDuration) {
      digitalWrite(OUTPUT_PIN_START + i, LOW);
      pulseStartTimes[i] = 0;
      displayNeedsUpdate = true;
    }
  }
}

void handleCVGate() {
  gate_state = digitalRead(GATE_PIN);

  if (last_gate_state == HIGH && gate_state == LOW) {
    unsigned long startWait = micros();
    while (micros() - startWait < settleDelayUS) {
      handleMIDI();
    }

    cv_value = analogRead(CV_PIN);

    int index = (cv_value * 12) / 1024;

    digitalWrite(OUTPUT_PIN_START + index, HIGH);
    pulseStartTimes[index] = millis();

    displayNeedsUpdate = true;
  }

  last_gate_state = gate_state;
}

void readInputs() {
  long raw_position = enc.read();
  long position = INVERT_ENCODER ? -raw_position : raw_position;

  // Only apply changes when actively rotating in edit mode
  if (editMode != 0) {
    long delta = position - lastEncoderPosition;
    if (delta != 0) {
      if (editMode == 1) {
        settleDelayUS = constrain(settleDelayUS + delta * 10, SETTLE_MIN_US, SETTLE_MAX_US);
      } else if (editMode == 2) {
        pulseDuration = constrain(pulseDuration + delta, PULSE_MIN_MS, PULSE_MAX_MS);
      }
      displayNeedsUpdate = true;
      lastEncoderPosition = position;
    }
  } else {
    lastEncoderPosition = position;
  }

  // Button handling
  int reading = digitalRead(ENCODER_SWITCH_PIN);
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > SWITCH_DEBOUNCE_MS) {
    if (reading != debouncedButtonState) {
      debouncedButtonState = reading;
      if (debouncedButtonState == LOW) {
        editMode = (editMode + 1) % 3;
        displayNeedsUpdate = true;
        // Do NOT update lastEncoderPosition here - prevents jump on mode change
      }
    }
  }
  lastButtonState = reading;

  handleCVGate();
}

void updateDisplay() {
  display.clearDisplay();

  // Base text for top line
  display.setCursor(0, 0);
  display.setTextColor(WHITE);
  display.print("On:");
  display.print(pulseDuration);
  display.print("ms Del:");
  display.print(settleDelayUS);
  display.print("us");

  // Clean inversion for edited parameter
  if (editMode == 1) {  // Settle Delay
    // Clear area first to prevent ghosting
    display.fillRect(60, 0, 68, 8, WHITE);
    display.setTextColor(BLACK);
    display.setCursor(60, 0);
    display.print("Del:");
    display.print(settleDelayUS);
    display.print("us");
    display.setTextColor(WHITE);
  } 
  else if (editMode == 2) {  // Pulse On time
    display.fillRect(0, 0, 55, 8, WHITE);
    display.setTextColor(BLACK);
    display.setCursor(0, 0);
    display.print("On:");
    display.print(pulseDuration);
    display.print("ms");
    display.setTextColor(WHITE);
  }

  // CV line
  display.setCursor(0, 8);
  display.print("CV: ");
  float voltage = (cv_value * 5.0f) / 1024.0f;
  display.print(voltage, 2);
  display.print("V -> ");
  int step = (cv_value * 12) / 1024;
  display.print(step + 1);

  if (gate_state == LOW) {
    display.fillRect(118, 8, 8, 8, WHITE);
  } else {
    display.drawRect(118, 8, 8, 8, WHITE);
  }

  // MIDI line
  display.setCursor(0, 16);
  display.print("Note: ");
  if (last_note != -1) {
    display.print(last_note);
    int note_mod = last_note % 12;
    int octave = (last_note / 12) - 1;
    display.print(" (");
    display.print(noteNames[note_mod]);
    display.print(octave);
    display.print(")");
  } else {
    display.print("None");
  }

  if (last_vel > 0) {
    display.fillRect(118, 16, 8, 8, WHITE);
  } else {
    display.drawRect(118, 16, 8, 8, WHITE);
  }

  // Outputs
  for (int i = 0; i < 12; i++) {
    int x = i * 10;
    if (pulseStartTimes[i] != 0) {
      display.fillRect(x, 24, 8, 8, WHITE);
    } else {
      display.drawRect(x, 24, 8, 8, WHITE);
    }
  }

  display.display();
  displayNeedsUpdate = false;
  lastDisplayUpdate = millis();
}

void loop() {
  handleMIDI();
  checkPulseTimers();
  readInputs();

  unsigned long now = millis();
  if (displayNeedsUpdate || (now - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL)) {
    updateDisplay();
  }

  handleMIDI();
}