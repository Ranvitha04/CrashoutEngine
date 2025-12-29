/*!
@file       observable.cpp
@author     miratristan.d@digipen.edu
@date       19/09/2025
@brief      Member Function definitions for the Base class Observable. Includes definitions for registerObserver, sendMessage.

All content © 2025 Digipen Institute of Technology Singapore. All rights reserved.

//__________________________________________________________________________*/

#include "observable.h"
#include <utility>
#include <algorithm>

namespace Messaging {
	Observable::Observable() : mType {} {}

	Observable::Observable(const EventType& type) : mType { type } {}

	void Observable::registerObserver(const EventType& messageId, Observer& observer) {
		mMessageMap.insert(Multimap::value_type{ messageId, observer });
	}
	void Observable::sendMessage(Messages& message) {
		std::pair<Multimap::iterator, Multimap::iterator> range = mMessageMap.equal_range(message.GetType());
		for (Multimap::iterator it{ range.first }; it != range.second; ++it) {
			//std::cout << "Message Processed by Observer: " << it->second->GetId() << std::endl;
			it->second.getHandler(message.GetType())(message, it->second);
		}
	}
	
	EventType Observable::Type() {
		return mType;
	}
}