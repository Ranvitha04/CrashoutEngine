/*!
@file   observable.h
@author Ranvitha (68%), Renfred Lee (32%)
@date   19/09/2025

@brief  Declaration for the Base class Observable. Allows other systems to inherit from this class to register Observers.

All content (c) 2025 Digipen Institute of Technology Singapore. All rights reserved.

//__________________________________________________________________________*/

#pragma once

#include <map>
#include "messages.h"
#include "observer.h"

namespace Messaging {

	/*!
	@class Observable
	@brief Base class for Observers to watch events from.
	*/
	/** @brief Observable */
	class Observable {
		protected:
			Observable();
			Observable(const EventType& id);
		private:
			using Multimap = std::multimap<EventType, Observer&>;
			Multimap mMessageMap;
			EventType mType;
		public:

			virtual ~Observable() = default;

			/*!
			@brief Constructor for message
			@param messageId
			@param observer
			*/
			void registerObserver(const EventType& messageType, Observer& observer);

			/*!
			@brief Call the callback function for the appropriate message passed in
			@param msg the data used to identify which callback function to execute
			*/
			void sendMessage(Messages& msg);

			EventType Type();

			/*!
			@brief Cleanup
			*/
			void ClearObservers() {
				std::multimap<EventType, Observer&>().swap(mMessageMap);
			}

	};
}

