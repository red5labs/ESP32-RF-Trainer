# ESP32 WROOM 433MHz RF Transmitter with Web Config

Arduino sketch for **ESP32 WROOM** that transmits configurable data over a **433MHz RF module**, with a **web interface** on the ESP32’s WiFi AP so you can control it from a phone. Uses **software OOK** (no RadioHead); only built-in libraries are required.

## Features

- **Configurable payload**: Set the text (up to 60 characters) sent over 433MHz.
- **Burst duration**: How long each transmission burst lasts (seconds).
- **Burst interval**: Delay between bursts (milliseconds).
- **Start/Stop**: Start or stop transmission from the web UI.

## Hardware

- **ESP32 WROOM** dev board (e.g. Freenove ESP32 WROOM, DOIT ESP32 DEVKIT V1, NodeMCU-32S).
- **433MHz transmitter** (e.g. 4-pin green PCB with trimmer). Pins are usually VCC, GND, Data; the 4th may be antenna. See [WIRING.md](WIRING.md) if your module’s order differs.

See **[WIRING.md](WIRING.md)** for exact pin connections and notes on the MX-RM-5V receiver.

## Arduino IDE Setup

1. **Board**
   - Install **ESP32** support (Board Manager → “esp32” by Espressif).
   - Select **ESP32 Dev Module** (or the board that matches yours, e.g. Freenove ESP32 WROOM).

2. **Libraries**
   - Only built-in libraries are used (WiFi, WebServer, Preferences). No extra libraries to install.

3. **Open sketch**
   - Open `ESP32_Xmitter/ESP32_Xmitter.ino` in Arduino IDE.

4. **Upload**
   - Connect the ESP32 via USB, select the correct port, then **Upload**. If it fails, hold **BOOT** while clicking Upload.

## Usage

1. Power the ESP32 (USB or external).
2. Connect your phone to WiFi:
   - **SSID:** `ESP32-Xmitter`
   - **Password:** `xmitter1`
3. In the browser, open: **http://192.168.4.1**
4. Set **Payload** (text to send, max 60 chars), **Burst duration** (seconds), and **Interval between bursts** (ms).
5. Click **Save settings**, then **Start transmission** to begin; **Stop transmission** to end.

## Pin Summary (quick reference)

| ESP32 WROOM | 433MHz transmitter |
|-------------|--------------------|
| **GPIO 5**  | Data               |
| **3.3V**    | VCC                |
| **GND**     | GND                |

Full details: [WIRING.md](WIRING.md).

