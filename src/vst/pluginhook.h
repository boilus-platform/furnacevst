#pragma once
#include <vector>
#include "../audio/taAudio.h"
#include "../SDL/include/SDL_video.h"

int isAudioHookReady(void);

void VSTAudioHookInit(TAAudioDesc& ar, void (*callback)(void *, float **, int), void *userdata);

void HandOverSDLWindow(SDL_Window* w);

void SetAudioHookReallyReady(void);

void suspend2(bool run);

double GetVstMidiBoy(std::vector<unsigned char>* msg);

