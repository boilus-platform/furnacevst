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

#include "utfutils.h"

//-------------------------------------------------------------------------------------------------------
// VST Plug-Ins SDK
// Version 2.4       $Date: 2006/08/29 12:08:50 $
//
// Category     : VST 2.x Classes
// Filename     : vstplugmain.cpp
// Created by   : Steinberg Media Technologies
// Description  : VST Plug-In Main Entry
//
// © 2006, Steinberg Media Technologies, All Rights Reserved
//-------------------------------------------------------------------------------------------------------

#include "audioeffect.h"

//------------------------------------------------------------------------
/** Must be implemented externally. */
extern AudioEffect* createEffectInstance(audioMasterCallback audioMaster);

extern "C" {

#if defined (__GNUC__) && ((__GNUC__ >= 4) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 1)))
#define VST_EXPORT	__attribute__ ((visibility ("default")))
#else
#define VST_EXPORT
#endif

	//------------------------------------------------------------------------
	/** Prototype of the export function main */
	//------------------------------------------------------------------------
	VST_EXPORT AEffect* VSTPluginMain(audioMasterCallback audioMaster)
	{
		// Get VST Version of the Host
		if (!audioMaster(0, audioMasterVersion, 0, 0, 0, 0))
			return 0;  // old version

		// Create the AudioEffect
		AudioEffect* effect = createEffectInstance(audioMaster);
		if (!effect)
			return 0;

		// Return the VST AEffect structur
		return effect->getAeffect();
	}

	// support for old hosts not looking for VSTPluginMain
#if (TARGET_API_MAC_CARBON && __ppc__)
	VST_EXPORT AEffect* main_macho(audioMasterCallback audioMaster) { return VSTPluginMain(audioMaster); }
#elif WIN32 || _WIN64
	VST_EXPORT AEffect* MAIN(audioMasterCallback audioMaster) { return VSTPluginMain(audioMaster); }
#elif BEOS
	VST_EXPORT AEffect* main_plugin(audioMasterCallback audioMaster) { return VSTPluginMain(audioMaster); }
#endif

} // extern "C"

//------------------------------------------------------------------------
#if WIN32 || _WIN64
#include <windows.h>
void* hInstance;

extern "C" {
	DWORD WINAPI MainGUIThread(LPVOID lpParam) {
		main(NULL, NULL);
		return 0;
	}

	BOOL WINAPI DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID lpvReserved)
	{
		hInstance = hInst;
		switch (dwReason)
		{
		case DLL_PROCESS_ATTACH:
			CreateThread(NULL, 0, MainGUIThread, 0, 0, 0);
			break;
		default:
			break;
		}
		return 1;
	}
} // extern "C"
#endif
