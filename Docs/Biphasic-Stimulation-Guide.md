# Biphasic Stimulation Guide
OpenNerve Gen2
April 2026

---

## Introduction

Biphasic stimulation is a charge-balanced electrical stimulation technique in which each pulse consists of two sequential current phases of opposite polarity. Unlike monophasic (square wave) stimulation, where current flows in only one direction, biphasic pulses deliver a cathodic (stimulating) phase followed by an anodic (charge-recovery) phase. This ensures that the net charge injected into tissue over each pulse cycle is approximately zero.

### Why Biphasic Stimulation Matters

Charge-balanced stimulation is the gold standard for chronic neuromodulation because it significantly reduces two critical failure modes:

- **Tissue damage** — Unbalanced charge accumulation causes irreversible electrochemical reactions at the electrode-tissue interface, leading to pH shifts, gas evolution, and cell death. Biphasic pulses neutralize these reactions each cycle.
- **Electrode degradation** — Sustained DC current corrodes metal electrodes over time. Charge balancing dramatically extends electrode life, which is essential for implantable devices.

### Comparison with Existing Stimulation Modes

| Feature | Square Wave (Monophasic) | Sine Wave | Biphasic |
|---|---|---|---|
| Waveform | Unidirectional current pulse | Continuous sinusoidal AC | Two-phase current pulse with opposite polarities |
| Charge balance | No | Inherently balanced | Yes (when cathodic and anodic charge are matched) |
| Tissue safety | Limited to acute/short-term use | Safe for continuous use | Safe for chronic implanted use |
| Parameter control | Width, amplitude, frequency | Amplitude, frequency, on/off time | Cathodic width, anodic width, interphase gap, amplitude, frequency |
| Primary use case | Bench testing, acute experiments | Vagus nerve signal block (VNSB) | Chronic therapeutic stimulation |

### Clinical Applications

Biphasic stimulation is used across a wide range of neuromodulation therapies:

- **Vagus Nerve Stimulation (VNS)** — Treatment of drug-resistant epilepsy and depression. Typical parameters: 20-30 Hz, 200-500 us pulse width, 0.25-3.5 mA.
- **Spinal Cord Stimulation (SCS)** — Chronic pain management. Typical parameters: 40-120 Hz, 100-500 us pulse width, 1-10 mA.
- **Deep Brain Stimulation (DBS)** — Treatment of Parkinson's disease, essential tremor, and dystonia. Typical parameters: 130-185 Hz, 60-200 us pulse width, 1-5 mA.

---

## Waveform Parameters

The biphasic waveform is fully configurable through the following parameters:

| Parameter | Description | Unit | Range | Default |
|---|---|---|---|---|
| Cathodic Width | Duration of the stimulating (first) phase | us | 50-1000 | 200 |
| Anodic Width | Duration of the charge-balancing (second) phase | us | 50-1000 | 200 |
| Interphase Gap | Zero-current interval between cathodic and anodic phases | us | 0-500 | 50 |
| Pulse Frequency | Repetition rate of biphasic pulses | Hz | 1-1200 | 30 |
| Pulse Amplitude | Current amplitude of stimulation | mA | 0.1-5.0 | 0.5 |
| Train On Duration | Duration of active stimulation within each train cycle | s | 10-300 | 30 |
| Train Off Duration | Duration of stimulation pause between train cycles | s | 0-300 | 60 |

### Timing Diagram

One complete biphasic pulse cycle:

```
        Cathodic         Interphase    Anodic
        Phase            Gap           Phase
    +--+----------+                   
    |  |          |                   
    |  |          |                   
----+  +          +-------------------+          +----
                                      |          |
                                      |          |
                                      +----------+--+
    |<----------->|<--------->|<----------->|
     cathodic_w    interphase   anodic_w
                   gap
    |<---------------- pulse_period ----------------->|
```

- **Cathodic phase**: The initial stimulating phase. Current flows from the cathode electrode into tissue. This is the therapeutically active phase that depolarizes neural membranes.
- **Interphase gap**: A brief zero-current interval separating the two phases. This gap allows the cathodic phase to achieve its full stimulating effect before the anodic phase begins charge recovery. Setting this to 0 creates a symmetric biphasic pulse with no gap.
- **Anodic phase**: The charge-recovery phase. Current flows in the reverse direction to balance the charge injected during the cathodic phase.
- **Pulse period**: The total time from the start of one pulse to the start of the next, determined by the pulse frequency: `pulse_period = 1 / frequency`.

