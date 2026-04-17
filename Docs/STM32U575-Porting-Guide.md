# STM32U575 Porting Guide

## Overview

This document describes how to port the OpenNerve IPG Gen2 firmware from the **STM32U585QIIxQ** to the **STM32U575QIIxQ** MCU variant. The STM32U575 is a cost-reduced variant in the same UFBGA132 package that omits the hardware cryptographic accelerators (HASH, PKA, AES/SAES) but is otherwise pin-compatible with identical memory layout (2 MB Flash, 768 KB RAM + 16 KB SRAM4).

## Key Differences: STM32U585 vs STM32U575

| Feature | STM32U585 | STM32U575 |
|---------|-----------|-----------|
| Hardware HASH (SHA-256) | Yes | **No** |
| Hardware PKA (ECDSA) | Yes | **No** |
| Hardware AES/SAES | Yes | **No** |
| Flash | 2048 KB | 2048 KB |
| RAM | 768 KB + 16 KB SRAM4 | 768 KB + 16 KB SRAM4 |
| Package (UFBGA132) | Yes | Yes |
| Core (Cortex-M33) | Yes | Yes |
| Pin-compatible | - | Yes |

## Conditional Compilation Strategy

The firmware uses the preprocessor define `STM32U585xx` (set by the STM32 HAL/CMSIS headers based on the target MCU) to conditionally enable hardware crypto features. When building for the STM32U575, the define `STM32U575xx` is set instead, and all crypto-dependent code is compiled out.

### HAL Module Configuration (`stm32u5xx_hal_conf.h`)

The HAL modules `HAL_HASH_MODULE_ENABLED` and `HAL_PKA_MODULE_ENABLED` are wrapped with `#ifdef STM32U585xx` so they are only enabled on the U585 variant.

### Affected Source Files

| File | Changes |
|------|---------|
| `Core/Inc/stm32u5xx_hal_conf.h` | HASH and PKA modules conditionally enabled |
| `Core/Inc/hash.h` | `extern HASH_HandleTypeDef` guarded by `HAL_HASH_MODULE_ENABLED` |
| `Core/Inc/pka.h` | `extern PKA_HandleTypeDef` guarded by `HAL_PKA_MODULE_ENABLED` |
| `Core/Src/hash.c` | Init function body and MSP callbacks guarded |
| `Core/Src/pka.c` | Handle declaration and MSP callbacks guarded |
| `Core/Src/stm32u5xx_it.c` | HASH and PKA interrupt handlers guarded |
| `App/Functions/Src/app_func_authentication.c` | ECDSA verification and SHA-256 hashing guarded |

### New Files for U575

| File | Description |
|------|-------------|
| `STM32U575QIIXQ_FLASH.ld` | Linker script (identical memory layout to U585) |
| `Core/Startup/startup_stm32u575qiixq.s` | Startup assembly (identical vector table; HASH/PKA IRQs remain as weak defaults) |

## Behavioral Differences on STM32U575

When built for the STM32U575:

- **ECDSA signature verification** (`app_func_auth_verify_sign_admin`, `app_func_auth_user_class_get`): Returns failure / `USER_CLASS_INVALID`. Hardware PKA is not available.
- **SHA-256 hash comparison** (`app_func_auth_compare_fram_hash`, `app_func_auth_compare_flash_hash`): Returns failure. Hardware HASH is not available.
- **All other functionality** (stimulation therapy, sensing, BLE communication, OTA updates, parameter management) operates identically.

## Building for STM32U575

In STM32CubeIDE:

1. Create a new build configuration targeting `STM32U575QIIxQ`
2. Set the device in project properties to STM32U575QIIxQ
3. Use linker script `STM32U575QIIXQ_FLASH.ld`
4. Use startup file `Core/Startup/startup_stm32u575qiixq.s`
5. The CMSIS/HAL headers will automatically define `STM32U575xx` instead of `STM32U585xx`

## Future Considerations

- **Software crypto fallback**: If authentication is required on the U575 variant, a software implementation of ECDSA (P-256) and SHA-256 could be integrated (e.g., micro-ecc, mbedTLS). This would be gated with `#ifndef STM32U585xx` to use software crypto only when hardware is unavailable.
- **OTA authentication**: The current OTA update flow uses ECDSA signature verification. On U575, OTA updates will proceed without cryptographic verification unless a software fallback is implemented.
