#include "combat/DirectionalController.h"

#include "combat/DirectionalConfig.h"
#include "game/GameHelper.h"
#include "util/LogUtil.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <format>

namespace MC::DIRECTIONAL
{
	namespace
	{
		constexpr float PI = 3.14159265358979323846f;

		DirectionBucket ResolveBucket(float a_x, float a_y, float a_deadzone)
		{
			const float magnitude = std::sqrt(a_x * a_x + a_y * a_y);
			if (magnitude < a_deadzone) {
				return DirectionBucket::Center;
			}

			float angle = std::atan2(-a_y, a_x);
			if (angle < 0.0f) {
				angle += 2.0f * PI;
			}

			const float sector = (2.0f * PI) / 8.0f;
			const int index = static_cast<int>(std::floor((angle + sector * 0.5f) / sector)) % 8;

			switch (index) {
				case 0: return DirectionBucket::Right;
				case 1: return DirectionBucket::TopRight;
				case 2: return DirectionBucket::Top;
				case 3: return DirectionBucket::TopLeft;
				case 4: return DirectionBucket::Left;
				case 5: return DirectionBucket::BottomLeft;
				case 6: return DirectionBucket::Bottom;
				case 7: return DirectionBucket::BottomRight;
				default: return DirectionBucket::Center;
			}
		}

		RE::NiPoint2 BucketAxis(DirectionBucket a_bucket)
		{
			switch (a_bucket) {
				case DirectionBucket::Top: return RE::NiPoint2(0.0f, -1.0f);
				case DirectionBucket::TopRight: return RE::NiPoint2(0.7071f, -0.7071f);
				case DirectionBucket::Right: return RE::NiPoint2(1.0f, 0.0f);
				case DirectionBucket::BottomRight: return RE::NiPoint2(0.7071f, 0.7071f);
				case DirectionBucket::Bottom: return RE::NiPoint2(0.0f, 1.0f);
				case DirectionBucket::BottomLeft: return RE::NiPoint2(-0.7071f, 0.7071f);
				case DirectionBucket::Left: return RE::NiPoint2(-1.0f, 0.0f);
				case DirectionBucket::TopLeft: return RE::NiPoint2(-0.7071f, -0.7071f);
				default: return RE::NiPoint2(0.0f, 0.0f);
			}
		}

		bool ContainsInsensitive(std::string_view a_input, std::string_view a_token)
		{
			auto lower = [](char c) {
				return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
			};

			for (size_t i = 0; i + a_token.size() <= a_input.size(); ++i) {
				bool match = true;
				for (size_t j = 0; j < a_token.size(); ++j) {
					if (lower(a_input[i + j]) != lower(a_token[j])) {
						match = false;
						break;
					}
				}
				if (match) {
					return true;
				}
			}
			return false;
		}

		const char* BucketName(DirectionBucket a_bucket)
		{
			switch (a_bucket) {
				case DirectionBucket::Center: return "Center";
				case DirectionBucket::Top: return "Top";
				case DirectionBucket::TopRight: return "TopRight";
				case DirectionBucket::Right: return "Right";
				case DirectionBucket::BottomRight: return "BottomRight";
				case DirectionBucket::Bottom: return "Bottom";
				case DirectionBucket::BottomLeft: return "BottomLeft";
				case DirectionBucket::Left: return "Left";
				case DirectionBucket::TopLeft: return "TopLeft";
				default: return "Center";
			}
		}
	}

	Controller* Controller::GetSingleton()
	{
		static Controller singleton;
		return std::addressof(singleton);
	}

	Controller::Controller()
	{
		auto* config = Config::GetSingleton();
		config->Load();
		m_enabled.store(config->GetData().enabled);
	}

	void Controller::OnMouseDelta(int a_dx, int a_dy)
	{
		if (!m_enabled.load()) {
			return;
		}

		const auto& config = Config::GetSingleton()->GetData();
		const float inputScale = 0.018f * std::clamp(config.mouseSensitivity, 0.05f, 10.0f);
		m_targetInputX += static_cast<float>(a_dx) * inputScale;
		m_targetInputY += static_cast<float>(a_dy) * inputScale;

		const float mag = std::sqrt((m_targetInputX * m_targetInputX) + (m_targetInputY * m_targetInputY));
		if (mag > 1.0f) {
			m_targetInputX /= mag;
			m_targetInputY /= mag;
		}
	}

