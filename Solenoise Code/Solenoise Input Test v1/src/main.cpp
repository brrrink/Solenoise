//----------------------ENCODER OLED TEST-------------------------//
//---------------Encoder and OLED Test --------------------//
// Teensy 4 Pins:
// - OLED display on I2C address 0x3C
// - For Teensy 4: SDA on pin 18, SCL on pin 19
// - Encoder pins: Enc-L (A) on pin 16 (A2), Enc-R (B) on pin 17 (A3)
// - Encoder switch on pin 20 (A6)

// - CV input 0-5V is on pin 14(A0)
// - Gate input 0-5V is on pin 15 (A1)
// - Transistor outputs T1 to T12 are on pins 2 - 13

// - MIDI TRS input to be added on available pins: 20 is TX5, 21 is RX5

//----------------------INCLUDES----------------------------//
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Encoder.h>
//---------------------------------------------------------//

//-----------------------CONSTANTS-------------------------//
#define OLED_RESET 4       // OLED reset pin (not used on Teensy, but for compatibility)
Adafruit_SSD1306 display(OLED_RESET);

#define SCREEN_WIDTH 128 // OLED width: 128 pixels (before rotation)
#define SCREEN_HEIGHT 32 // OLED height: 32 pixels (before rotation)

#if (SSD1306_LCDHEIGHT != 32)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

#define ENCODER_SWITCH_PIN 20
#define CV_PIN 14
#define GATE_PIN 15
#define PPR 24  // Pulses per revolution for Bourns PEC11R
#define CPR (PPR * 4)  // Counts per revolution (x4 quadrature)
//---------------------------------------------------------//

//-----------------------VARIABLES-------------------------//
Encoder enc(16, 17);  // Encoder on pins 16 and 17

bool switchState = false;  // Toggle state for switch
int lastButtonState = HIGH;  // Previous state of the button (pullup, HIGH = not pressed)
int cv_value = 0; // Current CV input ADC value
int gate_state = HIGH; // Current Gate input state (using int for digitalRead)
//---------------------------------------------------------//

void setup() {
  Serial.begin(9600);  // Start serial for debugging
  Wire.begin();        // Initialize I2C for OLED

  // Initialize OLED with I2C address 0x3C
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setRotation(0);  // Rotate display, 0 is horizontal
  display.clearDisplay();
  display.setTextSize(1);  // Set text size
  display.setTextColor(WHITE);

  // Setup encoder switch pin with pullup
  pinMode(ENCODER_SWITCH_PIN, INPUT_PULLUP);

  // Setup gate input pin with internal pullup
  pinMode(GATE_PIN, INPUT_PULLUP);
}

void loop() {
  display.clearDisplay(); // Clear the OLED

  // Read encoder position (negated to invert direction)
  long position = -enc.read();
  // Compute count (0 to 23)
  long detent_position = position / 4;
  int count = ((detent_position % 24) + 24) % 24;

  // Display encoder count
  display.setCursor(0, 0);
  display.print("Encoder: ");
  display.println(count);

  // Read switch and detect press (falling edge)
  int buttonState = digitalRead(ENCODER_SWITCH_PIN);
  if (buttonState == LOW && lastButtonState == HIGH) {
    switchState = !switchState;  // Toggle state
    delay(50);  // Simple debounce
  }
  lastButtonState = buttonState;

  // Draw graphical box for switch state in upper right (at x=118, y=0, 10x10 pixels)
  if (switchState) {
    display.fillRect(118, 0, 10, 10, WHITE);  // Filled white
  } else {
    display.drawRect(118, 0, 10, 10, WHITE);  // Border white (implicit black fill)
  }

  // Read CV and Gate
  cv_value = analogRead(CV_PIN);
  gate_state = digitalRead(GATE_PIN);

  // Display CV value below encoder
  display.setCursor(0, 16);
  display.print("CV: ");
  display.println(cv_value);

  // Draw graphical box for gate state to the right (at x=118, y=16, 10x10 pixels)
  // Filled when gate is low (active low), outline when high
  if (gate_state == LOW) {
    display.fillRect(118, 16, 10, 10, WHITE);  // Filled white
  } else {
    display.drawRect(118, 16, 10, 10, WHITE);  // Border white (implicit black fill)
  }

  // Update display
  display.display();

  // Small delay for loop
  delay(10);
}