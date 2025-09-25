#pragma once

#include <string>

namespace Messaging {

	/*COMMON MESSAGE TYPES*/
	inline std::string getCollisionMsgID() {
		return std::string{ "COLLISION_ID" };
	}

	/*!
	@class Messages
	@brief Base class to inherit from. Used to send data when Observable is emitting a message 
	*/
	class Messages {
		protected:
			std::string mId;
			Messages() : mId{ "Default" } {}

			/*!
			@brief Constructor for message
			@param id arg to set mId to
			*/
			Messages(std::string const& id) : mId{ id } {}

			/*!
			@brief Move Constructor for message
			@param id arg to set mId to
			*/
			Messages(std::string&& id) : mId{ id } {}
		public:
			virtual ~Messages() = default;
			std::string id() { return mId; }
	};
}
