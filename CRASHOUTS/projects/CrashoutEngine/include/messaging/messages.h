#pragma once

#include <string>

namespace Messaging {

	/*COMMON MESSAGE TYPES*/
	inline std::string getCollisionMsgID() {
		return std::string{ "COLLISION_ID" };
	}

	/*INHERIT FROM*/
	class Messages {
		protected:
			std::string mId;
			Messages() : mId{ "Default" } {}
			Messages(std::string const& id) : mId{ id } {}
			Messages(std::string&& id) : mId{ id } {}
		public:
			virtual ~Messages() = default;
			std::string id() { return mId; }
	};
}
