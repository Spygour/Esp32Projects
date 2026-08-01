# ESP32 Projects

## Overview

This repository contains a collection of **ESP-IDF experiments, prototypes, and example projects** developed for Espressif SoCs.

The projects cover different embedded concepts including:

- GPIO interrupts and FreeRTOS task synchronization
- UART communication
- Wi-Fi networking
- Ethernet-to-Wi-Fi bridging
- PWM experiments
- TCP communication with a Python helper server

Each project is independent and can be built, flashed, and tested separately using standard ESP-IDF tools.

---

## Repository Structure

| Path | Description |
|---|---|
| `Esp55ProjectRISCV/` | Collection of ESP32-family example projects. |
| `Esp55ProjectRISCV/MovementSensor/` | GPIO interrupt example using an ISR and FreeRTOS task to measure movement timing. |
| `Esp55ProjectRISCV/UartProject/` | UART echo example using ESP-IDF UART APIs. |
| `Esp55ProjectRISCV/WifiTest/wifiTest/` | Wi-Fi station example connecting to a configured network and sending test messages. |
| `Esp55ProjectRISCV/firstProject/` | ESP-IDF project based on an Ethernet-to-Wi-Fi bridge example. |
| `Esp55ProjectRISCV/secProject/project-name/` | SoftAP-oriented example project template. |
| `EspP4Examples/` | Ethernet-to-Wi-Fi bridge example for ESP32-P4 targets with packet flow-control handling. |
| `PWM/ledc_fade/` | PWM/LEDC experimentation project. |
| `PYTHON SERVER/python.py` | Python TCP server used for communication testing. |

---

# Project Highlights

## 1. Movement Sensor

The `Esp55ProjectRISCV/MovementSensor/` project demonstrates an interrupt-driven embedded design.

A GPIO edge triggers an ISR, which communicates with a FreeRTOS task to process and report timing information.

Features:

- GPIO interrupt handling
- ISR-to-task synchronization
- FreeRTOS task management

---

## 2. UART Echo

The `Esp55ProjectRISCV/UartProject/` project demonstrates basic serial communication.

Features:

- UART initialization using ESP-IDF APIs
- Receiving serial data
- Echoing received bytes back to the sender

---

## 3. Wi-Fi Client Messaging

The `Esp55ProjectRISCV/WifiTest/wifiTest/` project configures the ESP32 as a Wi-Fi station.

Features:

- Wi-Fi station mode
- Network connection handling
- TCP message transmission

---

## 4. Ethernet-to-Wi-Fi Bridge

The `EspP4Examples/` project demonstrates packet forwarding between Ethernet and Wi-Fi interfaces.

Features:

- Ethernet communication
- Wi-Fi networking
- Queue-based flow control
- Handling different processing speeds between interfaces

---

## 5. Python TCP Server

The `PYTHON SERVER/python.py` script provides a simple TCP server for communication testing.

Features:

- TCP socket communication
- Data reception and echo response
- Useful for testing ESP32 networking applications

---

# Prerequisites

Before building the projects, install:

- ESP-IDF configured for your target board
- Compatible ESP32 development board
- Python 3
- USB serial connection for flashing and monitoring

---

# Getting Started

Each project is an independent ESP-IDF application.

Navigate to the desired project directory:

```bash
cd <project_directory>
idf.py set-target <target>
idf.py menuconfig
idf.py -p <PORT> flash
idf.py -p <PORT> monitor
idf.py -p <PORT> flash monitor
idf.py fullclean
idf.py -p <PORT> erase-flash