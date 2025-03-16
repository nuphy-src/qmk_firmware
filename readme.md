# Project forked from nuphy-src/qmk_firmware

## improvements:

1. Added SignalRGB support for Nuphy Keyboards using qmk firmware.
2. Fixed some keymapping errors in nuphy keyboard plugin for Air96 V2/Air75 V2 from [SignalRGB](https://github.com/SRGBmods/qmk-plugins/blob/main/Nuphy/NuPhy_Air96v2_QMK_ANSI_Keyboard.js). Plugins are saved in [SRGB_qmk_plugins](/SRGB_qmk_plugins). You should copy .js file to C:\Users\\[YOUR USERNAME]\Documents\WhirlwindFX\Plugins.
3. Added SignalRGB plugin for Air60 v2.
4. Added reverse NumLock indicator for Air96 V2.
5. Changed battery indicator for Air96 v2: Led appears yellow when 50-95%, rather than orange.

## Note: SignalRGB / VIA Issue
(From [document](https://docs.signalrgb.com/qmk) provided by SignalRGB)

Please note that SignalRGB and VIA do not happily run at the same time. You must quit either VIA (or close the browser tab) before running SignalRGB, or close SignalRGB before attempting to use VIA as the raw_hid_recieve solution that both Software(s) use conflict with each other.

---
# Quantum Mechanical Keyboard Firmware

[![Current Version](https://img.shields.io/github/tag/qmk/qmk_firmware.svg)](https://github.com/qmk/qmk_firmware/tags)
[![Discord](https://img.shields.io/discord/440868230475677696.svg)](https://discord.gg/Uq7gcHh)
[![Docs Status](https://img.shields.io/badge/docs-ready-orange.svg)](https://docs.qmk.fm)
[![GitHub contributors](https://img.shields.io/github/contributors/qmk/qmk_firmware.svg)](https://github.com/qmk/qmk_firmware/pulse/monthly)
[![GitHub forks](https://img.shields.io/github/forks/qmk/qmk_firmware.svg?style=social&label=Fork)](https://github.com/qmk/qmk_firmware/)

This is a keyboard firmware based on the [tmk\_keyboard firmware](https://github.com/tmk/tmk_keyboard) with some useful features for Atmel AVR and ARM controllers, and more specifically, the [OLKB product line](https://olkb.com), the [ErgoDox EZ](https://ergodox-ez.com) keyboard, and the Clueboard product line.

## Documentation

* [See the official documentation on docs.qmk.fm](https://docs.qmk.fm)

The docs are powered by [Docsify](https://docsify.js.org/) and hosted on [GitHub](/docs/). They are also viewable offline; see [Previewing the Documentation](https://docs.qmk.fm/#/contributing?id=previewing-the-documentation) for more details.

You can request changes by making a fork and opening a [pull request](https://github.com/qmk/qmk_firmware/pulls), or by clicking the "Edit this page" link at the bottom of any page.

## Supported Keyboards

* [Planck](/keyboards/planck/)
* [Preonic](/keyboards/preonic/)
* [ErgoDox EZ](/keyboards/ergodox_ez/)
* [Clueboard](/keyboards/clueboard/)
* [Cluepad](/keyboards/clueboard/17/)
* [Atreus](/keyboards/atreus/)

The project also includes community support for [lots of other keyboards](/keyboards/).

## Maintainers

QMK is developed and maintained by Jack Humbert of OLKB with contributions from the community, and of course, [Hasu](https://github.com/tmk). The OLKB product firmwares are maintained by [Jack Humbert](https://github.com/jackhumbert), the Ergodox EZ by [ZSA Technology Labs](https://github.com/zsa), the Clueboard by [Zach White](https://github.com/skullydazed), and the Atreus by [Phil Hagelberg](https://github.com/technomancy).

## Official Website

[qmk.fm](https://qmk.fm) is the official website of QMK, where you can find links to this page, the documentation, and the keyboards supported by QMK.
