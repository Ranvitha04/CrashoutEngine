/*!
@file   observer.h
@author Renfred Lee
@date   19/09/2025

@brief  Declaration for the Base class Observar. Allows other system to inherit from this class to listen to messages emitted by
			Observables

All content (c) 2025 Digipen Institute of Technology Singapore. All rights reserved.

//__________________________________________________________________________*/

#pragma once

#include <map>
#include "messages.h"

namespace Messaging {
	/** @brief Observer */
	class Observer;

	using HANDLER = void (*)(Messages&, Observer&);

	/*!
	@class Observer
	@brief Base class that executes HANDLER when appropriate message is received from Observable
	*/
	/** @brief Observer */
	class Observer {
		protected:
			Observer() : mType{ 0 }, delayed{ false } {}
			Observer(const EventType& type, bool delayed = false) : mType{ type }, delayed{delayed} {}
		private:
			std::map<EventType, HANDLER> mHandlers;
			EventType mType;
			bool delayed;
		public:
			virtual ~Observer() = default;

			/*!
			@brief Set the function that will handle messages of a certain messageId
			@param messageType type of message to react to
			@param callBackFn function that will execute to handle the message
			*/
			void attachHandler(const EventType& messageType, HANDLER callBackFn) {
				mHandlers[messageType] = callBackFn;
			}

			bool isDelayed() {
				return delayed;
			}

			/*!
			@brief Getter for callback functions
			@param messageType type of message to get the callback function from
			*/
			HANDLER getHandler(const EventType& messageType) {
				return mHandlers[messageType];
			}
		
	};
}