### Train Timing

Stimulation is delivered in repeating train cycles:

```
|<-------- Train On -------->|<------ Train Off ------>|<-------- Train On -------->|
|  pulse pulse pulse ... pulse|                         |  pulse pulse pulse ... pulse|
```

- During **Train On**, biphasic pulses are delivered at the configured frequency.
- During **Train Off**, no stimulation is delivered. Set Train Off to 0 for continuous stimulation.

---

## BLE Command Protocol

Four new BLE opcodes support biphasic stimulation. All commands require an active **Admin-authenticated** BLE session (opcode `0xF0`).

### Packet Format

All commands follow the standard OpenNerve BLE packet format:

```
[Opcode (1B)] [PayloadLen (1B)] [Payload (0-N bytes)] [CRC16 (2B)]
```

All multi-byte values in the payload are **uint32_t, little-endian**.

---

### 0x4C — SET_BIPHASIC_PARAMETERS

Sets all biphasic stimulation parameters in a single command.

**Request:**

| Byte Offset | Field | Type | Description |
|---|---|---|---|
| 0 | Opcode | uint8 | `0x4C` |
| 1 | PayloadLen | uint8 | `28` |
| 2-5 | cathodic_width | uint32_t | Cathodic phase width in microseconds |
| 6-9 | anodic_width | uint32_t | Anodic phase width in microseconds |
| 10-13 | interphase_gap | uint32_t | Interphase gap in microseconds |
| 14-17 | pulse_period | uint32_t | Pulse period in microseconds (= 1,000,000 / frequency) |
| 18-21 | amplitude | uint32_t | Current amplitude in microamps (e.g. 500 = 0.5 mA) |
| 22-25 | train_on | uint32_t | Train on duration in seconds |
| 26-29 | train_off | uint32_t | Train off duration in seconds |
| 30-31 | CRC16 | uint16_t | CRC-16 over bytes 0-29 |

**Response:**

| Byte Offset | Field | Type | Description |
|---|---|---|---|
| 0 | Opcode | uint8 | `0x4C` |
| 1 | PayloadLen | uint8 | `0` |
| 2 | Status | uint8 | `STATUS_SUCCESS` or `STATUS_INVALID` |
| 3-4 | CRC16 | uint16_t | CRC-16 |

**Status codes:**
- `STATUS_SUCCESS` — Parameters accepted and stored
- `STATUS_INVALID` — One or more parameters out of range; no parameters were changed

**Example — set 30 Hz, 200/200 us, 50 us gap, 0.5 mA, 30s on / 60s off:**

```csharp
// pulse_period for 30 Hz = 1,000,000 / 30 = 33,333 us
// amplitude in microamps: 0.5 mA = 500 uA
byte[] payload = new byte[28];
BitConverter.GetBytes((uint)200).CopyTo(payload, 0);      // cathodic_width
BitConverter.GetBytes((uint)200).CopyTo(payload, 4);      // anodic_width
BitConverter.GetBytes((uint)50).CopyTo(payload, 8);       // interphase_gap
BitConverter.GetBytes((uint)33333).CopyTo(payload, 12);   // pulse_period
BitConverter.GetBytes((uint)500).CopyTo(payload, 16);     // amplitude (0.5 mA)
BitConverter.GetBytes((uint)30).CopyTo(payload, 20);      // train_on
BitConverter.GetBytes((uint)60).CopyTo(payload, 24);      // train_off
SendCommand(0x4C, payload);
```

---

### 0x4D — GET_BIPHASIC_PARAMETERS

Reads the current biphasic stimulation parameters.

**Request:**

| Byte Offset | Field | Type | Description |
|---|---|---|---|
| 0 | Opcode | uint8 | `0x4D` |
| 1 | PayloadLen | uint8 | `0` |
| 2-3 | CRC16 | uint16_t | CRC-16 |

**Response:**

| Byte Offset | Field | Type | Description |
|---|---|---|---|
| 0 | Opcode | uint8 | `0x4D` |
| 1 | PayloadLen | uint8 | `28` |
| 2 | Status | uint8 | `STATUS_SUCCESS` |
| 3-6 | cathodic_width | uint32_t | Cathodic phase width in microseconds |
| 7-10 | anodic_width | uint32_t | Anodic phase width in microseconds |
| 11-14 | interphase_gap | uint32_t | Interphase gap in microseconds |
| 15-18 | pulse_period | uint32_t | Pulse period in microseconds |
| 19-22 | amplitude | uint32_t | Current amplitude in microamps |
| 23-26 | train_on | uint32_t | Train on duration in seconds |
| 27-30 | train_off | uint32_t | Train off duration in seconds |
| 31-32 | CRC16 | uint16_t | CRC-16 |

