/**
 * Furnace Tracker - multi-system chiptune tracker
 * Copyright (C) 2021-2022 tildearrow and contributors
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <string.h>
#include <vector>
#include "../ta-log.h"
#include "vstAudio.h"
#include <assert.h>

void taVSTProcess(void* inst, float** buf, int nframes) {
  TAAudioVST* in=(TAAudioVST*)inst;
  in->onProcess(buf,nframes);
}

void TAAudioVST::onProcess(float** buf, int nframes) {
  if (audioProcCallback!=NULL) {
    if (midiIn!=NULL) midiIn->gather();
    audioProcCallback(audioProcCallbackUser,inBufs,outBufs,desc.inChans,desc.outChans,nframes);
  }
  for (size_t j=0; j<nframes; j++) {
    for (size_t i=0; i<desc.outChans; i++) {
      buf[i][j] = outBufs[i][j];
    }
  }
}

void* TAAudioVST::getContext() {
  assert(false);
  return NULL;
}

bool TAAudioVST::quit() {
  if (!initialized) return false;
  
  if (running) {
    running=false;
  }

  for (int i=0; i<desc.outChans; i++) {
    delete[] outBufs[i];
  }

  delete[] outBufs;
  
  initialized=false;
  return true;
}

bool TAAudioVST::setRun(bool run) {
  if (!initialized) return false;
  suspend2(run);
  running=run;
  return running;
}

std::vector<String> TAAudioVST::listAudioDevices() {
  std::vector<String> ret;
  ret.push_back("VST");
  return ret;
}

bool TAAudioVST::init(TAAudioDesc& request, TAAudioDesc& response) {
  if (initialized) {
    logE("audio already initialized");
    return false;
  }
  if (!audioSysStarted) {
      //Spin and wait for hook to initialize.
      while (!isAudioHookReady()) {

      }
    audioSysStarted=true;
  }

  desc=request;
  desc.outFormat=TA_AUDIO_FORMAT_F32;
  TAAudioDesc ar;
  VSTAudioHookInit(ar, taVSTProcess, this);
  desc.deviceName=request.deviceName;
  desc.name="";
  desc.rate=ar.rate;
  desc.inChans=0;
  desc.outChans=ar.outChans;
  desc.bufsize=ar.bufsize;
  desc.fragments=1;

  if (desc.outChans>0) {
    outBufs=new float*[desc.outChans];
    for (int i=0; i<desc.outChans; i++) {
      outBufs[i]=new float[desc.bufsize];
    }
  }

  response=desc;
  initialized=true;
  return true;
}