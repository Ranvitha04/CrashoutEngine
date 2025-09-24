#pragma once

#include <map>
#include <string>
#include "messages.h"

namespace Messaging {
	using HANDLER = void (*)(Messages&);

	class Observer {
		protected:
			Observer() : mId{ "Default_id" } {}
			Observer(const std::string& id) : mId{ id } {}
		private:
			std::map<std::string, HANDLER> mHandlers;
			std::string mId;
		public:
			virtual ~Observer() = default;

			void attachHandler(const std::string& messageId, HANDLER callBackFn) {
				mHandlers[messageId] = callBackFn;
			}

			HANDLER getHandler(const std::string& messageId) {
				return mHandlers[messageId];
			}
	};
}