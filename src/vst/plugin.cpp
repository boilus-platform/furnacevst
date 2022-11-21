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

struct furnacedata {
	furnacedata() {
		data.resize(2 * sizeof(float));
		*getParaInBlob(0) = 0.0f;
		*getParaInBlob(1) = 0.0f;
	}

	float* getParaInBlob(int index) {
		return ((float*)data.data()) + index;
	}

	size_t getFurnaceBlobSize() {
		return data.size() - 2 * sizeof(float);
	}

	unsigned char* getFurnaceBlobPtr() {
		return data.data() + 2 * sizeof(float);
	}

	void saveFurnaceBlob(void* ptr, size_t size) {
		data.resize(2 * sizeof(float) + size);
		memcpy(getFurnaceBlobPtr(), ptr, size);
	}
	std::vector<unsigned char> data;
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
	int n_inst = 0;
	bool newproj = true;
	int ins_num = -1;
	bool psync = false;
	bool blobloaded = false;
	furnacedata chunk;
};

ADevice global;

void saveFurnaceBlob(void* ptr, size_t size) {
	global.chunk.saveFurnaceBlob(ptr, size);
}

bool projectSyncEnabled() {
	return global.psync;
}

bool isBlobLoaded() {
	int val = global.blobloaded;
	global.blobloaded = true;
	return val;
}

Blob getBlob() {
	Blob b;
	b.blob = new unsigned char[global.chunk.getFurnaceBlobSize()];
	b.size = global.chunk.getFurnaceBlobSize();
	memcpy(b.blob, global.chunk.getFurnaceBlobPtr(), global.chunk.getFurnaceBlobSize());
	return b;
}

bool isNewProject() {
	bool cval = global.newproj;
	global.newproj = false;
	return cval;
}

void setProjectSync(bool val) {
	global.hack->setParameterAutomated(0, val ? 1.0f : 0.0f);
}


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

class FurnaceVST : public AudioEffectX {
public:
	FurnaceVST(audioMasterCallback audioMaster) :
		AudioEffectX(audioMaster, 1, 2) {
		this->getAeffect()->flags |= effFlagsHasEditor;
		if (audioMaster) {
			setNumInputs(0);
			setNumOutputs(2);
			canProcessReplacing();
			isSynth();
			//setEditor(&e);
			setUniqueID('frnc');
			noTail(false);
			programsAreChunks();
		}
		
		suspend();
		global.blocksize = getBlockSize();
		global.hack = this;
	}

	void getProgramName(char* name) {
		strcpy(name, "default");
	}

	void getParameterDisplay(VstInt32 index, char* text) {
		switch (index) {
		case 0:
			if (global.psync)
				memcpy(text, "On", 3);
			else
				memcpy(text, "Off", 4);
			break;
		case 1:
			std::string val = std::to_string(global.ins_num).c_str();
			memcpy(text, val.c_str(), val.size());
			break;
		}
	}

	bool getParameterProperties(VstInt32 index, VstParameterProperties* p) {
		const char* names[] = { "Project Sync", "Instrument Num" };
		memcpy(p->label, names[index], strlen(names[index]) + 1);
		return true;
	}

	float getParameter(VstInt32 index) {
		switch (index) {
		case 0:
			return global.psync ? 1.0f : 0.0f;
		case 1:
			if (global.ins_num > 0)
				return global.ins_num;
			else
				return 0;
		}
	}

	void setParameter(VstInt32 index, float value) {
		switch (index) {
		case 0:
			global.newproj = false;
			if (value > 0.5f)
				global.psync = true;
			else
				global.psync = false;
			break;
		case 1:
			float eps = 0.0001f;
			global.ins_num = fmin((int)roundf(1.0f / ((1.0f-value)+eps)), global.n_inst) - 1;
			break;
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

	VstInt32 getChunk(void** data, bool isPreset) {
		*global.chunk.getParaInBlob(0) = getParameter(0);
		*global.chunk.getParaInBlob(1) = getParameter(1);
		*data = global.chunk.data.data();
		return global.chunk.data.size();
	}

	VstInt32 setChunk(void* data, VstInt32 byteSize, bool isPreset) {
		if (byteSize > 7) {
			global.chunk.data.resize(byteSize);
			memcpy(global.chunk.data.data(), data, byteSize);
			setParameter(0, *global.chunk.getParaInBlob(0));
			setParameter(1, *global.chunk.getParaInBlob(1));
		}
		return 0;
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

private:
	int hide = 0;
	HWND host_win = 0;
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
	return new FurnaceVST(audioMaster);
}


#endif