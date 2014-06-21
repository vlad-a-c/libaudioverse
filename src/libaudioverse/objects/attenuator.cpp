/**Copyright (C) Austin Hicks, 2014
This file is part of Libaudioverse, a library for 3D and environmental audio simulation, and is released under the terms of the Gnu General Public License Version 3 or (at your option) any later version.
A copy of the GPL, as well as other important copyright and licensing information, may be found in the file 'LICENSE' in the root of the Libaudioverse repository.  Should this file be missing or unavailable to you, see <http://www.gnu.org/licenses/>.*/
#include <libaudioverse/libaudioverse.h>
#include <libaudioverse/libaudioverse_properties.h>
#include <libaudioverse/private_devices.hpp>
#include <libaudioverse/private_objects.hpp>
#include <libaudioverse/private_properties.hpp>
#include <libaudioverse/private_macros.hpp>
#include <limits>

class LavAttenuatorObject: public LavObject {
	public:
	virtual void init(LavDevice* device, unsigned int numChannels);
	virtual void process();
};

void LavAttenuatorObject::init(LavDevice* device, unsigned int numChannels) {
	LavObject::init(device, numChannels, numChannels);
	properties[Lav_ATTENUATOR_MULTIPLIER] = createFloatProperty("multiplier", 1.0f, -std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity());
}

LavObject* createAttenuatorObject(LavDevice* device, unsigned int numChannels) {
	auto retval = new LavAttenuatorObject();
	retval->init(device, numChannels);
	return retval;
}

void LavAttenuatorObject::process() {
	const float mul = properties[Lav_ATTENUATOR_MULTIPLIER]->getFloatValue();
	for(unsigned int i = 0; i < device->getBlockSize(); i++) {
		for(unsigned int j = 0; j < num_outputs; j++) {
			outputs[j][i] = inputs[j][i]*mul;
		}
	}
}

//begin public api

Lav_PUBLIC_FUNCTION LavError Lav_createAttenuatorObject(LavDevice* device, unsigned int numChannels, LavObject** destination) {
	PUB_BEGIN
	LOCK(*device);
	auto retval = createAttenuatorObject(device, numChannels);
	*destination = retval;
	PUB_END
}
