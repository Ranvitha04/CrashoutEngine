/*!
@file   messages.h
@author tristan (56%), Ranvitha (25%), Renfred Lee (19%)
@date   19/09/2025

@brief  Declaration for the Base class Messages, and common IDS for messages to listen to.

All content (c) 2025 Digipen Institute of Technology Singapore. All rights reserved.

//__________________________________________________________________________*/

#pragma once

#include <cstdint>

namespace Messaging {

	/*COMMON MESSAGE TYPES*/
	enum class EventType : uint32_t {
		None = 0,

		// Physical Collisions
		CollisionEnter = 1,
		CollisionStay = 2,
		CollisionExit = 3,

		// Trigger Overlaps
		TriggerEnter = 4,
		TriggerStay = 5,
		TriggerExit = 6
	};

	/*!
	@class Messages
	@brief Base class to inherit from. Used to send data when Observable is emitting a message
	*/
	/** @brief Messages */
	class Messages {
	protected:
		EventType mType;
		Messages() : mType { EventType::None } {}

		/*!
		@brief Constructor for message
		@param type arg to set mType to
		*/
		Messages(EventType const& type) : mType { type } {}

		/*!
		@brief Move Constructor for message
		@param type arg to set mType to
		*/
		Messages(EventType&& type) : mType { type } {}


	public:
		virtual ~Messages() = default;
		EventType GetType() const { return mType; }
	};
}