**Example:**

```csharp
var resp = SendCommand(0x4D, new byte[0]);
uint cathodicWidth  = BitConverter.ToUInt32(resp.Payload, 0);
uint anodicWidth    = BitConverter.ToUInt32(resp.Payload, 4);
uint interphaseGap  = BitConverter.ToUInt32(resp.Payload, 8);
uint pulsePeriod    = BitConverter.ToUInt32(resp.Payload, 12);
uint amplitude      = BitConverter.ToUInt32(resp.Payload, 16);
uint trainOn        = BitConverter.ToUInt32(resp.Payload, 20);
uint trainOff       = BitConverter.ToUInt32(resp.Payload, 24);

double frequency = 1_000_000.0 / pulsePeriod;
double amp_mA = amplitude / 1000.0;
Console.WriteLine($"Biphasic: {cathodicWidth}/{anodicWidth} us, gap={interphaseGap} us, freq={frequency:F1} Hz, amp={amp_mA:F1} mA");
Console.WriteLine($"Train: {trainOn}s on / {trainOff}s off");
```

---

### 0x4E — ENABLE_BIPHASIC_STIMULATION

Enables biphasic stimulation output. Optionally enables impedance monitoring during stimulation.

**Request:**

| Byte Offset | Field | Type | Description |
|---|---|---|---|
| 0 | Opcode | uint8 | `0x4E` |
| 1 | PayloadLen | uint8 | `1` |
| 2 | imc_enable | uint8 | `0x00` = stimulation only, `0x01` = stimulation + impedance monitoring |
| 3-4 | CRC16 | uint16_t | CRC-16 |

**Response:**

| Byte Offset | Field | Type | Description |
|---|---|---|---|
| 0 | Opcode | uint8 | `0x4E` |
| 1 | PayloadLen | uint8 | `0` |
| 2 | Status | uint8 | `STATUS_SUCCESS` or `STATUS_INVALID` |
| 3-4 | CRC16 | uint16_t | CRC-16 |

**Example — start biphasic stimulation with impedance monitoring:**

```csharp
SendCommand(0x4E, new byte[] { 0x01 });
```

---

### 0x4F — DISABLE_BIPHASIC_STIMULATION

Immediately stops biphasic stimulation output.

**Request:**

| Byte Offset | Field | Type | Description |
|---|---|---|---|
| 0 | Opcode | uint8 | `0x4F` |
| 1 | PayloadLen | uint8 | `0` |
| 2-3 | CRC16 | uint16_t | CRC-16 |

**Response:**

| Byte Offset | Field | Type | Description |
|---|---|---|---|
| 0 | Opcode | uint8 | `0x4F` |
| 1 | PayloadLen | uint8 | `0` |
| 2 | Status | uint8 | `STATUS_SUCCESS` |
| 3-4 | CRC16 | uint16_t | CRC-16 |

**Example:**

```csharp
SendCommand(0x4F, new byte[0]);
```

---

## Usage Examples

### 1. Configure and Start Biphasic Stimulation

Step-by-step procedure to configure and start biphasic stimulation via BLE:

1. **Connect and authenticate** — Establish a BLE connection and complete the Admin authentication handshake (`0xF0`).
2. **Set parameters** — Send `SET_BIPHASIC_PARAMETERS` (`0x4C`) with the desired waveform configuration.
3. **Verify parameters** — Send `GET_BIPHASIC_PARAMETERS` (`0x4D`) and confirm the response matches your intended settings.
4. **Start stimulation** — Send `ENABLE_BIPHASIC_STIMULATION` (`0x4E`) with `imc_enable = 0x00`.
5. **Monitor** — Stimulation will cycle through Train On / Train Off periods automatically.
6. **Stop stimulation** — Send `DISABLE_BIPHASIC_STIMULATION` (`0x4F`) when finished.

### 2. Start/Stop Biphasic Stimulation

```csharp
// Start biphasic stimulation (no impedance monitoring)
SendCommand(0x4E, new byte[] { 0x00 });

// ... stimulation is active ...

// Stop biphasic stimulation
SendCommand(0x4F, new byte[0]);
```

### 3. Biphasic Stimulation with Impedance Monitoring

