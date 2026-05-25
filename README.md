Tapegrade
Tapegrade is a mono-input / stereo-output cassette warble processor for the Workshop System Computer Card platform.
It recreates unstable cassette transport behaviour using interpolated delay modulation, wow/flutter drift, tape-darkening, hiss, crackle, and soft saturation.
Designed specifically for low CPU fixed-point DSP operation on RP2040 hardware.
 
Features
•    Mono input → internally generated stereo tape image
•    Interpolated modulated delay line
•    Wow and flutter transport instability
•    Stereo decorrelated modulation
•    Continuous tape-condition morphing
•    Cassette-style tone shaping
•    Soft tape saturation
•    Dynamic hiss and crackle generation
•    CV modulation support
•    Low CPU fixed-point DSP
•    RP2040 ISR-safe implementation
 
Signal Flow
Audio In 1
    ↓
Soft Saturation
    ↓
Tape Delay Buffer
    ↓
Wow / Flutter Modulation
    ↓
Stereo Delay Reads
    ↓
Tape Tone Filtering
    ↓
Hiss / Crackle Injection
    ↓
Wet Saturation
    ↓
Wet/Dry Mix
    ↓
Stereo Outputs
 
Inputs and Controls
Audio Input
Audio In 1
Main mono audio input.
Tapegrade is designed for mono input operation only. Stereo width is generated internally by the tape modulation engine.
Audio In 2
Dynamic tape-condition modulation input.
This input continuously morphs the tape state between:
•    clean cassette
•    aged cassette
•    heavily damaged cassette
Audio-rate modulation is supported.
 
Knobs
Main
Wet/dry mix.
•    Fully CCW = dry
•    Fully CW = fully processed
 
X
Tape modulation depth.
Controls overall wow/flutter intensity.
Higher values increase:
•    pitch instability
•    tape wobble
•    stereo movement
CV1
CV modulation for X.
CV1 modulates tape depth.
 
Y
Transport instability.
Controls flutter speed and transport agitation.
Higher values create:
•    faster flutter
•    rougher transport motion
•    more unstable tape behaviour
CV2
CV modulation for Y.
CV2 modulates instability amount.
 
Switch Modes
Up — Clean Tape
•    brighter response
•    minimal degradation
•    subtle transport movement
•    low hiss
 
Middle — Old Tape
•    darker tone
•    moderate hiss
•    more instability
•    worn cassette character
 
Down — Damaged Tape
•    darkest response
•    strong hiss
•    crackle bursts
•    unstable degraded cassette behaviour
 
DSP Design
Delay Engine
Tapegrade uses a single interpolated circular delay line.
Stereo width is created using independent left/right modulation paths.
The delay line is intentionally short to emulate cassette transport instability rather than echo.
 
Wow and Flutter
Transport instability combines:
•    slow random wow drift
•    independent flutter oscillators
•    decorrelated stereo modulation
This creates natural tape movement without repetitive LFO behaviour.
 
Tape Wear Morphing
Tape condition is continuously variable.
The degradation engine dynamically adjusts:
•    tone darkness
•    modulation instability
•    hiss level
•    crackle density
Audio In 2 can continuously scan through tape states.
 
Saturation
Tapegrade uses lightweight fixed-point soft clipping.
Saturation is applied:
1.    before the tape buffer
2.    again on the wet path
This creates compressed cassette-style texture while remaining computationally efficient.
 
Technical Notes
•    Designed for 48kHz ISR processing
•    Optimised for RP2040
•    No floating point DSP
•    No dynamic memory allocation
•    Fixed-point arithmetic throughout
•    4096-sample circular buffer
•    Approximately 85ms internal tape memory
 
