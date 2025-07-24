# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build and Development Commands

This is a PlatformIO project for STM32L476RG. Use these commands:

```bash
# Build the project
pio run

# Upload to board
pio run --target upload

# Monitor serial output
pio device monitor

# Clean build
pio run --target clean

# Build and upload in one command
pio run --target upload --target monitor
```

## System Architecture

This is an industrial cooling system with IoT capabilities built around STM32L476RG microcontroller.

### Core System Flow
The system operates through a main loop in `src/main.cpp` that orchestrates:
1. **Sensor Management**: Virtual sensor array (4 sensors from 1 physical DHT11) via SensorManager
2. **Control Logic**: Automatic temperature-based cooling control via SystemControl  
3. **Network Communication**: MQTT-based IoT connectivity via connect module
4. **Data Persistence**: SD card logging and CSV export via logs module
5. **Hardware Interrupts**: Emergency/mode buttons via InterruptManager

### Key Design Patterns
- **Modular Library Structure**: Each major function isolated in `/lib/` directories
- **Interrupt-Driven Events**: Hardware buttons trigger immediate system responses
- **State Machine Control**: SystemControl manages MANUAL/AUTOMATIC modes with emergency override
- **Virtual Sensor Simulation**: Single DHT11 creates 4 virtual sensors with offsets for redundancy
- **Dual Logging**: Separate debug logs and structured CSV data export

### Critical System States
- **Emergency Mode**: Hardware interrupt immediately disables all actuators
- **Network Connectivity**: System operates locally when WiFi/MQTT unavailable
- **Sensor Health**: Individual sensor failure detection with system-wide averaging
- **Temperature Thresholds**: Configurable min/max setpoints with critical limits

### MQTT Communication Structure
- **Publishing**: `sala_maquinas/sensor[1-4]/temperatura|humidade` 
- **Control Subscriptions**: `sala_maquinas/config/*` and `sala_maquinas/controlo/*`
- **Remote Commands**: Temperature setpoints, mode changes, emergency control

### Hardware Configuration
Key pin assignments in `lib/config/config.hpp`:
- DHT11 sensor on PC1
- ESP8266 UART on PA10/PA9  
- SD card SPI on PA5-PA7/PB6
- Emergency button on PA0, Mode button on PA1
- Status LEDs on PA8-PA11, PB0-PB1

### Data Flow
1. **Sensor Reading**: 2-second intervals → SensorManager → virtual sensor array
2. **Control Processing**: Temperature averaging → SystemControl → actuator decisions  
3. **Network Publishing**: 5-second MQTT intervals → remote monitoring
4. **Data Logging**: 1-minute CSV saves → persistent storage on SD card

### Development Notes
- System designed for 24/7 autonomous operation
- All timing uses millis() with overflow handling
- RTC provides accurate timestamps for logs and data
- WiFi/MQTT failures handled gracefully with local operation
- Virtual sensors provide fault tolerance and spatial distribution simulation