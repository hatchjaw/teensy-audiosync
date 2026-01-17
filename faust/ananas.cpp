#include "ananas.h"
#include <AnanasUtils.h>

#include <string.h> // for memset

// IMPORTANT: in order for MapUI to work, the teensy linker must be g++
#include "faust/gui/MapUI.h"
#include "faust/gui/meta.h"
#include "faust/dsp/dsp.h"

// MIDI support
#if MIDICTRL
#include "faust/gui/MidiUI.h"
#include "faust/midi/teensy-midi.h"
#endif

// for polyphonic synths
#ifdef NVOICES
#include "faust/dsp/poly-dsp.h"
#endif

// we require macro declarations
#define FAUST_UIMACROS

// but we will ignore most of them
#define FAUST_ADDBUTTON(l,f)
#define FAUST_ADDCHECKBOX(l,f)
#define FAUST_ADDVERTICALSLIDER(l,f,i,a,b,s)
#define FAUST_ADDHORIZONTALSLIDER(l,f,i,a,b,s)
#define FAUST_ADDNUMENTRY(l,f,i,a,b,s)
#define FAUST_ADDVERTICALBARGRAPH(l,f,a,b)
#define FAUST_ADDHORIZONTALBARGRAPH(l,f,a,b)

/******************************************************************************
 *******************************************************************************

 VECTOR INTRINSICS

 *******************************************************************************
 *******************************************************************************/

<<includeIntrinsic>>

/********************END ARCHITECTURE SECTION (part 1/2)****************/

/**************************BEGIN USER SECTION **************************/

<<includeclass>>

/***************************END USER SECTION ***************************/

/*******************BEGIN ARCHITECTURE SECTION (part 2/2)***************/

#define MULT_16 32767
#define DIV_16 0.0000305185

#if MIDICTRL
std::list<GUI*> GUI::fGuiList;
ztimedmap GUI::gTimedZoneMap;
#endif

AudioFaust::AudioFaust()
{
#ifdef NVOICES
    int nvoices = NVOICES;
    fDSP = new mydsp_poly(new mydsp(), nvoices, true, true);
#else
    fDSP = new mydsp();
#endif

    // allocating Faust inputs
    if (fDSP->getNumInputs() > 0) {
        fInChannel = new float *[fDSP->getNumInputs()];
    } else {
        fInChannel = nullptr;
    }

    // allocating Faust outputs
    if (fDSP->getNumOutputs() > 0) {
        fOutChannel = new float *[fDSP->getNumOutputs()];
    } else {
        fOutChannel = nullptr;
    }

    fUI = new MapUI();

#if MIDICTRL
    fMIDIHandler = new ananas_midi();
    fMIDIInterface = new MidiUI(fMIDIHandler);
#endif
}

AudioFaust::~AudioFaust()
{
    delete fDSP;
    delete fUI;
    for (int i = 0; i < fDSP->getNumInputs(); i++) {
        delete[] fInChannel[i];
    }
    delete [] fInChannel;
    for (int i = 0; i < fDSP->getNumOutputs(); i++) {
        delete[] fOutChannel[i];
    }
    delete [] fOutChannel;
#if MIDICTRL
    delete fMIDIInterface;
    delete fMIDIHandler;
#endif
}

void AudioFaust::prepare(const uint sampleRate)
{
    fDSP->init(static_cast<int>(sampleRate));
    fDSP->buildUserInterface(fUI);

    // allocating Faust inputs
    if (fDSP->getNumInputs() > 0) {
        for (int i = 0; i < fDSP->getNumInputs(); i++) {
            fInChannel[i] = new float[ananas::Constants::AudioBlockFrames];
        }
    }

    // allocating Faust outputs
    if (fDSP->getNumOutputs() > 0) {
        for (int i = 0; i < fDSP->getNumOutputs(); i++) {
            fOutChannel[i] = new float[ananas::Constants::AudioBlockFrames];
        }
    }

#if MIDICTRL
    fDSP->buildUserInterface(fMIDIInterface);
    fMIDIInterface->run();
#endif
}

void AudioFaust::processImpl(int16_t **inputBuffer, int16_t **outputBuffer, size_t numFrames)
{
    if (getNumInputs() > 0) {
        for (size_t channel{0}; channel < getNumInputs(); channel++) {
            for (size_t i{0}; i < numFrames; i++) {
                int16_t val = inputBuffer[channel][i];
                fInChannel[channel][i] = val * DIV_16;
            }
        }
    }

    fDSP->compute(numFrames, fInChannel, fOutChannel);

    for (size_t channel{0}; channel < getNumOutputs(); channel++) {
        for (size_t i{0}; i < numFrames; i++) {
            int16_t val = fOutChannel[channel][i] * MULT_16;
            outputBuffer[channel][i] = val;
        }
    }
}

void AudioFaust::setParamValue(const std::string &path, float value) const
{
    fUI->setParamValue(path, value);
}

float AudioFaust::getParamValue(const std::string &path) const
{
    return fUI->getParamValue(path);
}

size_t AudioFaust::printTo(Print &p) const
{
    return p.print("WFS:               ") + AudioProcessor::printTo(p);
}

size_t AudioFaust::getNumInputs() const
{
    return fDSP->getNumInputs();
}

size_t AudioFaust::getNumOutputs() const
{
    return fDSP->getNumOutputs();
}

/********************END ARCHITECTURE SECTION (part 2/2)****************/
