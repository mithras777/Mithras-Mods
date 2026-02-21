#pragma once

#include "RE/S/SendHUDMessage.h"

namespace RE
{
	inline void DebugNotification(const char* a_message)
	{
		SendHUDMessage::ShowHUDMessage(a_message);
	}
}
