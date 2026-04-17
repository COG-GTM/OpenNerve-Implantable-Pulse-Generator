# Asymmetric Biphasic Stimulation Waveform

## 1. Technical Specification

### Waveform Shape

The biphasic stimulation waveform consists of four phases per pulse cycle:

```
        Positive Phase    Interphase Gap    Negative Phase      Idle
       |<--- pw+ --->|<---- gap ---->|<--- pw- --->|<-- idle -->|
       |              |               |              |            |
 DAC+  |  ___________  |               |              |            |
       | |           | |               |              |            |
       | |           | |               |              |            |
  0V   |-|           |-|_______________|-             |____________|---
       |              |               | |            |            |
 DAC-  |              |               | |____________|            |
       |              |               |              |            |
       |<------------- pulsePeriod_us ------------------------------>|
```

**Phase Sequence:** POSITIVE -> GAP -> NEGATIVE -> IDLE -> POSITIVE -> ...

### Timing Parameters

| Parameter | Field | Unit | Description |
|-----------|-------|------|-------------|
| Positive Phase Width | `positiveWidth_us` | us | Duration of the cathodic (positive) current phase |
| Negative Phase Width | `negativeWidth_us` | us | Duration of the anodic (negative) current phase |
| Interphase Gap | `interphaseGap_us` | us | Dead time between positive and negative phases (can be 0) |
| Pulse Period | `pulsePeriod_us` | us | Total period of one biphasic pulse cycle |
| Train On Duration | `trainOnDuration_ms` | ms | Duration of active stimulation within a train cycle |
| Train Off Duration | `trainOffDuration_ms` | ms | Duration of rest (discharge) within a train cycle |

**Idle Time** is computed automatically:
```
idle_us = pulsePeriod_us - positiveWidth_us - interphaseGap_us - negativeWidth_us
```

### State Machine

The ISR callback (`app_func_stim_biphasic_cb`) implements a four-state machine driven by timer interrupts:

```
                 TO_LOW                    TO_LOW
  POSITIVE  ------------->  GAP  ------------->  NEGATIVE
     ^                (if gap > 0)                  |
     |                                              |
     |    TO_LOW (skip gap)                    TO_LOW
     |   POSITIVE ---------> NEGATIVE               |
     |                (if gap == 0)                  v
     |                                            IDLE
     +<-----------------  TO_LOW  -----------------+
```

On each `TO_LOW` interrupt:
- The timer autoreload and compare registers are reconfigured for the next phase duration
- This allows asymmetric positive/negative widths without additional timers

On each `BEFORE_HIGH` interrupt:
- The multiplexer is configured for the upcoming phase:
  - POSITIVE: routes to `sel_positive` (cathodic path)
  - NEGATIVE: routes to `sel_negative` (anodic path)
  - GAP/IDLE: routes to `sel_discharge` (passive discharge)

### Train Timing

Stimulation pulses are only delivered during the train-on period. During train-off, the multiplexer is set to discharge regardless of the phase state. The train timer advances with each pulse period.

```
|<---- train_on ---->|<---- train_off ---->|<---- train_on ---->|
|  pulse pulse pulse  |    discharge         |  pulse pulse pulse  |
```

### Ramp Envelope

When ramp is enabled, the DAC amplitude follows a sine-quarter-wave envelope:
- **Ramp Up**: DAC voltage increases from 0 to `max_amplitude_mV` over `rampUpDuration_us`
- **Steady State**: DAC held at `max_amplitude_mV`
- **Ramp Down**: DAC voltage decreases from `max_amplitude_mV` to 0 over `rampDownDuration_us`

Ramp parameters are set via `app_func_stim_biphasic_dac1_ramp_set()`.

### Charge Balance

For symmetric parameters (`positiveWidth_us == negativeWidth_us`), the waveform is inherently charge-balanced since both phases use the same DAC amplitude for equal durations.

For asymmetric parameters, charge balance is **not** enforced by firmware. The caller is responsible for ensuring safe charge delivery. Consider:
```
Q_positive = I * positiveWidth_us
Q_negative = I * negativeWidth_us
```
If `positiveWidth_us != negativeWidth_us`, a net DC charge will be delivered per pulse.

---

## 2. BLE Protocol

### Opcodes

| Opcode | Value | Direction | Description |
|--------|-------|-----------|-------------|
| `OP_SET_BIPHASIC_STIMULUS_PARAMETERS` | `0x4C` | Write | Set biphasic waveform parameters |
| `OP_GET_BIPHASIC_STIMULUS_PARAMETERS` | `0x4D` | Read | Get current biphasic waveform parameters |

### SET Payload Format (24 bytes)

| Offset | Size | Field | Unit |
|--------|------|-------|------|
| 0 | 4 | `positiveWidth_us` | microseconds |
| 4 | 4 | `negativeWidth_us` | microseconds |
| 8 | 4 | `interphaseGap_us` | microseconds |
| 12 | 4 | `pulsePeriod_us` | microseconds |
| 16 | 4 | `trainOnDuration_ms` | milliseconds |
| 20 | 4 | `trainOffDuration_ms` | milliseconds |

All fields are `uint32_t`, little-endian byte order.

### GET Response Format (24 bytes)

Same layout as the SET payload, reading back the currently configured values. Train durations are converted back from internal microsecond representation.

### Response Status Codes

| Status | Meaning |
|--------|---------|
| `STATUS_OK` | Parameters accepted |
| `STATUS_PAYLOAD_LEN_ERR` | Payload length != 24 bytes |

---

## 3. Parameter Reference (SPIDs)

