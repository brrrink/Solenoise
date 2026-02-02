//----------------------ENCODER OLED MIDI TEST-------------------------//
//---------------Encoder, OLED, and MIDI Input Test --------------------//
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

const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

#define MIDI_NOTE_START 36  // C2
#define MIDI_NOTE_END 47    // B2
#define OUTPUT_PIN_START 2  // T1 to T12 on pins 2-13

#define PULSE_DURATION 30 // Hold HIGH for XXms (adjust as needed)
#define DISPLAY_UPDATE_INTERVAL 50  // Update OLED every XXms
#define SWITCH_DEBOUNCE_MS 50  // Debounce time for switch

//---------------------------------------------------------//

//-----------------------VARIABLES-------------------------//
Encoder enc(16, 17);

bool switchState = false;
int lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;

int cv_value = 0;
int gate_state = HIGH;
int last_gate_state = HIGH;  // For edge detection
int last_note = -1;
int last_vel = 0;

unsigned long lastDisplayUpdate = 0;
bool displayNeedsUpdate = true;  // Flag for changes

unsigned long pulseStartTimes[12] = {0};  // Timers for each output (0 = inactive)

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

  LPUART5_CTRL &= ~(1UL << 19);  // Disable TX
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
      displayNeedsUpdate = true;  // Flag for display

      if (note >= MIDI_NOTE_START && note <= MIDI_NOTE_END && isOn) {
        int index = note - MIDI_NOTE_START;
        int pin = OUTPUT_PIN_START + index;
        digitalWrite(pin, HIGH);
        pulseStartTimes[index] = millis();  // Start timer
      }
      // NoteOff ignored for pulsing; timer handles LOW
    }
  }
}

void checkPulseTimers() {
  unsigned long now = millis();
  for (int i = 0; i < 12; i++) {
    if (pulseStartTimes[i] != 0 && now - pulseStartTimes[i] >= PULSE_DURATION) {
      digitalWrite(OUTPUT_PIN_START + i, LOW);
      pulseStartTimes[i] = 0;  // Reset timer
      displayNeedsUpdate = true;
    }
  }
}

void readInputs() {
  long position = -enc.read();
  long detent_position = position / 4;
  int count = ((detent_position % 24) + 24) % 24;

  // Encoder display (only flag update if changed; for simplicity, always flag here)
  // You could add change detection for count if needed

  int reading = digitalRead(ENCODER_SWITCH_PIN);
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > SWITCH_DEBOUNCE_MS) {
    if (reading == LOW) {
      switchState = !switchState;
      displayNeedsUpdate = true;
    }
  }
  lastButtonState = reading;

  cv_value = analogRead(CV_PIN);
  gate_state = digitalRead(GATE_PIN);

  // Detect falling edge on gate (active low trigger)
  if (last_gate_state == HIGH && gate_state == LOW) {
    // Map CV value (0-1023) to output index 0-11
    int index = (cv_value * 12) / 1024;
    int pin = OUTPUT_PIN_START + index;
    digitalWrite(pin, HIGH);
    pulseStartTimes[index] = millis();
    displayNeedsUpdate = true;
  }
  last_gate_state = gate_state;

  // Flag update if CV/gate change significantly (optional optimization)
}

void updateDisplay() {
  display.clearDisplay();

  // Encoder
  display.setCursor(0, 0);
  display.print("Encoder: ");
  long position = -enc.read();
  long detent_position = position / 4;
  int count = ((detent_position % 24) + 24) % 24;
  display.println(count);

  if (switchState) {
    display.fillRect(118, 0, 8, 8, WHITE);
  } else {
    display.drawRect(118, 0, 8, 8, WHITE);
  }

  // CV/Gate
  display.setCursor(0, 8);
  display.print("CV: ");
  display.println(cv_value);

  if (gate_state == LOW) {
    display.fillRect(118, 8, 8, 8, WHITE);
  } else {
    display.drawRect(118, 8, 8, 8, WHITE);
  }

  // MIDI
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

  // Output boxes at lower part
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
  handleMIDI();  // Handle MIDI first for low latency
  checkPulseTimers();  // Check pulses non-blocking
  readInputs();  // Read other inputs

  unsigned long now = millis();
  if (displayNeedsUpdate || (now - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL)) {
    updateDisplay();
  }

  // Optional: Call handleMIDI() again at end for even tighter polling
  handleMIDI();
}