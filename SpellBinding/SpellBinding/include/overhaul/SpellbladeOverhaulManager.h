#pragma once

#include <cstdint>
#include <string>

namespace SB_OVERHAUL
{
	class Manager final : public REX::Singleton<Manager>
	{
	public:
		void Initialize();
		void Update(RE::PlayerCharacter* a_player, float a_deltaTime);
		void Serialize(SKSE::SerializationInterface* a_intfc) const;
		void Deserialize(SKSE::SerializationInterface* a_intfc);
		void OnRevert();

		void HandleSetSetting(const std::string& a_payload);
		void HandleAction(const std::string& a_payload);
		void HandleHudAction(const std::string& a_payload);

		void ToggleUI();
		void PushUISnapshot() const;
		void PushHUDSnapshot() const;
		[[nodiscard]] std::string BuildSnapshotJson() const;

	private:
		std::int32_t m_lastActiveChainIndex{ -1 };
	};
}
