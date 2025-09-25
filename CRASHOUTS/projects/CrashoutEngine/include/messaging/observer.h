#pragma once

#include <map>
#include <string>
#include <list>
#include <vector>
#include <memory>
#include "messages.h"

namespace Messaging {
	using HANDLER = void (*)(Messages&);

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

			void attachHandler(const std::string& messageId, HANDLER callBackFn) {
				mHandlers[messageId] = callBackFn;
			}

			bool isDelayed() {
				return delayed;
			}

			HANDLER getHandler(const std::string& messageId) {
				return mHandlers[messageId];
			}

		
	};
}