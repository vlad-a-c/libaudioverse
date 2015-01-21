/**Copyright (C) Austin Hicks, 2014
This file is part of Libaudioverse, a library for 3D and environmental audio simulation, and is released under the terms of the Gnu General Public License Version 3 or (at your option) any later version.
A copy of the GPL, as well as other important copyright and licensing information, may be found in the file 'LICENSE' in the root of the Libaudioverse repository.  Should this file be missing or unavailable to you, see <http://www.gnu.org/licenses/>.*/

/**Handles functionality common to all objects: linking, allocating, and freeing, as well as parent-child relationships.*/
#include <libaudioverse/libaudioverse.h>
#include <libaudioverse/libaudioverse_properties.h>
#include <libaudioverse/private/memory.hpp>
#include <libaudioverse/private/connections.hpp>
#include <libaudioverse/private/node.hpp>
#include <libaudioverse/private/properties.hpp>
#include <libaudioverse/private/simulation.hpp>
#include <libaudioverse/private/macros.hpp>
#include <libaudioverse/private/metadata.hpp>
#include <libaudioverse/private/kernels.hpp>
#include <algorithm>
#include <memory>
#include <stdlib.h>
#include <string.h>

LavOutputConnection::LavOutputConnection(std::shared_ptr<LavSimulation> simulation, LavNode* node, int start, int count) {
	this->simulation = simulation;
	this->node= node;
	this->start =start;
	this->count = count;
}

void LavOutputConnection::add(int inputBufferCount, float** inputBuffers, bool shouldApplyMixingMatrix) {
	//make sure our node has been ticked (nodes short-circuit if the simulation has not advanced).
	node->tick();
	//get the array of outputs from our node.
	float** outputArray=node->getOutputBufferArray();
	//it is the responsibility of our node to keep us configured, so we assume what info we have is accurate. If it is not, that is the fault of our node.
	const float* mixingMatrix= simulation->getMixingMatrix(count, inputBufferCount); //mixing from our outputs to their inputs.
	if(shouldApplyMixingMatrix && mixingMatrix) {
		applyMixingMatrix(simulation->getBlockSize(), count, outputArray+start, inputBufferCount, inputBuffers, mixingMatrix);
	}
	else { //copy and drop.
		int channelsToAdd =std::min(count, inputBufferCount);
		for(int i=0; i < channelsToAdd; i++) additionKernel(simulation->getBlockSize(), outputArray[i+start], inputBuffers[i], inputBuffers[i]);
	}
}

void LavOutputConnection::reconfigure(int newStart, int newCount) {
	start=newStart;
	count= newCount;
}

void LavOutputConnection::clear() {
	for(auto &c: connected_to) {
		auto c_s= c.lock();
		if(c_s) c_s->forgetConnection(this);
	}
	//and then kill the whole set.
	connected_to.clear();
}

void LavOutputConnection::connectHalf(std::shared_ptr<LavInputConnection> inputConnection) {
	connected_to.insert(inputConnection);
}

LavInputConnection::LavInputConnection(std::shared_ptr<LavSimulation> simulation, LavNode* node, int start, int count) {
	this->simulation = simulation;
	this->node= node;
	this->start=start;
	this->count = count;
}

void LavInputConnection::add(bool shouldApplyMixingMatrix) {
	addNodeless(node->getInputBufferArray(), shouldApplyMixingMatrix);
}

void LavInputConnection::addNodeless(float** inputs, bool shouldApplyMixingMatrix) {
	for(auto &i: connected_to) {
		auto i_s = i.lock();
		if(i_s == nullptr) continue;
		i_s->add(count, inputs+start, shouldApplyMixingMatrix);
	}
}

void LavInputConnection::reconfigure(int newStart, int newCount) {
	start= newStart;
	count= newCount;
}

void LavInputConnection::connectHalf(std::shared_ptr<LavOutputConnection> outputConnection) {
	connected_to.insert(outputConnection);
}

void LavInputConnection::forgetConnection(LavOutputConnection* which) {
	decltype(connected_to) removing;
	for(auto &i: connected_to) {
		auto i_s=i.lock();
		//we might as well take this opportunity to clean up dead weak pointers since we're already creating the temporary set.
		if(i_s == nullptr || i_s.get()== which) removing.insert(i);
	}
	for(auto &i: removing) {
		connected_to.erase(i);
	}
}


void makeConnection(std::shared_ptr<LavOutputConnection> output, std::shared_ptr<LavInputConnection> input) {
	output->connectHalf(input);
	input->connectHalf(output);
}