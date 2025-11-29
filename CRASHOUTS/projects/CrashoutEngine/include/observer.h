/*!
@file       observer.h
@author     miratristan.d@digipen.edu
@date       19/09/2025
@brief      Declaration for the Base class Observar. Allows other system to inherit from this class to listen to messages emitted by
			Observables

All content © 2025 Digipen Institute of Technology Singapore. All rights reserved.

//__________________________________________________________________________*/

#pragma once

#include <map>
#include <string>
#include "messages.h"

namespace Messaging {
	class Observer;

	using HANDLER = void (*)(Messages&, Observer&);

	/*!
	@class Observer
	@brief Base class that executes HANDLER when appropriate message is received from Observable
	*/
	class Observer {
		protected:
			Observer() : mId{ "Default_id" }, delayed{ false } {}
			Observer(const std::string& id, bool delayed = false) : mId{ id }, delayed{delayed} {}
		private:
			std::map<std::string, HANDLER> mHandlers;
			std::string mId;
			bool delayed;
		public:
			virtual ~Observer() = default;

			/*!
			@brief Set the function that will handle messages of a certain messageId
			@param messageId type of message to react to
			@param callBackFn function that will execute to handle the message
			*/
			void attachHandler(const std::string& messageId, HANDLER callBackFn) {
				mHandlers[messageId] = callBackFn;
			}

			bool isDelayed() {
				return delayed;
			}

			/*!
			@brief Getter for callback functions
			@param messageId type of message to get the callback function from
			*/
			HANDLER getHandler(const std::string& messageId) {
				return mHandlers[messageId];
			}
		
	};
}