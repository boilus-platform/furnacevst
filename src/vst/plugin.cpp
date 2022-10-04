#include "../vst/audioeffectx.h"
#include <stdio.h>
#include <cmath>
#include <array>
#include "pluginhook.h"
#include <queue>
#include <mutex>
#include <vector>
#include "../vst/aeffeditor.h"
#include <vector>
#include <thread>
#if defined(_WIN32) || defined(_WIN64)
#include <Windows.h>
#endif


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
	SDL_Window* w = NULL;
};



ADevice global;

void HandOverSDLWindow(SDL_Window* w) {
	global.w = w;
}

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
	else
		global.hack->suspend();

}

void HideItPleaseGod(HWND tohide) {
	std::this_thread::sleep_for(std::chrono::milliseconds(250));
	ShowWindow((HWND)tohide, 0);
	if (global.w) {
		SDL_ShowWindow(global.w);
		SDL_RaiseWindow(global.w);
		SDL_SetWindowAlwaysOnTop(global.w, SDL_TRUE);
	}
}

class MatrixSynth : public AudioEffectX {
public:
	MatrixSynth(audioMasterCallback audioMaster) :
		AudioEffectX(audioMaster, 0, 2) {
		this->getAeffect()->flags |= effFlagsHasEditor;
		if (audioMaster) {
			setNumInputs(0);
			setNumOutputs(2);
			canProcessReplacing();
			isSynth();
			//setEditor(&e);
			setUniqueID('frnc');
			noTail(false);
		}
		
		suspend();
		global.blocksize = getBlockSize();
		global.hack = this;
	}

	void getParameterDisplay(VstInt32 index, char* text) {
		switch (index) {
		case 0:
			if (psync)
				memcpy(text, "True", 5);
			else
				memcpy(text, "False", 6);
			break;
		case 1:
			std::string val = std::to_string(ins_num).c_str();
			memcpy(text, val.c_str(), val.size());
			break;
		}
	}

	bool getParameterProperties(VstInt32 index, VstParameterProperties* p) {
		const char* names[] = { "Project Sync", "Instrument Num" };
		memcpy(p->label, names[index], strlen(names[index]) + 1);
		return true;
	}

	float getParameter(VstInt32 index, float value) {
		switch (index) {
		case 0:
			return psync ? 1.0f : 0.0f;
		case 1:
			if (ins_num > 0)
				return ins_num;
			else
				return 0;
		}
	}

	virtual VstInt32 getGetTailSize() {
		return 1;
	}

	virtual void setSampleRate(float sampleRate) {
		global.sr = sampleRate;
		global.ready = 1;
	}

	virtual VstIntPtr dispatcher(VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt) {
		switch (opcode) {
		case effEditOpen:
			hide = 1;
#if defined(_WIN32) || defined(_WIN64)
			host_win = (HWND)ptr;
#endif
			if(global.w)
				SDL_ShowWindow(global.w);
			break;
		case effEditClose:
			if (global.w) {
				SDL_HideWindow(global.w);
				SDL_SetWindowAlwaysOnTop(global.w, SDL_FALSE);
			}
			break;
		}
		return AudioEffectX::dispatcher(opcode, index, value, ptr, opt);
	}

	void processReplacing(float** inputs, float** outputs, VstInt32 sampleFrames) {
		float* outl = outputs[0];
		float* outr = outputs[1];
		float pi = 3.14159f;
		float sr = getSampleRate();
		memset(outr, 0, sampleFrames * sizeof(float));
		memset(outl, 0, sampleFrames * sizeof(float));
#if defined(_WIN32) || defined(_WIN64)
		if (hide) {
			hide = 0;
			std::thread t(HideItPleaseGod, host_win);
			t.detach();
		}
#endif
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
	int ins_num = -1;
	bool psync = false;
	int hide = 0;
	HWND host_win = 0;
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