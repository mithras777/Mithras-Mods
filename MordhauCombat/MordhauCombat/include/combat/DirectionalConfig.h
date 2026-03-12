#pragma once

namespace MC::DIRECTIONAL
{
	struct ConfigData
	{
		bool enabled{ true };
		bool debugMode{ false };
		float mouseSensitivity{ 1.0f };
		float deadzone{ 0.20f };
		float smoothing{ 0.18f };
		float dragMaxAccel{ 0.20f };
		float dragMaxSlow{ -0.20f };
		float dragSmoothing{ 0.35f };
		float weaponSpeedMultScale{ 1.0f };
		float overlayScale{ 1.0f };
		unsigned int toggleKey{ 0x2F };  // V
	};

	class Config
	{
	public:
		static Config* GetSingleton();

		const ConfigData& GetData() const;
		ConfigData& GetMutableData();

		bool Load();
		bool Save() const;

	private:
		Config() = default;

		ConfigData m_data{};
	};
}
