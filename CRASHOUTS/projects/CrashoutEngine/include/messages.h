/*!
@file       messages.h
@author     miratristan.d@digipen.edu
@date       19/09/2025
@brief      Declaration for the Base class Messages, and common IDS for messages to listen to.

//__________________________________________________________________________*/

#pragma once

#include <string>

namespace Messaging {

	/*COMMON MESSAGE TYPES*/
	inline std::string getCollisionMsgID() {
		return std::string{ "0" };
	}

	inline std::string getEnterMsgID() {
		return std::string{ "1" };
	}

	inline std::string getStayMsgID() {
		return std::string{ "2" };
	}

	inline std::string getExitMsgID() {
		return std::string{ "3" };
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
