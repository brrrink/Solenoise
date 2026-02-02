

# Solenoise

## Overview

![solenoise](/images/Solenoise.png)

Solenoise is an open-source hardware and software system designed to transform everyday objects—such as metal plates, wooden surfaces, or architectural elements—into programmable percussive instruments. By leveraging solenoids as electromagnetic actuators, Solenoise enables automated performance, enhancing musical expressivity and extending human capabilities in experimental music, interactive installations, and architectural sound design.

The resurgence of analog modular synthesis, coupled with advancements in physical computing, has inspired this project. Traditional percussive instruments are limited by their fixed physical properties; Solenoise addresses this by providing a versatile control interface that bridges digital and physical realms.

## Key Features

- **Multi-Modal Input Control**: Supports class-compliant MIDI over TRS Mini-Jack, CV/gate inputs with internal logic for simplified and seamless integration with midi capable devices, modular synthesisers, and sequencers.
- **Hardware Simplicity**: Built around a Teensy 4.0 microcontroller with driverless MIDI input and simplified input protection circuits, ensuring compatibility with external devices.
- **Efficient Actuation**: Uses logic-level MOSFETs and protective transistors for safe and efficient operation of inductive loads (solenoids).
- **Configurable Software**: A flexible framework optimises latency and expressivity, with a focus on CV/Trigger input processing along with class compliant MIDI input.
- **Open-Source Ethos**: Unlike proprietary systems (e.g., Polyend Perc, Dada Machines Automat), Solenoise aims to democratise solenoid augmentation and aims to further collaborative innovation.

## Hardware Requirements

- **Microcontroller**: Teensy 4.0 (required for high-speed processing and MIDI support).
- **Solenoids**: Compatible 12V DC solenoids (e.g., push-pull types for striking objects).
- **Power Supply**: 12VDC Powers the development and the solenoids, with internal voltage protection and rectification.

### Input/Output

- **MIDI**: class-compliant MIDI (TRS Type A) via 3.175mm (1/8 inch) 'Mini Jack' input.
- **CV/Gate**: Analog inputs with protection circuits.

## Installation

- **Clone the repository**: git clone https://github.com/brrrink/Solenoise/solenoise.git
- cd solenoise
- Use Platformio to upload firmware to Teensy 4.0, hardware is available to purchase with pre-mounted SMD components or see the Solenoise Production Files folder, for all production ready design files.
- Upload these design files to the PCB manufacturer of your choice, who should be able to provide full soldered SMD PCB's from these files.
- A small number of through-hole components have to be added, these have been minimised to the degree possible to facilitate ease of construction.

## Usage

### Basic Setup

Attach solenoids to objects (e.g., via 3D-printed mounts—in progress enhancement).

Connect inputs:
- MIDI: Plug into a DAW or MIDI controller.
- Gates: Trigger via digital signals from sequencers.
- CV/Trigger: Use modular synth outputs for voltage-based control.

### Example: MIDI-Controlled Percussion

Send MIDI notes to Solenoise via Mini Jack (Type A).

### CV/Gate Integration

CV input controls the allocation of solenoid (12 in total)

Gate input will trigger discrete strikes, supplemented with MIDI for analog and moblie (battery powered) workflows.

### Performance Evaluation

Latency: <5ms for MIDI/CV processing (tested on Teensy 4.0).

Applications: Experimental music (e.g., hybrid human-robot performances), interactive art (e.g., responsive installations), sound design (e.g., architectural acoustics).

## Comparisons to Existing Systems

Solenoise builds on commercial and academic precedents but emphasises accessibility:

- **Commercial**: QRS PNOmation (piano retrofits), Polyend Perc (discontinued drumming machine), Dada Machines Automat (modular but costly), Koma Elektronik Field Kit (electroacoustic with solenoid control), Archil Lab Poet Pilot (object actuation).
- **Academic**: Logos Foundation <Vibi> (robotic vibraphone), Ajay Kapur's MahaDeviBot (percussion robot), Z-Machines guitar-bot.
- **Unique Aspects**: Open-source, CV/Gate inputs extend MIDI, driverless and mobile focussed design. 

## Future Enhancements

- 3D-printed solenoid mounts for easy attachment.
- Wireless control (e.g., via ESP32 integration).
- Expanded inputs/outputs for larger setups.
- Community-contributed presets and mappings.

## Contributing

Contributions are welcome! Fork the repo, create a branch, and submit a pull request. Focus areas:
- Bug fixes in firmware.
- New hardware variants (e.g., Eurorack compatibility).
- Documentation improvements.

Please follow the Code of Conduct.

## License

All non commercial use is approved.

Steal this code.

Commercial use is not approved without consent, contact the authour for more details: j.harding@ulster.ac.uk

