# mcHF — Open Source HF SDR Transceiver

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE)
[![Discussion](https://img.shields.io/badge/groups.io-mcHF-green.svg)](https://groups.io/g/mcHF/)

The mcHF is a fully open source software defined radio transceiver for the HF amateur bands (3–30 MHz). Its goal is planet-wide, off-grid communication — ionosphere permitting — with no dependence on cellular or satellite infrastructure.

Both hardware and firmware build on the work of many radio amateurs. The design is modular, uses mostly off-the-shelf components that can be soldered by hand, and is straightforward to repair when something goes wrong. An amateur radio licence is required to transmit.

[![mcHF demonstration video](https://img.youtube.com/vi/7Q5eKNbZNY8/0.jpg)](https://www.youtube.com/watch?v=7Q5eKNbZNY8)

## Features

- Transmit and receive on the 3–30 MHz amateur bands
- SSB, AM, FM and CW modes
- FT8 and WSPR digital mode decoding (in development)
- Real-time spectrum scope and waterfall
- Large 4.3" IPS LCD with capacitive touch
- 25 W output power
- Built-in 80 Wh lithium battery
- Dual USB-C ports for power, charging and PC connection
- Bluetooth stereo headphone support
- LoRa sub-GHz auxiliary radio

## Hardware

The radio is built around an STM32H747BIT6 dual-core MCU: the Cortex-M7 (480 MHz) runs the user interface and system control, while the Cortex-M4 handles the real-time DSP (demodulation, AGC, filtering, spectrum FFT).

### Altium PCB project files

| Status  | Version    | Location |
|---------|------------|----------|
| Latest  | 0.9 rev B  | [pcb/v9](./pcb/v9) |
| Develop | 0.8.5      | [pcb/v8/mchf](./pcb/v8/mchf) |
| Legacy  | 0.6.3      | [pcb/__legacy/v6](./pcb/__legacy/v6) |

### PCB gerbers

| Revision | Version | Location |
|----------|---------|----------|
| rev 8    | 0.8.5   | [testing/gerbers/0_8_5](./testing/gerbers/0_8_5) |
| rev 7    | 0.7.0   | [pcb/__legacy/bin/v7](./pcb/__legacy/bin/v7) |
| rev 6    | 0.6.3   | [pcb/__legacy/bin/v6](./pcb/__legacy/bin/v6) |

## Repository layout

| Directory | Contents |
|-----------|----------|
| [firmware](./firmware) | All firmware projects (see below) |
| [pcb](./pcb) | Altium PCB projects and gerbers |
| [mechanical](./mechanical) | Enclosure and mechanical design files |
| [testing](./testing) | Test data and manufacturing files |

## Building the firmware

The firmware is developed with [STM32CubeIDE](https://www.st.com/en/development-tools/stm32cubeide.html) (Eclipse CDT managed build — no separate Makefiles).

Clone with submodules (the ST HAL and the FT8 library are pulled in as git submodules):

```
git clone --recurse-submodules https://github.com/m0nka/mcHF.git
```

There are three independent firmware projects, each opened separately in STM32CubeIDE:

| Project | Core | Location |
|---------|------|----------|
| Application processor (UI, control) | CM7 | [firmware/app_proc/proj](./firmware/app_proc/proj) |
| Baseband DSP | CM4 | [firmware/baseband](./firmware/baseband) |
| Bootloader | CM7 | [firmware/bootloader](./firmware/bootloader) |

Open the project's `.project` file in STM32CubeIDE and use *Build → Build Project*. The application processor build produces `firmware/app_proc/proj/Release/mchf_app_proc.bin`, which can be flashed over SWD with `program_radio.bat` (requires STM32CubeProgrammer).

## Previous hardware generations

Older mcHF hardware versions are supported by the community-maintained UHSDR firmware:

- Firmware repository: [df8oe/UHSDR](https://github.com/df8oe/UHSDR)
- Project crash course and documentation: [UHSDR wiki](https://github.com/df8oe/UHSDR/wiki)

## Community and contributing

Questions, build reports and development discussion are welcome in the [mcHF discussion group](https://groups.io/g/mcHF/).

Feel free to build one yourself and contribute — bug reports, fixes and improvements to hardware, firmware or documentation are all appreciated.

## License

mcHF is licensed under the [GNU General Public License v3.0](LICENSE).

Copyright © 2013–2026 Krassi Atanassov, M0NKA
