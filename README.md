# LittleFS Implementation on IS25LP128 NOR Flash

This repository provides a reference implementation of the LittleFS filesystem on the IS25LP128 NOR Flash memory using the nRF52832 development board and Zephyr RTOS. The implementation showcases how to set up LittleFS on a non-volatile memory device like IS25LP128, with optimized read/write operations and wear leveling.

## Project Overview

The main objective of this project is to create a robust filesystem solution for managing non-volatile data storage on the IS25LP128 NOR Flash. LittleFS is a lightweight filesystem designed for embedded systems that require reliability and a small memory footprint. This repository demonstrates how to integrate LittleFS, enabling file storage, retrieval, and wear leveling capabilities for the IS25LP128 memory.

### Key Features
- **Memory Used**: IS25LP128 (128 Mbit Serial NOR Flash Memory)
- **Board**: nRF52832 Development Kit
- **RTOS**: Zephyr RTOS
- **Filesystem**: LittleFS (Lightweight Fail-Safe Filesystem)
- **Operations Supported**: Read, Write, File Creation, Deletion, and Directory Management

## Hardware and Software Requirements

### Hardware
- nRF52832 Development Kit
- IS25LP128 NOR Flash Memory Module
- Debugger (e.g., J-Link) for programming and debugging

### Software
- Zephyr RTOS (2.6.0 or later recommended)
- ARM GCC Toolchain
- nRF Command Line Tools or Segger Embedded Studio
- CMake (for building the project)
