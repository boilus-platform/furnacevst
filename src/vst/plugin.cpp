#include "../vst/audioeffectx.h"
#include <stdio.h>
#include <cmath>
#include <array>
#include "pluginhook.h"
#include <queue>
#include <mutex>
#include <vector>
#include <vector>


#if defined(_WIN32) || defined(_WIN64)
  #define VST_EXPORT  extern "C" __declspec(dllexport)
#else
  #define VST_EXPORT extern
#endif
#ifndef __vstxsynth__
#define __vstxsynth__

enum {
	// Global
	kNumPrograms = 128,
	kNumOutputs = 2,

	// Parameters Tags
	kWaveform1 = 0,
	kFreq1,
	kVolume1,

	kWaveform2,
	kFreq2,
	kVolume2,

	kVolume,

	kNumParams
};

struct VoiceHandler {
	int key = 0;
	int active = false;
	int samples = 0;
	int velocity = 0;
};

struct ADevice {
	int ready = 0;
	int sr = 0;
	int blocksize = 0;
	void* userptr = 0;
	void (*callback)(void*, float**, int) = 0;
	int ultraready = 0;
	int suspended = 1;
	std::queue<std::vector<unsigned char>> midiboys;
	AudioEffectX* hack = NULL;
	double time = 0.0f;
};

ADevice global;

int isAudioHookReady(void) {
	return global.ready;
}

double GetVstMidiBoy(std::vector<unsigned char>* msg) {
	std::mutex m;
	m.lock();
	if (global.midiboys.size()) {
		*msg = global.midiboys.front();
		global.midiboys.pop();
	}
	else {
		*msg = std::vector<unsigned char>();
	}
	m.unlock();
	global.time += 1.0f;
	return global.time;
}

void SetAudioHookReallyReady(void) {
	global.ultraready = 1;
}

void VSTAudioHookInit(TAAudioDesc& ar, void (*callback)(void*, float**, int), void* userdata) {
	global.callback = (void (*)(void *, float **, int))callback;
	global.userptr = userdata;
	ar.outChans = 2;
	ar.rate = global.sr;
	ar.bufsize = global.blocksize;
}

void suspend2(bool run) {
	global.suspended = run;
	if (run)
		global.hack->resume();
	//else
		//global.hack->suspend();

}

class MatrixSynth : public AudioEffectX {
public:
	MatrixSynth(audioMasterCallback audioMaster) :
		AudioEffectX(audioMaster, 0, 0) {
		if (audioMaster) {
			setNumInputs(0);
			setNumOutputs(2);
			canProcessReplacing();
			isSynth();
			setUniqueID('frnc');
		}
		
		suspend();
		global.blocksize = getBlockSize();
		global.hack = this;
	}

	virtual void setSampleRate(float sampleRate) {
		global.sr = sampleRate;
		global.ready = 1;
	}

	virtual void resume() {
		__wantEventsDeprecated();
	}

	void processReplacing(float** inputs, float** outputs, VstInt32 sampleFrames) {
		float* outl = outputs[0];
		float* outr = outputs[1];
		float pi = 3.14159f;
		float sr = getSampleRate();
		memset(outr, 0, sampleFrames * sizeof(float));
		memset(outl, 0, sampleFrames * sizeof(float));
		if (global.ultraready && global.suspended) {
			global.callback(global.userptr, outputs, sampleFrames);
		}
	}

	VoiceHandler* FindFirstVoiceAvailable(VstInt32 note) {
		for (VoiceHandler& v : voices)
			if (v.key == note)
				return &v;
		for (VoiceHandler& v : voices)
			if (!v.active)
				return &v;
		return nullptr;
	}

	void noteOn(VstInt32 note, VstInt32 velocity, VstInt32 delta) {
		VoiceHandler* v = FindFirstVoiceAvailable(note);
		if (!v)
			return;
		v->key = note;
		v->active = true;
		v->velocity = velocity;
	}

	VstInt32 processEvents(VstEvents* ev) {
		for (VstInt32 i = 0; i < ev->numEvents; i++) {
			if ((ev->events[i])->type != kVstMidiType)
				continue;
			VstMidiEvent* event = (VstMidiEvent*)ev->events[i];
			char *midiData = event->midiData;
			std::vector<unsigned char> myboy;
			for (int i = 0; i < 3; i++)
				myboy.push_back(event->midiData[i]);
			global.midiboys.push(myboy);
		} 
		return 1;
	}

	void noteOff(VstInt32 note) {
		VoiceHandler* v = FindFirstVoiceAvailable(note);
		v->active = false;
		v->samples = 0;
	}

private:
	VoiceHandler voices[32];
	const float components[10] = {
		0.55f, 0.45f, 0.375f, 0.3f, 
		0.0f, 0.18f, 0.00f, 0.0f, 
		0.00f, 0.0f
	};
	int samples = 0;
	VstInt32 cnote = 0;
	bool noteIsOn = false;
};

AudioEffect* createEffectInstance(audioMasterCallback audioMaster)
{
	return new MatrixSynth(audioMaster);
}


#endif