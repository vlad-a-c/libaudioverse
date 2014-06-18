/**Copyright (C) Austin Hicks, 2014
This file is part of Libaudioverse, a library for 3D and environmental audio simulation, and is released under the terms of the Gnu General Public License Version 3 or (at your option) any later version.
A copy of the GPL, as well as other important copyright and licensing information, may be found in the file 'LICENSE' in the root of the Libaudioverse repository.  Should this file be missing or unavailable to you, see <http://www.gnu.org/licenses/>.*/
#include <libaudioverse/private_devices.hpp>
#include <portaudio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <thread>
#include <atomic>
#include <set>

int portaudioOutputCallback(const void* input, void* output, unsigned long frameCount, const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void* userData);

class LavPortaudioDevice: LavDevice {
	void audioOutputThreadFunction(); //the function that runs as our output thread.
	std::thread audioOutputThread;
	std::atomic_flag runningFlag; //when this clears, the audio thread self-terminates.
	PaStream *stream; //the portaudio stream we work with.  Started and stopped by the background thread for us.
	//a ringbuffer of dispatched buffers.
	//the elements are protected by the atomic ints.
	float** buffers;
	std::atomic<int> *buffer_statuses;
	int callback_buffer_index; //the index the callback will go to on its next invocation.
	friend int portaudioOutputCallback(const void* input, void* output, unsigned long frameCount, const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void* userData);
};

LavError initializeAudioBackend() {
	PaError err = Pa_Initialize();
	if(err < 0) {
		return Lav_ERROR_CANNOT_INIT_AUDIO;
	}
	return Lav_ERROR_NONE;
}

/**This algorithm is complex and consequently requires some explanation.
- At the beginning, i.e. when this thread starts, all of the buffers point to valid memory locations big enough for one block.  In addition, all atomic ints are set to 0, meaning unprocessed.
- This thread processes buffers as fast as it possibly can.  If it sees an atomic int of 0, it assumes unprocessed and goes to sleep for a bit.
- The callback will set the output buffer to 0 if it sees an atomic int of 0 at its current reading position.
- If the callback sees any other value, it will copy the memory out and flip that value to 0.
*/
void LavPortaudioDevice::audioOutputThreadFunction() {
	int rb_index = 0; //our index into the buffers array.
	while(runningFlag.test_and_set()) {
		if(buffer_statuses[rb_index].load()) { //we just caught up and the queue is full.
			continue;
		}
		//process into this buffer.
		getBlock(buffers[rb_index]);
		//mark it as safe for the audio callback.
		buffer_statuses[rb_index].store(1);
		//and compute the next index.
		rb_index += 1;
		rb_index %= mixahead;
	}
}

int portaudioOutputCallback(const void* input, void* output, unsigned long frameCount, const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void* userData) {
	LavPortaudioDevice * const dev = (LavPortaudioDevice*)userData;
	const int haveBuffer = dev->buffer_statuses[dev->callback_buffer_index].load();
	if(haveBuffer) {
		memcpy(output, dev->buffers[dev->callback_buffer_index], sizeof(float)*frameCount);
		dev->buffer_statuses[dev->callback_buffer_index].store(0);
		dev->callback_buffer_index++;
	}
	else {
		memset(output, 0, frameCount*sizeof(float));
	}
	return paContinue;
}
