```cpp
//
//  Tapegrade.cpp
//
//
//  Created by Adrian Vos on 25/05/2026.
//

#include "ComputerCard.h"

// Tapegrade
// Stereo cassette-warble processor for Workshop Computer.
//
// DSP features:
//
// - interpolated modulated delay
// - wow/flutter transport instability
// - stereo decorrelation
// - cassette tone shaping
// - soft saturation
// - hiss/crackle degradation
// - low CPU fixed-point DSP
//
// Designed for:
// - 48kHz ISR operation
// - RP2040 efficiency
// - stable real-time behaviour

class Tapegrade : public ComputerCard
{
public:

    // 4096-sample circular delay buffer.
    //
    // ~85ms at 48kHz.
    static constexpr int BUFFER_SIZE = 4096;
    static constexpr int BUFFER_MASK = BUFFER_SIZE - 1;

    int16_t delayBuffer[BUFFER_SIZE];

    int32_t writePos = 0;

    // Slow random wow drift.
    int32_t wowL = 0;
    int32_t wowR = 0;

    // Flutter oscillators.
    uint32_t flutterPhaseL = 0;
    uint32_t flutterPhaseR = 16384;

    // Random generator state.
    uint32_t rng = 1;

    // Tone filter memory.
    int32_t toneL = 0;
    int32_t toneR = 0;

    //
    // Additional DSP state.
    //

    int32_t crackleLPFL = 0;
    int32_t crackleLPFR = 0;

    int32_t hissLPFL = 0;
    int32_t hissLPFR = 0;
    
    // Cheap xorshift RNG.
    inline uint32_t FastRandom()
    {
        rng ^= rng << 13;
        rng ^= rng >> 17;
        rng ^= rng << 5;
        return rng;
    }

    // Triangle oscillator.
    inline int32_t Triangle(uint32_t phase)
    {
        int32_t x = (phase >> 20) & 2047;

        if (x > 1023)
        {
            x = 2047 - x;
        }

        return (x << 1) - 1023;
    }

    // Clamp helper.
    inline int32_t Clamp2048(int32_t x)
    {
        if (x > 2047) return 2047;
        if (x < -2048) return -2048;
        return x;
    }

    // Cheap tape-style soft saturation.
    //
    // Adds compression-like behaviour
    // without expensive nonlinear DSP.
    inline int32_t SoftClip(int32_t x)
    {
        x = x + (x >> 1);

        int32_t ax = (x < 0) ? -x : x;

        x -= (x * (ax >> 5)) >> 6;

        return Clamp2048(x);
    }

    // Linear interpolated delay read.
    //
    // Fractional precision: 8 bits.
    inline int32_t ReadDelayInterpolated(int32_t readPos)
    {
        int32_t indexA = (readPos >> 8) & BUFFER_MASK;
        int32_t indexB = (indexA + 1) & BUFFER_MASK;

        int32_t frac = readPos & 255;

        int32_t a = delayBuffer[indexA];
        int32_t b = delayBuffer[indexB];

        return a + (((b - a) * frac) >> 8);
    }

    virtual void ProcessSample() override
    {
        // Controls.
        int32_t mix = KnobVal(Knob::Main);
        int32_t depth = KnobVal(Knob::X);
        int32_t instability = KnobVal(Knob::Y);

        // Stereo input handling.
        //
        // If Audio2 disconnected:
        // mono input duplicated internally.
        int32_t inputL = AudioIn1();
        int32_t inputR = Connected(Input::Audio2)
            ? AudioIn2()
            : inputL;

        // Tape saturation.
        inputL = SoftClip(inputL);
        inputR = SoftClip(inputR);

        // Store summed mono signal into tape path.
        //
        // Tape machines naturally collapse
        // some stereo width internally.
        int32_t tapeInput = (inputL + inputR) >> 1;

        delayBuffer[writePos] = tapeInput;

        // External transport modulation.
        int32_t externalMod = 0;

        if (Connected(Input::CV2))
        {
            externalMod = CVIn2() >> 2;
        }

        // Random wow drift.
        //
        // Updated slowly for smooth motion.
        if ((writePos & 63) == 0)
        {
            wowL += ((int32_t)(FastRandom() & 31) - 15);
            wowR += ((int32_t)(FastRandom() & 31) - 15);

            wowL -= wowL >> 7;
            wowR -= wowR >> 7;
        }

        // Flutter oscillator speed.
        flutterPhaseL += 4000 + (instability << 1);
        flutterPhaseR += 4300 + (instability << 1);

        int32_t flutterL = Triangle(flutterPhaseL);
        int32_t flutterR = Triangle(flutterPhaseR);

        // Tape condition settings.
        int32_t toneShift = 5;

        bool oldMode = false;
        bool damagedMode = false;

        // Nonlinear modulation depth scaling.
        //
        // Gives finer control at low settings.
        int32_t modeDepth = (depth * depth) >> 12;

        switch (SwitchVal())
        {
            case Switch::Up:
                toneShift = 4;
                break;

            case Switch::Middle:
                toneShift = 5;
                modeDepth += 200;
                oldMode = true;
                break;

            case Switch::Down:
                toneShift = 6;
                modeDepth += 500;
                damagedMode = true;
                break;
        }

        // Combined modulation.
        int32_t modL = wowL + (flutterL << 2);
        int32_t modR = wowR + (flutterR << 2);

        modL += externalMod;
        modR -= externalMod;

        modL = (modL * modeDepth) >> 12;
        modR = (modR * modeDepth) >> 12;

        // Base tape delay.
        //
        // Stored in 8-bit fractional format.
        int32_t baseDelay = 1400 << 8;

        // Fractional modulated read positions.
        int32_t readL =
            ((writePos << 8) - baseDelay - (modL << 3));

        int32_t readR =
            ((writePos << 8) - baseDelay - (modR << 3));

        // Interpolated reads.
        int32_t wetL = ReadDelayInterpolated(readL);
        int32_t wetR = ReadDelayInterpolated(readR);

        // Tone shaping.
        //
        // Older tape becomes darker.
        toneL += (wetL - toneL) >> toneShift;
        toneR += (wetR - toneR) >> toneShift;

        wetL = toneL;
        wetR = toneR;

        // Old cassette mode:
        // subtle filtered hiss.
        if (oldMode)
        {
            // Generate white noise.
            int32_t noiseL =
                ((int32_t)(FastRandom() & 127) - 64);

            int32_t noiseR =
                ((int32_t)(FastRandom() & 127) - 64);

            // Low-pass memory for high-pass filter.
            //
            // Removes low frequencies from hiss
            // so noise feels more tape-like.
            hissLPFL += (noiseL - hissLPFL) >> 4;
            hissLPFR += (noiseR - hissLPFR) >> 4;

            // High-passed hiss.
            int32_t hissL = (noiseL - hissLPFL) >> 2;
            int32_t hissR = (noiseR - hissLPFR) >> 2;

            wetL += hissL;
            wetR += hissR;
        }

        // Damaged cassette mode:
        // louder hiss + filtered crackle.
        if (damagedMode)
        {
            // Brighter broadband hiss.
            int32_t noiseL =
                ((int32_t)(FastRandom() & 255) - 128);

            int32_t noiseR =
                ((int32_t)(FastRandom() & 255) - 128);

            // High-pass shaping for realistic tape hiss.
            hissLPFL += (noiseL - hissLPFL) >> 4;
            hissLPFR += (noiseR - hissLPFR) >> 4;

            int32_t hissL = (noiseL - hissLPFL) >> 1;
            int32_t hissR = (noiseR - hissLPFR) >> 1;

            wetL += hissL;
            wetR += hissR;

            // Crackle excitation impulses.
            //
            // Instead of single-sample digital clicks,
            // we excite short decaying bursts.
            if ((FastRandom() & 2047) == 0)
            {
                crackleLPFL +=
                    (((int32_t)(FastRandom() & 255) - 128) << 2);
            }

            if ((FastRandom() & 2047) == 0)
            {
                crackleLPFR +=
                    (((int32_t)(FastRandom() & 255) - 128) << 2);
            }

            // Low-pass decay filter.
            //
            // Produces softer analog-style crackle.
            crackleLPFL -= crackleLPFL >> 4;
            crackleLPFR -= crackleLPFR >> 4;

            wetL += crackleLPFL >> 1;
            wetR += crackleLPFR >> 1;
        }

        // Protect wet path before mixing.
        wetL = Clamp2048(wetL);
        wetR = Clamp2048(wetR);

        // Wet/dry mix.
        int32_t dryGain = 4095 - mix;

        int32_t outL =
            ((inputL * dryGain) + (wetL * mix)) >> 12;

        int32_t outR =
            ((inputR * dryGain) + (wetR * mix)) >> 12;

        // Final output protection.
        outL = Clamp2048(outL);
        outR = Clamp2048(outR);

        AudioOut1(outL);
        AudioOut2(outR);

        // LED feedback.
        //
        // 0 = left modulation
        // 1 = instability
        // 2 = right modulation
        // 3 = mix
        // 4 = depth
        LedBrightness(0, (modL + 2048) & 4095);
        LedBrightness(1, instability);
        LedBrightness(2, (modR + 2048) & 4095);
        LedBrightness(3, mix);
        LedBrightness(4, depth);

        // Advance circular buffer.
        writePos++;
        writePos &= BUFFER_MASK;
    }
};

int main()
{
    // 144MHz is stable and reduces
    // some converter artefacts.
    set_sys_clock_khz(144000, true);

    Tapegrade tapegrade;

    // Required for jack detection.
    tapegrade.EnableNormalisationProbe();

    tapegrade.Run();
}
```
