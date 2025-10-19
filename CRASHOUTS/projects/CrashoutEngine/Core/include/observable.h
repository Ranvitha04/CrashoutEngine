/*!
@file       observable.h
@author     miratristan.d@digipen.edu
@date       19/09/2025
@brief      Declaration for the Base class Observable. Allows other systems to inherit from this class to register Observers.

//__________________________________________________________________________*/

#pragma once
#pragma warning(disable: 4251)
#include <map>
#include <string>

#include "ImportExport.h"
#include "messages.h"
#include "observer.h"

namespace Messaging {

	/*!
	@class Observable
	@brief Base class for Observers to watch events from.
	*/
	DLL_API class Observable {
		protected:
			Observable();
			Observable(const std::string& id);
		private:
			using Multimap = std::multimap<std::string, Observer&>;
			Multimap mMessageMap;
			std::string mId;
		public:

			virtual ~Observable() = default;

			/*!
			@brief Constructor for message
			@param messageId
			@param observer
			*/
			DLL_API void registerObserver(const std::string& messageId, Observer& observer);

			/*!
			@brief Call the callback function for the appropriate message passed in
			@param msg the data used to identify which callback function to execute
			*/
			void sendMessage(Messages& msg);

			std::string id();

	};
}

