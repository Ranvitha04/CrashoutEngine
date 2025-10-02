/*!
@file       observable.cpp
@author     miratristan.d@digipen.edu
@date       19/09/2025
@brief      Member Function definitions for the Base class Observable. Includes definitions for registerObserver, sendMessage.

//__________________________________________________________________________*/

#include "messaging/observable.h"
#include <utility>
#include <algorithm>

namespace Messaging {
	Observable::Observable() : mId {"Default_Message"} {}

	Observable::Observable(const std::string& id) : mId {id} {}

	void Observable::registerObserver(const std::string& messageId, Observer& observer) {
		mMessageMap.insert(Multimap::value_type{ messageId, observer });
	}
	void Observable::sendMessage(Messages& message) {
		std::pair<Multimap::iterator, Multimap::iterator> range = mMessageMap.equal_range(message.id());
		for (Multimap::iterator it{ range.first }; it != range.second; ++it) {
			//std::cout << "Message Processed by Observer: " << it->second->GetId() << std::endl;
			it->second.getHandler(message.id())(message, it->second);
		}
	}
	std::string Observable::id() {
		return mId;
	}
}