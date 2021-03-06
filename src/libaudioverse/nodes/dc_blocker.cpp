/* Copyright 2016 Libaudioverse Developers. See the COPYRIGHT
file at the top-level directory of this distribution.

Licensed under the mozilla Public License, version 2.0 <LICENSE.MPL2 or
https://www.mozilla.org/en-US/MPL/2.0/> or the Gbnu General Public License, V3 or later
<LICENSE.GPL3 or http://www.gnu.org/licenses/>, at your option. All files in the project
carrying such notice may not be copied, modified, or distributed except according to those terms. */
#include <libaudioverse/libaudioverse.h>
#include <libaudioverse/libaudioverse_properties.h>
#include <libaudioverse/nodes/dc_blocker.hpp>
#include <libaudioverse/private/node.hpp>
#include <libaudioverse/private/server.hpp>
#include <libaudioverse/private/properties.hpp>
#include <libaudioverse/private/macros.hpp>
#include <libaudioverse/private/memory.hpp>
#include <libaudioverse/private/constants.hpp>
#include <libaudioverse/implementations/dc_blocker.hpp>

namespace libaudioverse_implementation {

DcBlockerNode::DcBlockerNode(std::shared_ptr<Server> server, int channels): Node(Lav_OBJTYPE_DC_BLOCKER_NODE, server, channels, channels) {
	if(channels <= 0) ERROR(Lav_ERROR_RANGE, "Can only filter 1 or greater channels.");
	appendInputConnection(0, channels);
	appendOutputConnection(0, channels);
	this->channels = channels;
	blockers = new DcBlocker*[channels];
	for(int i = 0; i < channels; i++) blockers[i] = new DcBlocker(server->getSr());
	setShouldZeroOutputBuffers(false);
}

std::shared_ptr<Node> createDcBlockerNode(std::shared_ptr<Server> server, int channels) {
	return standardNodeCreation<DcBlockerNode>(server, channels);
}

DcBlockerNode::~DcBlockerNode() {
	for(int i = 0; i < channels; i++) delete blockers[i];
	delete[] blockers;
}

void DcBlockerNode::process() {
	for(int channel = 0; channel < channels; channel++) {
		auto &b = *blockers[channel];
		for(int i = 0; i < block_size; i++) {
			output_buffers[channel][i] = b.tick(input_buffers[channel][i]);
		}
	}
}

void DcBlockerNode::reset() {
	for(int i = 0; i < channels; i++) blockers[i]->reset();
}

//begin public api

Lav_PUBLIC_FUNCTION LavError Lav_createDcBlockerNode(LavHandle serverHandle, int channels, LavHandle* destination) {
	PUB_BEGIN
	auto server = incomingObject<Server>(serverHandle);
	LOCK(*server);
	auto retval = createDcBlockerNode(server, channels);
	*destination = outgoingObject<Node>(retval);
	PUB_END
}

}