	void Controller::OnAttackPressed(bool a_power)
	{
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player || !IsContextValid(player) || !m_enabled.load()) {
			return;
		}

		m_latched.bucket = m_cursor.bucket;
		m_latched.power = a_power;
		m_latched.startWorldTime = GAME::HELPER::Get_DeltaWorldTime();
		m_latched.active = true;

		if (Config::GetSingleton()->GetData().debugMode) {
			const auto msg = std::format("MC Attack Triggered: {} {}", a_power ? "Power" : "Light", BucketName(m_latched.bucket));
			RE::SendHUDMessage::ShowHUDMessage(msg.c_str());
		}
	}

	void Controller::OnAnimationEvent(std::string_view a_tag, std::string_view)
	{
		if (ContainsInsensitive(a_tag, "attackstop")) {
			auto* player = RE::PlayerCharacter::GetSingleton();
			if (player) {
				ClearAttackRuntime(player);
			}
			return;
		}

		if (ContainsInsensitive(a_tag, "attackstart")) {
			const bool isPower = ContainsInsensitive(a_tag, "power");
			OnAttackPressed(isPower);
		}
	}

	void Controller::Update(float a_delta)
	{
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			return;
		}

		if (!m_enabled.load() || !IsContextValid(player)) {
			ClearAttackRuntime(player);
			m_targetInputX = 0.0f;
			m_targetInputY = 0.0f;
			m_cursor.x = 0.0f;
			m_cursor.y = 0.0f;
			m_cursor.magnitude = 0.0f;
			m_cursor.bucket = DirectionBucket::Center;
			m_smoothedInputX = 0.0f;
			m_smoothedInputY = 0.0f;
			PublishGraphVariables(player);
			return;
		}

		const auto& config = Config::GetSingleton()->GetData();
		const float smoothing = std::clamp(config.smoothing, 0.0f, 0.95f);
		const float response = 8.0f + ((1.0f - smoothing) * 26.0f);
		const float alpha = std::clamp(1.0f - std::exp(-response * std::max(0.0f, a_delta)), 0.01f, 1.0f);

		// Smooth 360 deg motion: accumulate target input then ease toward it over time.
		m_smoothedInputX += (m_targetInputX - m_smoothedInputX) * alpha;
		m_smoothedInputY += (m_targetInputY - m_smoothedInputY) * alpha;

		const float decay = std::exp(-6.0f * std::max(0.0f, a_delta));
		m_targetInputX *= decay;
		m_targetInputY *= decay;

		if (std::abs(m_targetInputX) < 0.0005f) m_targetInputX = 0.0f;
		if (std::abs(m_targetInputY) < 0.0005f) m_targetInputY = 0.0f;

		const float smoothMag = std::sqrt((m_smoothedInputX * m_smoothedInputX) + (m_smoothedInputY * m_smoothedInputY));
		if (smoothMag > 0.0001f) {
			m_lastNormInputX = m_smoothedInputX / smoothMag;
			m_lastNormInputY = m_smoothedInputY / smoothMag;
		} else {
			m_lastNormInputX = 0.0f;
			m_lastNormInputY = 0.0f;
		}
		UpdateBuckets();

		if (m_latched.active) {
			const auto axis = BucketAxis(m_latched.bucket);
			const float dot = (m_lastNormInputX * axis.x) + (m_lastNormInputY * axis.y);
			const float smooth = std::clamp(config.dragSmoothing, 0.01f, 0.95f);
			m_dragScalar = (m_dragScalar * (1.0f - smooth)) + (dot * smooth);
			m_dragScalar = std::clamp(m_dragScalar, config.dragMaxSlow, config.dragMaxAccel);
			ApplyWeaponSpeedScalar(player, m_dragScalar * config.weaponSpeedMultScale);
		} else {
			ApplyWeaponSpeedScalar(player, 0.0f);
			m_dragScalar = 0.0f;
		}

		PublishGraphVariables(player);
	}

	void Controller::SetEnabled(bool a_enabled)
	{
		m_enabled.store(a_enabled);
		auto* config = Config::GetSingleton();
		config->GetMutableData().enabled = a_enabled;
		config->Save();
	}

	bool Controller::IsEnabled() const
	{
		return m_enabled.load();
	}

	void Controller::ToggleEnabled()
	{
		SetEnabled(!IsEnabled());
	}

	CursorVectorState Controller::GetCursorState() const
	{
		return m_cursor;
	}

	LatchedAttackState Controller::GetLatchedState() const
	{
		return m_latched;
	}

	float Controller::GetDragScalar() const
	{
		return m_dragScalar;
	}

	bool Controller::ShouldShowOverlay() const
	{
		auto* player = RE::PlayerCharacter::GetSingleton();
		return m_enabled.load() && player && IsContextValid(player);
	}

	void Controller::UpdateBuckets()
	{
		const auto& config = Config::GetSingleton()->GetData();
		m_cursor.x = m_smoothedInputX;
		m_cursor.y = m_smoothedInputY;
		m_cursor.magnitude = std::clamp(std::sqrt(m_cursor.x * m_cursor.x + m_cursor.y * m_cursor.y), 0.0f, 1.0f);
		m_cursor.bucket = ResolveBucket(m_cursor.x, m_cursor.y, config.deadzone);
	}

	void Controller::PublishGraphVariables(RE::PlayerCharacter* a_player) const
	{
		if (!a_player) {
			return;
		}

		a_player->SetGraphVariableBool("MC_Enabled", m_enabled.load());
		a_player->SetGraphVariableFloat("MC_DirX", m_cursor.x);
		a_player->SetGraphVariableFloat("MC_DirY", m_cursor.y);
		a_player->SetGraphVariableInt("MC_DirBucket", static_cast<std::int32_t>(m_cursor.bucket));
		a_player->SetGraphVariableInt("MC_DirLatchedBucket", static_cast<std::int32_t>(m_latched.bucket));
		a_player->SetGraphVariableBool("MC_IsDirAttack", m_latched.active);
		a_player->SetGraphVariableFloat("MC_DragAccelScalar", m_dragScalar);
	}

	void Controller::ApplyWeaponSpeedScalar(RE::PlayerCharacter* a_player, float a_scalar)
	{
		if (!a_player) {
			return;
		}

		if (std::abs(a_scalar - m_appliedWeaponSpeedMod) < 0.0005f) {
			return;
		}

		a_player->AsActorValueOwner()->ModActorValue(RE::ACTOR_VALUE_MODIFIER::kTemporary,
		                                             RE::ActorValue::kWeaponSpeedMult,
		                                             a_scalar - m_appliedWeaponSpeedMod);
		m_appliedWeaponSpeedMod = a_scalar;
	}

	void Controller::ClearAttackRuntime(RE::PlayerCharacter* a_player)
	{
		m_latched.active = false;
		m_latched.power = false;
		m_latched.bucket = DirectionBucket::Center;
		m_dragScalar = 0.0f;
		ApplyWeaponSpeedScalar(a_player, 0.0f);
	}

	bool Controller::IsContextValid(RE::PlayerCharacter* a_player) const
	{
		if (!a_player) {
			return false;
		}

		auto* camera = RE::PlayerCamera::GetSingleton();
		if (!camera || !camera->IsInFirstPerson()) {
			return false;
		}

		if (a_player->AsActorState()->GetWeaponState() != RE::WEAPON_STATE::kDrawn) {
			return false;
		}

		if (a_player->IsDead()) {
			return false;
		}

		if (IsMenuBlocked()) {
			return false;
		}

		return true;
	}

	bool Controller::IsMenuBlocked() const
	{
		auto* ui = RE::UI::GetSingleton();
		if (!ui) {
			return true;
		}

		if (ui->GameIsPaused()) {
			return true;
		}

		return ui->IsMenuOpen(RE::MainMenu::MENU_NAME) ||
		       ui->IsMenuOpen(RE::LoadingMenu::MENU_NAME) ||
		       ui->IsMenuOpen(RE::Console::MENU_NAME) ||
		       ui->IsMenuOpen(RE::DialogueMenu::MENU_NAME) ||
		       ui->IsMenuOpen(RE::InventoryMenu::MENU_NAME) ||
		       ui->IsMenuOpen(RE::JournalMenu::MENU_NAME) ||
		       ui->IsMenuOpen(RE::MapMenu::MENU_NAME);
	}
}
