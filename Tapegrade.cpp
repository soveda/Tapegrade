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
// This is an intentionally simple first prototype.
// The DSP focuses on:
//
// - short modulated delay
// - wow/flutter movement
// - stereo width
// - musical instability
// - low CPU usage
//
// The code prioritises clarity and safety inside ProcessSample().

class Tapegrade : public ComputerCard
{
public:

    // Delay buffer.
    //
    // 4096 samples gives roughly 85ms at 48kHz.
    // This is enough for tape-style vibrato movement
    // without becoming a noticeable echo.
    int16_t delayBuffer[4096];

    int32_t writePos = 0;

    // Slow wow movement.
    // These values drift gradually over time.
    int32_t wowL = 0;
    int32_t wowR = 0;

    // Flutter phase accumulators.
    int32_t flutterPhaseL = 0;
    int32_t flutterPhaseR = 16384;

    // Small random generator state.
    uint32_t rng = 1;

    // Simple one-pole tone filter state.
    int32_t toneL = 0;
    int32_t toneR = 0;

    // Generate very cheap pseudo-random numbers.
    //
    // We avoid expensive random libraries inside
    // the audio interrupt.
    inline uint32_t FastRandom()
    {
        rng ^= rng << 13;
        rng ^= rng >> 17;
        rng ^= rng << 5;
        return rng;
    }

    // Cheap triangle oscillator.
    //
    // Used for flutter movement.
    // Triangle waves sound slightly more mechanical
    // than pure sine waves.
    inline int32_t Triangle(int32_t phase)
    {
        int32_t x = (phase >> 20) & 2047;

        if (x > 1023)
        {
            x = 2047 - x;
        }

        return (x - 512);
    }

    virtual void ProcessSample()
    {
        // Read controls.
        int32_t mix = KnobVal(Knob::Main);
        int32_t depth = KnobVal(Knob::X);
        int32_t instability = KnobVal(Knob::Y);

        // Audio input.
        int32_t input = AudioIn1();

        // Tape-style soft saturation.
        //
        // This gently rounds loud peaks and adds
        // slight compression behaviour.
        input = input + (input >> 1);

        if (input > 2047)
        {
            input = 2047;
        }

        if (input < -2048)
        {
            input = -2048;
        }

        // Store into delay buffer.
        delayBuffer[writePos] = input;

        // External instability input.
        //
        // If CV2 is patched we use it to influence
        // tape transport movement.
        int32_t externalMod = 0;

        if (Connected(Input::CV2))
        {
            externalMod = CVIn2() >> 2;
        }

        // Random wow drift.
        //
        // We only update occasionally to keep
        // movement smooth and slow.
        if ((writePos & 63) == 0)
        {
            wowL += ((int32_t)(FastRandom() & 31) - 15);
            wowR += ((int32_t)(FastRandom() & 31) - 15);

            // Pull wow back toward centre.
            wowL -= wowL >> 7;
            wowR -= wowR >> 7;
        }

        // Flutter oscillator speed.
        //
        // Higher instability increases the apparent
        // transport instability.
        flutterPhaseL += 20000 + (instability << 2);
        flutterPhaseR += 20500 + (instability << 2);

        int32_t flutterL = Triangle(flutterPhaseL);
        int32_t flutterR = Triangle(flutterPhaseR);

        // Tape condition switch.
        //
        // Different switch positions alter:
        // - darkness
        // - instability range
        // - stereo movement
        // - hiss and crackle behaviour
        int32_t toneShift = 5;
        int32_t modeDepth = depth;

        bool oldMode = false;
        bool damagedMode = false;

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

        // Final stereo modulation amounts.
        int32_t modL = wowL + (flutterL << 2);
        int32_t modR = wowR + (flutterR << 2);

        modL += externalMod;
        modR -= externalMod;

        modL = (modL * modeDepth) >> 12;
        modR = (modR * modeDepth) >> 12;

        // Delay tap positions.
        //
        // Left and right channels use slightly different
        // read positions to create stereo width.
        int32_t baseDelay = 1400;

        int32_t readL = writePos - baseDelay - (modL >> 5);
        int32_t readR = writePos - baseDelay - (modR >> 5);

        readL &= 4095;
        readR &= 4095;

        int32_t wetL = delayBuffer[readL];
        int32_t wetR = delayBuffer[readR];

        // Simple tone filtering.
        //
        // Older tape modes become darker.
        toneL += (wetL - toneL) >> toneShift;
        toneR += (wetR - toneR) >> toneShift;

        wetL = toneL;
        wetR = toneR;

        // Older tape modes add noise and degradation.
        //
        // The goal is not perfect tape emulation.
        // We want the feeling of ageing media,
        // dirty playback paths, and worn recordings.

        // Old cassette mode adds subtle hiss.
        //
        // This should feel soft and nostalgic rather
        // than aggressively noisy.
        if (oldMode)
        {
            int32_t hissL = ((int32_t)(FastRandom() & 127) - 64) >> 3;
            int32_t hissR = ((int32_t)(FastRandom() & 127) - 64) >> 3;

            wetL += hissL;
            wetR += hissR;
        }

        // Damaged mode increases hiss and adds crackle.
        //
        // This mode should sound unstable, dirty,
        // and partially broken.
        if (damagedMode)
        {
            // Louder broadband hiss.
            int32_t hissL = ((int32_t)(FastRandom() & 255) - 128) >> 2;
            int32_t hissR = ((int32_t)(FastRandom() & 255) - 128) >> 2;

            wetL += hissL;
            wetR += hissR;

            // Occasional crackle bursts.
            //
            // Rare random spikes imitate damaged tape,
            // oxide loss, or dirty tape heads.
            if ((FastRandom() & 2047) == 0)
            {
                wetL += ((int32_t)(FastRandom() & 1023) - 512);
            }

            if ((FastRandom() & 2047) == 0)
            {
                wetR += ((int32_t)(FastRandom() & 1023) - 512);
            }
        }

        // Wet/dry mix.
        //
        // Main knob crossfades between the original
        // signal and the processed cassette signal.
        int32_t dryGain = 4095 - mix;

        int32_t outL = ((input * dryGain) + (wetL * mix)) >> 12;
        int32_t outR = ((input * dryGain) + (wetR * mix)) >> 12;

        // Output protection.
        if (outL > 2047) outL = 2047;
        if (outL < -2048) outL = -2048;

        if (outR > 2047) outR = 2047;
        if (outR < -2048) outR = -2048;

        AudioOut1(outL);
        AudioOut2(outR);

        // LED feedback.
        //
        // Left LEDs display modulation movement.
        // Right LEDs display instability amount.
        LedBrightness(0, (modL + 2048) & 4095);
        LedBrightness(2, (modR + 2048) & 4095);

        LedBrightness(1, instability);
        LedBrightness(3, depth);
        LedBrightness(5, mix);

        // Advance circular buffer.
        writePos++;
        writePos &= 4095;
    }
};

int main()
{
    // 144MHz reduces some ADC artefacts while
    // remaining very stable on Workshop Computer.
    set_sys_clock_khz(144000, true);

    Tapegrade tapegrade;

    // Enable jack detection for the transport
    // instability input.
    tapegrade.EnableNormalisationProbe();

    tapegrade.Run();
}
