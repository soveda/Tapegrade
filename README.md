# Tapegrade

Cassette-style tape degradation processor for the  
Music Thing Modular Workshop System Computer inspired by the ZVEX lofi junky.

---

# Features

## Tape Transport Modulation

Tapegrade continuously modulates playback delay time to create unstable cassette-style pitch movement and stereo wandering.

---

## Tape Tone Shaping

The delayed signal passes through a simple low-pass smoothing stage.

Switch modes progressively darken the signal to emulate:

- fresh tape
- aged cassette
- damaged tape

---

## Tape Saturation

Input audio is processed through a lightweight soft-clipping stage.
---

## Noise & Degradation

### Old Tape Mode

Adds:

- filtered cassette hiss
- subtle high-frequency noise

### Damaged Tape Mode

Adds:

- louder broadband hiss
- filtered decaying crackle bursts
- unstable degraded tape texture

---

# Controls

| Control | Function |
|---|---|
| Main | Wet/dry mix |
| X | Modulation depth |
| Y | Transport instability / flutter speed |
| Switch Up | Cleaner tape |
| Switch Middle | Old cassette |
| Switch Down | Damaged cassette |

---

# Inputs

| Input | Function |
|---|---|
| Audio In 1 | Main audio input |
| CV In 2 | External modulation control |

---

# Outputs

| Output | Function |
|---|---|
| Audio Out 1 | Left output |
| Audio Out 2 | Right output |


---

# DSP Notes

Implementation details:

- fixed-point DSP throughout
- 8-bit fractional interpolated delay reads
- 4096-sample circular delay buffer
- no floating point in audio path
- ISR-safe low-overhead processing

Approximate tape delay:

- ~85ms at 48kHz

---

# Build

Example build process:

```bash
mkdir build
cd build
cmake ..
make