To monitor electrode impedance during stimulation, enable impedance monitoring when starting:

```csharp
// Set biphasic parameters first
byte[] payload = new byte[28];
BitConverter.GetBytes((uint)200).CopyTo(payload, 0);      // cathodic_width = 200 us
BitConverter.GetBytes((uint)200).CopyTo(payload, 4);      // anodic_width = 200 us
BitConverter.GetBytes((uint)50).CopyTo(payload, 8);       // interphase_gap = 50 us
BitConverter.GetBytes((uint)33333).CopyTo(payload, 12);   // 30 Hz
BitConverter.GetBytes((uint)500).CopyTo(payload, 16);     // amplitude = 0.5 mA (500 uA)
BitConverter.GetBytes((uint)30).CopyTo(payload, 20);      // train_on = 30 s
BitConverter.GetBytes((uint)60).CopyTo(payload, 24);      // train_off = 60 s
SendCommand(0x4C, payload);

// Enable biphasic stimulation WITH impedance monitoring
SendCommand(0x4E, new byte[] { 0x01 });

// Impedance results are logged to FRAM and can be read via OP_READ_IPG_LOG (0xAE)
// See IPG-Logging-Guide.md for details on reading impedance log entries (<IM> tags)
```

### 4. Common Research Protocol Parameter Sets

**VNS — Standard (30 Hz, 200 us symmetric):**

| Parameter | Value |
|---|---|
| Cathodic Width | 200 us |
| Anodic Width | 200 us |
| Interphase Gap | 50 us |
| Frequency | 30 Hz |
| Amplitude | 0.5 mA (titrate to therapeutic effect) |
| Train On | 30 s |
| Train Off | 300 s (5 min) |

**VNS — High Frequency (applied research):**

| Parameter | Value |
|---|---|
| Cathodic Width | 100 us |
| Anodic Width | 100 us |
| Interphase Gap | 25 us |
| Frequency | 100 Hz |
| Amplitude | 1.0 mA |
| Train On | 30 s |
| Train Off | 60 s |

**SCS — Tonic (research protocol):**

| Parameter | Value |
|---|---|
| Cathodic Width | 300 us |
| Anodic Width | 300 us |
| Interphase Gap | 100 us |
| Frequency | 50 Hz |
| Amplitude | 2.0 mA |
| Train On | 300 s (continuous) |
| Train Off | 0 s |

---

## Parameter Storage (FRAM)

Biphasic stimulation parameters are stored persistently in FRAM using the following Stimulation Parameter IDs (SPIDs). These values survive power cycles and are loaded automatically on boot.

| SPID | Name | Description | Stored Unit |
|---|---|---|---|
| SP20 | BIPHASIC_CATHODIC_WIDTH | Cathodic phase duration | microseconds (uint32) |
| SP21 | BIPHASIC_ANODIC_WIDTH | Anodic phase duration | microseconds (uint32) |
| SP22 | BIPHASIC_INTERPHASE_GAP | Gap between phases | microseconds (uint32) |
| SP23 | BIPHASIC_FREQUENCY | Pulse repetition frequency | Hz (uint32, stored as pulse period in us) |
| SP24 | BIPHASIC_AMPLITUDE | Stimulation current amplitude | milliamps (uint32, stored as DAC code) |
| SP25 | BIPHASIC_TRAIN_ON | Train on duration | seconds (uint32) |
| SP26 | BIPHASIC_TRAIN_OFF | Train off duration | seconds (uint32) |

Parameter changes are logged to the FRAM event log as `<PA>` entries (see [IPG Logging Guide](IPG-Logging-Guide.md) for details).

---

## Safety Considerations

### Charge Balance

For true charge balance, the total charge delivered during the cathodic phase must equal the total charge recovered during the anodic phase:

```
Q_cathodic = cathodic_width * amplitude
Q_anodic   = anodic_width  * amplitude
```

When `cathodic_width == anodic_width` (the default), charge balance is guaranteed because the same amplitude is used for both phases. If asymmetric pulse widths are configured, the firmware does **not** automatically adjust the anodic amplitude — the researcher is responsible for ensuring that the charge is balanced for their specific experimental protocol.

> **Recommendation:** Unless your protocol specifically requires asymmetric phase widths, keep `cathodic_width == anodic_width` for guaranteed charge balance.

### Maximum Safe Amplitude

The firmware enforces a `MAX_SAFE_AMPLITUDE` limit. If a commanded amplitude exceeds this limit:

