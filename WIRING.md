# ESP32 WROOM to 433MHz RF Transmitter – Pin Connections

Connections for an **ESP32 WROOM dev board** (e.g. Freenove ESP32 WROOM, DOIT ESP32 DEVKIT V1, NodeMCU-32S) and a **433MHz transmitter module** (e.g. 4-pin green PCB with trimmer).

## Wiring: ESP32 WROOM ↔ 433MHz Transmitter

| ESP32 WROOM | 433MHz transmitter |
|-------------|--------------------|
| **GPIO 5**  | **Data**           |
| **3.3V**    | **VCC**            |
| **GND**     | **GND**            |

- **GPIO 5**: On a Freenove ESP32 WROOM board, **5** is on the **bottom** header row, between pins **17** and **18**. Connect this to the transmitter’s **Data** (or DIN/ATAD) pin.
- **3.3V** and **GND**: Use any **3.3V** and **GND** on the board’s headers.

If your transmitter has **unlabelled** pins, the order is often **VCC, GND, Data** (4th pin may be antenna). Try that order; if it doesn’t work, swap the wire on Data to the adjacent pin.

**5V modules**: If your transmitter is 5V-only, use the board’s **5V** pin instead of 3.3V (only if your board exposes 5V and the module is rated for it).

## Antenna

- Add a **~17 cm** wire to the transmitter’s antenna pad (or 4th pin if it’s ANT) for better range at 433 MHz.

## MX-RM-5V receiver (IMG_9524)

The **MX-RM-5V** is a 433MHz **receiver** (GND, GND, VCC on the back). This sketch only **transmits**. To receive the data you’d use a second ESP32 (or another MCU) with the receiver and matching receive/decoding software.

## Sketch pin (quick reference)

- **RF_TX_PIN** = **5** → connect to transmitter **Data** pin. No other GPIOs are used for RF.