The following Stimulation Parameter IDs (SPIDs) are used for persistent parameter storage and therapy session configuration:

| SPID | ID | Type | Min | Max | Default | Step | Description |
|------|-----|------|-----|-----|---------|------|-------------|
| `SPID_WAVEFORM_TYPE` | SP20 | `_Float64` | 0 | 1 | 0 | 1 | 0 = square wave (existing), 1 = biphasic |
| `SPID_BIPHASIC_POSITIVE_WIDTH` | SP21 | `_Float64` | 10 | 10000 | 500 | 10 | Positive phase width (us) |
| `SPID_BIPHASIC_NEGATIVE_WIDTH` | SP22 | `_Float64` | 10 | 10000 | 500 | 10 | Negative phase width (us) |
| `SPID_BIPHASIC_INTERPHASE_GAP` | SP23 | `_Float64` | 0 | 10000 | 0 | 10 | Interphase gap (us) |
| `SPID_BIPHASIC_PULSE_FREQUENCY` | SP24 | `_Float64` | 1 | 2000 | 5 | 1 | Pulse frequency (Hz) |
| `SPID_BIPHASIC_TRAIN_ON_DURATION` | SP25 | `_Float64` | 10 | 300 | 10 | 10 | Train on duration (s) |
| `SPID_BIPHASIC_TRAIN_OFF_DURATION` | SP26 | `_Float64` | 0 | 300 | 90 | 10 | Train off duration (s) |

### Waveform Type Selection

When `SPID_WAVEFORM_TYPE` is set to `1`, the therapy session manager uses the biphasic SPIDs (SP21-SP26) to configure and start biphasic stimulation instead of the existing square-wave stimulation. When set to `0` (default), the existing `app_func_stim_stim1_start()` path is used.

---

## 4. Usage Guide

### DVT Mode: Direct BLE Control

1. **Configure parameters** via BLE opcode `0x4C`:
   ```
   Payload (24 bytes, little-endian):
     positiveWidth_us  = 200   -> [C8 00 00 00]
     negativeWidth_us  = 200   -> [C8 00 00 00]
     interphaseGap_us  = 100   -> [64 00 00 00]
     pulsePeriod_us    = 1000  -> [E8 03 00 00]
     trainOnDuration_ms = 10000 -> [10 27 00 00]
     trainOffDuration_ms = 90000 -> [90 5F 01 00]
   ```

2. **Set DAC voltage** using existing `OP_SET_DAC_AB_OUTPUT_VOLTAGE` opcode

3. **Start stimulation** using existing DVT start commands (the biphasic driver will be used when parameters are configured)

4. **Read back parameters** via BLE opcode `0x4D` (no payload required)

### Therapy Session: Automated Control

1. **Set `SPID_WAVEFORM_TYPE` to 1** via parameter management BLE commands

2. **Configure biphasic SPIDs** (SP21-SP26) with desired values

3. **Start therapy session** normally - the therapy session manager will automatically use the biphasic driver based on the waveform type setting

### Example: Asymmetric Biphasic with Interphase Gap

```
Positive width:  200 us
Negative width:  400 us  (asymmetric - longer anodic phase)
Interphase gap:  50 us
Pulse frequency: 30 Hz   (period = 33333 us)
Train on:        30 s
Train off:       60 s
```

This produces:
- 200 us cathodic pulse
- 50 us dead time
- 400 us anodic pulse
- 32683 us idle
- Repeated at 30 Hz for 30 seconds, then 60 seconds rest

---

## 5. Safety Considerations

### Parameter Validation

- **Pulse period constraint**: `pulsePeriod_us` should be greater than `positiveWidth_us + interphaseGap_us + negativeWidth_us`. If not, the idle phase duration defaults to 1 us minimum.
- **Frequency limits**: SPID validation enforces pulse frequency between 1-2000 Hz.
- **Width limits**: Phase widths are validated to 10-10000 us range.
- **Amplitude clamping**: The therapy session manager clamps pulse amplitude to `max_safe_amplitude_mA` before starting stimulation.

### Charge Balance

The firmware does **not** enforce charge balance for asymmetric configurations. For chronic implanted stimulation:
- Prefer symmetric widths (`positiveWidth_us == negativeWidth_us`) for charge-balanced delivery
- If asymmetric widths are required, ensure the application layer verifies charge balance
- Consider interphase gap to allow electrode recovery between phases

### Mutual Exclusion

The biphasic driver shares `HANDLE_PULSE1_TIM` with the existing `stim1` driver. They are mutually exclusive:
- `biphasicWave.is_running` is checked in ISR dispatch to route callbacks
- `app_func_stim_off()` stops both drivers
- Double-start is guarded (`if (biphasicWave.is_running) return`)

### Maximum Parameter Limits

| Parameter | Minimum | Maximum |
|-----------|---------|---------|
| Phase width | 10 us | 10,000 us |
| Interphase gap | 0 us | 10,000 us |
| Pulse frequency | 1 Hz | 2,000 Hz |
| Train on duration | 10 s | 300 s |
| Train off duration | 0 s | 300 s |
| Pulse amplitude | 0.2 mA | 5.0 mA (clamped to max safe) |

### Firmware-Level Protections

1. **Short circuit detection**: Therapy session checks for `EVENT_SHORT_CIRCUIT` before starting
2. **Watchdog refresh**: Maintained during therapy session loop
3. **Clean shutdown**: `app_func_stim_off()` disables all stimulation paths including biphasic
4. **Timer safety**: Stop functions halt all timer interrupts and set multiplexer to discharge
