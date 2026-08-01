# ESP32 Projects 
Workspace 
This repository contains a set of ESP-IDF experiments, prototypes, and example projects for Espressif SoCs. The projects range from UART and GPIO interrupt examples to Wi-Fi/Ethernet networking experiments and a small Python TCP helper server. 
## Overview 
The workspace is organized as a collection of independent ESP-IDF projects. Each project is self-contained and can be built and flashed independently using standard ESP-IDF tooling. 
## Repository Structure 
| Path | Description |
 | --- | --- | 
| Esp55ProjectRISCV/ | Collection of ESP32-family example projects. | 
| Esp55ProjectRISCV/MovementSensor/ | GPIO interrupt example that uses a FreeRTOS task and ISR to measure movement timing on GPIO4. | 
| Esp55ProjectRISCV/UartProject/ | UART echo example that reads bytes from a configured UART and sends them back to the sender. | 
| Esp55ProjectRISCV/WifiTest/wifiTest/ | Wi-Fi station example that connects to a fixed SSID/password and sends a test message to a hard-coded destination. | 
| Esp55ProjectRISCV/firstProject/ | An ESP-IDF project based on an Ethernet-to-Wi-Fi bridging example. | 
| Esp55ProjectRISCV/secProject/project-name/ | A SoftAP-oriented example project template. | 
| EspP4Examples/ | Ethernet-to-Wi-Fi bridge example for ESP32-P4-style targets with packet flow-control handling. |
| PWM/ledc_fade/ | PWM/LEDC experiment directory. The current firmware in PWM/ledc_fade/main/main.c is a minimal timer/print loop rather than a complete LED fade implementation. |
| PYTHON SERVER/python.py | Python TCP server that listens on 192.168.1.6:1234 and echoes back received data. | 

  ## Project Highlights 
  
  ### 1. Movement Sensor 
  The Esp55ProjectRISCV/MovementSensor/ project demonstrates an interrupt-driven design where a GPIO edge event triggers an ISR, which notifies a FreeRTOS task to report timing information. 
  ### 2. UART Echo 
  The Esp55ProjectRISCV/UartProject/ project uses ESP-IDF UART APIs to configure a UART port, read incoming bytes, and echo them back.
  ### 3. Wi-Fi Client Test Messaging
  The Esp55ProjectRISCV/WifiTest/wifiTest/ project initializes the ESP32 as a Wi-Fi station, connects to a hard-coded network, and sends a simple test payload to a fixed IPv4 address and port. 
  ### 4. Ethernet-to-Wi-Fi 
  Bridging The EspP4Examples/ project implements a packet-forwarding example between Ethernet and Wi-Fi. It includes a small queue-based flow-control mechanism to handle cases where the Ethernet path processes data faster than the Wi-Fi path. 
  ### 5. Python Echo Server 
  The PYTHON SERVER/python.py script provides a simple TCP listener that can be used alongside the Wi-Fi examples for basic message exchange testing. 
  ## Prerequisites 
  - ESP-IDF installed and configured for your target board. 
  - A compatible Espressif development board. 
  - Python 3 for the server script. 
  - A serial connection for flashing and monitoring firmware. 
  ## Getting Started 
  Each project is an independent ESP-IDF application. From the project directory, use the standard workflow: 
  bash 
  <span data-diff-end="50"></span> <span data-diff-start="51"></span>idf.py set-target <target> <span data-diff-end="51"></span> <span data-diff-start="52"></span>idf.py build <span data-diff-end="52"></span> <span data-diff-start="53"></span>idf.py -p <PORT> flash monitor <span data-diff-end="53"></span> <span data-diff-start="54"></span> For the Python server: bash <span data-diff-end="58"></span> <span data-diff-start="59"></span>python "PYTHON SERVER/python.py" <span data-diff-end="59"></span> <span data-diff-start="60"></span> 
  ## Important Notes 
  - Several Wi-Fi examples use hard-coded SSID/password values and fixed IP addresses. These should be updated before using them on a real network. 
  - The projects in this repository are primarily educational and experimental in nature, rather than production-ready applications. 
  - The PWM project currently contains a minimal test loop and does not yet implement a complete LED fade example.