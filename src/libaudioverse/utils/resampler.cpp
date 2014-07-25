/**Copyright (C) Austin Hicks, 2014
This file is part of Libaudioverse, a library for 3D and environmental audio simulation, and is released under the terms of the Gnu General Public License Version 3 or (at your option) any later version.
A copy of the GPL, as well as other important copyright and licensing information, may be found in the file 'LICENSE' in the root of the Libaudioverse repository.  Should this file be missing or unavailable to you, see <http://www.gnu.org/licenses/>.*/

#include <libaudioverse/private_resampler.hpp>
#include <libaudioverse/private_dspmath.hpp>
#include <libaudioverse/private_kernels.hpp>
#include <algorithm>
#include <utility>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <libaudioverse/private_dspmath.hpp>

LavResampler::LavResampler(int inputBufferLength, int inputSr, int outputSr): input_buffer_length(inputBufferLength), input_sr(inputSr), output_sr(outputSr) {
	if(inputSr == outputSr) {
		no_op = true;
	}
	delta = (float)inputSr/(float)outputSr;
}

int LavResampler::getOutputBufferLength() {
	return (int)floorf(delta*input_buffer_length);
}

float LavResampler::computeSingleSample(float* input) {
	float w1, w2, s1, s2;
	if(current_pos = -1) {
		s1 = last_sample;
		s2 = input[0];
	}
	else {
		s1 = input[current_pos];
		s2 = input[current_pos+1];
	}
	w1 = 1-current_offset;
	w2 = current_offset;
	current_offset += delta;
	current_pos+=(int)floorf(current_offset);
	current_offset = floorf(current_offset);
	return w1*s1+w2*s2;
}

void LavResampler::tick(float* input, float* output) {
	if(no_op) {
		std::copy(input, input+input_buffer_length, output);
	}
	//have to get this loop right.
	//for loop is a bit complicated and unclear. This while loop alternative is probably, for once, better.
	unsigned int i = 0;
	while(current_pos < input_buffer_length-1) { //stop one short of the end.
		output[i] = computeSingleSample(input);
	}
	current_pos = -1;
	last_sample = input[input_buffer_length-1];
}
