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

#include "../ta-log.h"
#include "taAudio.h"
#include "../vst/pluginhook.h"
#include "vstmidi.h"

// --- IN ---

bool TAMidiInVstMidi::gather() {
    std::vector<unsigned char> msg;
        while (true) {
            TAMidiMessage m;
            double t = GetVstMidiBoy(&msg);
            if (msg.empty()) break;
            // parse message
            m.time = t;
            m.type = msg[0];
            if (m.type != TA_MIDI_SYSEX && msg.size() > 1) {
                memcpy(m.data, msg.data() + 1, MIN(msg.size() - 1, 7));
            }
            else if (m.type == TA_MIDI_SYSEX) {
                m.sysExData.reset(new unsigned char[msg.size()]);
                m.sysExLen = msg.size();
                logD("got a SysEx of length %ld!", msg.size());
                memcpy(m.sysExData.get(), msg.data(), msg.size());
            }
            queue.push(m);
        }
  
    return true;
}

std::vector<String> TAMidiInVstMidi::listDevices() {
    std::vector<String> ret;
    ret.push_back("VST");
    return ret;
}

bool TAMidiInVstMidi::isDeviceOpen() {
    return isOpen;
}

bool TAMidiInVstMidi::openDevice(String name) {
    if (isOpen) return false;
    if (name == "VST")
        return true;
    return false;
}

bool TAMidiInVstMidi::closeDevice() {
    isOpen = false;
    return true;
}

bool TAMidiInVstMidi::init() {
    return true;
}

bool TAMidiInVstMidi::quit() {
    return true;
}