1. The amplitude is automatically clamped to `MAX_SAFE_AMPLITUDE`.
2. An `LSA` (Lower Stim Amplitude) event is written to the FRAM log.
3. Stimulation proceeds at the clamped amplitude.

Always verify the actual amplitude after setting parameters by using `GET_BIPHASIC_PARAMETERS` (`0x4D`).

### Electrode Impedance Monitoring

Electrode impedance should be checked before and during biphasic stimulation to ensure safe operation:

- **Before stimulation** — Use the impedance measurement function (see [Getting Started](Getting-Started.md)) to verify electrode impedance is within the normal range (approximately 100 Ohms to 5 kOhms).
- **During stimulation** — Enable impedance monitoring by setting `imc_enable = 0x01` when calling `ENABLE_BIPHASIC_STIMULATION` (`0x4E`). Impedance measurements are logged as `<IM>` entries in the FRAM log.
- **Abnormal impedance events** — The firmware automatically logs `SC` (short circuit), `HI` (high impedance), and `NI` (normal impedance) events. Monitor these in the log to detect electrode issues.

### Emergency Stop

Biphasic stimulation can be stopped immediately by any of the following:

- **BLE command** — Send `DISABLE_BIPHASIC_STIMULATION` (`0x4F`). Stimulation stops within one pulse period.
- **Magnet gesture** — Hold a magnet to the device for the configured duration to trigger a sleep transition. Stimulation is stopped and a `SE` (stim stop) event is logged before the device enters sleep.
- **Power loss** — If power is interrupted, stimulation stops immediately. The firmware writes a `SE` event on the next boot if a session was active.
- **Watchdog timeout** — If the firmware becomes unresponsive, the watchdog timer triggers a reset. A `UF` event is written on the subsequent boot.

---

## Firmware Architecture

### State Machine

The biphasic waveform is generated by a timer interrupt-driven state machine with four states:

```
BIPHASIC_CATHODIC  -->  BIPHASIC_INTERPHASE  -->  BIPHASIC_ANODIC  -->  BIPHASIC_IDLE
       |                                                                     |
       +<-------------------------------------------------------------------+
```

| State | Duration | Output |
|---|---|---|
| `BIPHASIC_CATHODIC` | `cathodic_width` us | DAC output at configured amplitude, cathodic polarity |
| `BIPHASIC_INTERPHASE` | `interphase_gap` us | DAC output at zero (no current) |
| `BIPHASIC_ANODIC` | `anodic_width` us | DAC output at configured amplitude, anodic polarity |
| `BIPHASIC_IDLE` | Remainder of pulse period | DAC output at zero (no current) |

The timer interrupt fires at each state transition. The interrupt service routine advances the state, reconfigures the DAC output and multiplexer polarity, and reloads the timer with the duration of the next state.

### DAC Control

The stimulation current amplitude is set via the onboard DAC. The firmware converts the requested amplitude in milliamps to a DAC code based on the hardware gain and reference voltage. The same DAC code is used for both cathodic and anodic phases; polarity is controlled by the analog multiplexer.

### Multiplexer Switching

Polarity reversal between cathodic and anodic phases is achieved by switching the analog multiplexer that routes current to the electrode pair. The multiplexer control signal is toggled in the timer ISR at the cathodic-to-anodic transition (after the interphase gap).

### Synchronization with Other Waveforms

Biphasic stimulation operates independently of the existing square wave and sine wave modes. Only one stimulation mode can be active at a time. Attempting to enable biphasic stimulation while another mode is active will return `STATUS_INVALID`. Stop the current stimulation mode before switching to biphasic.

---

## Relevant Source Files

| File | Description |
|---|---|
| `App/Src/app_mode_therapy_session.c` | Therapy session management, train on/off timing |
| `App/Functions/Src/app_func_state_machine.c` | State machine transitions and event logging |
| `App/Functions/Src/app_func_stim.c` | Stimulation waveform generation and DAC control |
| `App/Functions/Inc/app_func_stim.h` | Stimulation parameter definitions and constants |
| `App/Functions/Src/app_func_params.c` | FRAM parameter read/write for SPIDs |
| `App/Src/app_mode_ble_connection.c` | BLE opcode handlers for 0x4C-0x4F |
| `App/Bsp/Inc/bsp_dac.h` | DAC hardware abstraction |
| `App/Bsp/Inc/bsp_mux.h` | Multiplexer control for polarity switching |
