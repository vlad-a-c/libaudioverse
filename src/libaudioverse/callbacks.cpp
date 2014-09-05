/**Copyright (C) Austin Hicks, 2014
This file is part of Libaudioverse, a library for 3D and environmental audio simulation, and is released under the terms of the Gnu General Public License Version 3 or (at your option) any later version.
A copy of the GPL, as well as other important copyright and licensing information, may be found in the file 'LICENSE' in the root of the Libaudioverse repository.  Should this file be missing or unavailable to you, see <http://www.gnu.org/licenses/>.*/
#include <libaudioverse/libaudioverse.h>
#include <libaudioverse/private_callbacks.hpp>
#include <libaudioverse/private_simulation.hpp>
#include <libaudioverse/private_objects.hpp>
#include <string>
#include <memory>

/**Callbacks.*/

LavCallback::LavCallback() {
	is_firing.store(0);
}

LavCallback::LavCallback(const LavCallback &other):
name(other.name), associated_simulation(other.associated_simulation), handler(other.handler), associated_object(other.associated_object), user_data(other.user_data) {
	is_firing.store(0);
}

LavCallback& LavCallback::operator=(LavCallback other) {
	swap(*this, other);
	return *this;
}

void LavCallback::setHandler(LavEventCallback cb) {
	handler = cb;
}

LavEventCallback LavCallback::getHandler() {
	return handler;
}

void LavCallback::fire() {
	if(handler == nullptr) return; //nothing to do.
	if(no_multifire && is_firing.load()) return;
	is_firing.store(1);
	//we need to hold local copies of both the object and data in case they are changed between firing and processing by the simulation.
	auto obj = std::weak_ptr<LavObject>(associated_object->shared_from_this());
	void* userdata = user_data;
	LavEventCallback cb = handler;
	//fire a lambda that uses these by copy.
	associated_simulation->enqueueTask([=]() {
		auto shared_obj = obj.lock();
		if(shared_obj == nullptr) return;	
		//callbacks die with objects; if we get this far, this is still a valid pointer.
		cb(shared_obj.get(), userdata);
		this->is_firing.store(0);
	});
}

void LavCallback::associateSimulation(std::shared_ptr<LavSimulation> simulation) {
	associated_simulation = simulation;
}

void LavCallback::associateObject(LavObject* obj) {
	associated_object = obj;
}

const char* LavCallback::getName() {
	return name.c_str();
}

void LavCallback::setName(const char* n) {
	name = std::string(n);
}

void* LavCallback::getUserData() {
	return user_data;
}

void LavCallback::setUserData(void* data) {
	user_data = data;
}

bool LavCallback::getNoMultifire() {
	return no_multifire;
}

void LavCallback::setNoMultifire(bool what) {
	no_multifire = what;
}
