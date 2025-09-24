#pragma once

#include <map>
#include <string>
#include "messages.h"
#include "observer.h"

namespace Messaging {
	class Observable {
		protected:
			Observable();
			Observable(const std::string& id);
		private:
			using Multimap = std::multimap<std::string, Observer&>;
			Multimap mMessageMap;
			std::string mId;
		public:

			virtual ~Observable() = default;

			/*REGISTER OBSERVERS TO LISTEN TO THE MESSAGEID*/
			void registerObserver(const std::string& messageId, Observer& observer);

			/*SEND */
			void sendMessage(Messages&);

			std::string id();

	};
}

