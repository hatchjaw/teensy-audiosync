/* Audio Library for Teensy 3.X
* Copyright (c) 2014, Paul Stoffregen, paul@pjrc.com
 *
 * Development of this audio library was funded by PJRC.COM, LLC by sales of
 * Teensy and Audio Adaptor boards.  Please support PJRC's efforts to develop
 * open source software by purchasing Teensy or other PJRC products.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice, development funding notice, and this permission
 * notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef SGTL5000_H
#define SGTL5000_H

#include <AudioControl.h>

// SGTL5000-specific defines for headphones
#define AUDIO_HEADPHONE_DAC 0
#define AUDIO_HEADPHONE_LINEIN 1
/**
* Lifted from Audio/control_sgtl5000.h
*/
class SGTL5000 : public AudioControl
{
public:
    SGTL5000() : i2c_addr(0x0A)
    {
    }

    void setAddress(uint8_t level);

    void begin();
    bool enable() override;
    bool disable() override { return false; }
    bool volume(const float n) override { return volumeInteger(n * 129 + 0.499f); }
    bool inputLevel(float n) override { return false; }
    bool muteHeadphone() { return write(0x0024, ana_ctrl | (1 << 4)); }
    bool unmuteHeadphone() { return write(0x0024, ana_ctrl & ~(1 << 4)); }
    bool muteLineout() { return write(0x0024, ana_ctrl | (1 << 8)); }
    bool unmuteLineout() { return write(0x0024, ana_ctrl & ~(1 << 8)); }

    bool inputSelect(const int n) override
    {
        if (n == AUDIO_INPUT_LINEIN) {
            return write(0x0020, 0x055) // +7.5dB gain (1.3Vp-p full scale)
                   && write(0x0024, ana_ctrl | (1 << 2)); // enable linein
        } else if (n == AUDIO_INPUT_MIC) {
            return write(0x002A, 0x0173) // mic preamp gain = +40dB
                   && write(0x0020, 0x088) // input gain +12dB (is this enough?)
                   && write(0x0024, ana_ctrl & ~(1 << 2)); // enable mic
        } else {
            return false;
        }
    }

    bool headphoneSelect(int n)
    {
        if (n == AUDIO_HEADPHONE_DAC) {
            return write(0x0024, ana_ctrl | (1 << 6)); // route DAC to headphones out
        } else if (n == AUDIO_HEADPHONE_LINEIN) {
            return write(0x0024, ana_ctrl & ~(1 << 6)); // route linein to headphones out
        } else {
            return false;
        }
    }

    bool volume(float left, float right);

    bool micGain(unsigned int dB);

    bool lineInLevel(uint8_t n) { return lineInLevel(n, n); }

    bool lineInLevel(uint8_t left, uint8_t right);

    unsigned short lineOutLevel(uint8_t n);

    unsigned short lineOutLevel(uint8_t left, uint8_t right);

    unsigned short dacVolume(float n);

    unsigned short dacVolume(float left, float right);

    bool dacVolumeRamp();

    bool dacVolumeRampLinear();

    bool dacVolumeRampDisable();

    unsigned short adcHighPassFilterEnable();

    unsigned short adcHighPassFilterFreeze();

    unsigned short adcHighPassFilterDisable();

    unsigned short audioPreProcessorEnable();

    unsigned short audioPostProcessorEnable();

    unsigned short audioProcessorDisable();

    unsigned short eqFilterCount(uint8_t n);

    unsigned short eqSelect(uint8_t n);

    unsigned short eqBand(uint8_t bandNum, float n);

    void eqBands(float bass, float mid_bass, float midrange, float mid_treble, float treble);

    void eqBands(float bass, float treble);

    void eqFilter(uint8_t filterNum, int *filterParameters);

    unsigned short autoVolumeControl(uint8_t maxGain, uint8_t lbiResponse, uint8_t hardLimit, float threshold, float attack, float decay);

    unsigned short autoVolumeEnable();

    unsigned short autoVolumeDisable();

    unsigned short enhanceBass(float lr_lev, float bass_lev);

    unsigned short enhanceBass(float lr_lev, float bass_lev, uint8_t hpf_bypass, uint8_t cutoff);

    unsigned short enhanceBassEnable();

    unsigned short enhanceBassDisable();

    unsigned short surroundSound(uint8_t width);

    unsigned short surroundSound(uint8_t width, uint8_t select);

    unsigned short surroundSoundEnable();

    unsigned short surroundSoundDisable();

    void killAutomation() { semi_automated = false; }

    void setMasterMode(uint32_t freqMCLK_in);

    uint32_t cycStart{0},
    cycPostBegin{0},
    cycPostInit{0},
    cycPostAnPwr{0},
    cycPostDgPwr{0},
    cycPostLnOut{0},
    cycPostClock{0},
    cycPostSetup{0};

protected:
    bool muted;

    bool volumeInteger(unsigned int n); // range: 0x00 to 0x80
    uint16_t ana_ctrl;
    uint8_t i2c_addr;

    unsigned char calcVol(float n, unsigned char range);

    unsigned int read(unsigned int reg) const;

    bool write(unsigned int reg, unsigned int val);

    unsigned int modify(unsigned int reg, unsigned int val, unsigned int iMask);

    unsigned short dap_audio_eq_band(uint8_t bandNum, float n);

private:
    bool semi_automated;

    void automate(uint8_t dap, uint8_t eq);

    void automate(uint8_t dap, uint8_t eq, uint8_t filterCount);
};

//For Filter Type: 0 = LPF, 1 = HPF, 2 = BPF, 3 = NOTCH, 4 = PeakingEQ, 5 = LowShelf, 6 = HighShelf
#define FILTER_LOPASS 0
#define FILTER_HIPASS 1
#define FILTER_BANDPASS 2
#define FILTER_NOTCH 3
#define FILTER_PARAEQ 4
#define FILTER_LOSHELF 5
#define FILTER_HISHELF 6

//For frequency adjustment
#define FLAT_FREQUENCY 0
#define PARAMETRIC_EQUALIZER 1
#define TONE_CONTROLS 2
#define GRAPHIC_EQUALIZER 3


void calcBiquad(uint8_t filtertype, float fC, float dB_Gain, float Q, uint32_t quantization_unit, uint32_t fS, int *coef);


#endif //SGTL5000_H
