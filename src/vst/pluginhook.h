#pragma once
#include <vector>
#include "../audio/taAudio.h"
#include "../SDL/include/SDL_video.h"

int isAudioHookReady(void);

void VSTAudioHookInit(TAAudioDesc& ar, void (*callback)(void *, float **, int), void *userdata);

void HandOverSDLWindow(SDL_Window* w);

void SetAudioHookReallyReady(void);

void suspend2(bool run);

bool isNewProject();

void setProjectSync(bool val);

bool projectSyncEnabled();

bool isBlobLoaded();

void saveFurnaceBlob(void* ptr, size_t size);

struct Blob {
	unsigned char* blob;
	size_t size;
};

Blob getBlob();


double GetVstMidiBoy(std::vector<unsigned char>* msg);

