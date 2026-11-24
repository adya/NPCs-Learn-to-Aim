#include "AimDebug.h"

#ifndef NDEBUG

#	include <SKSEMCP/SKSEMenuFramework.hpp>

#	include <chrono>
#	include <mutex>
#	include <unordered_map>

using namespace ImGuiMCP;
using namespace ImGuiMCP::ImVec4Manager;
using namespace ImGuiMCP::ImDrawListManager;

namespace NLA::AimDebug
{
	namespace
	{
		struct Marker
		{
			RE::NiPoint3                          point;
			Mark                                  mark;
			std::string                           label;
			std::chrono::steady_clock::time_point recordedAt;
		};
		std::unordered_map<RE::FormID, Marker> markers;
		std::mutex                             markersMutex;

		constexpr auto kMarkerLifetime = std::chrono::milliseconds(500);

		RE::NiCamera* GetNiCamera() {
			auto* camera = RE::PlayerCamera::GetSingleton();
			if (!camera || !camera->cameraRoot) {
				return nullptr;
			}

			auto& children = camera->cameraRoot->GetChildren();
			if (children.empty() || !children[0]) {
				return nullptr;
			}

			return netimmerse_cast<RE::NiCamera*>(children[0].get());
		}

		/// The color and caption of a marker.
		std::pair<ImU32, const char*> Style(Mark mark) {
			switch (mark) {
			case Mark::kHead:
				return { IM_COL32(255, 60, 60, 255), "HEAD" };
			case Mark::kHand:
				return { IM_COL32(60, 200, 255, 255), "HAND" };
			case Mark::kLeg:
				return { IM_COL32(90, 230, 90, 255), "LEG" };
			default:
				return { IM_COL32(255, 220, 60, 255), "AIM" };
			}
		}

		void __stdcall Render() {
			if (!enabled) {
				return;
			}

			std::unordered_map<RE::FormID, Marker> local;
			{
				std::lock_guard lock(markersMutex);
				if (markers.empty()) {
					return;
				}
				std::swap(local, markers);
			}

			auto* niCamera = GetNiCamera();
			if (!niCamera) {
				return;
			}

			auto* io = GetIO();
			auto* drawList = GetForegroundDrawList();
			if (!io || !drawList) {
				return;
			}

			auto now = std::chrono::steady_clock::now();
			for (auto it = local.begin(); it != local.end();) {
				if (now - it->second.recordedAt > kMarkerLifetime) {
					it = local.erase(it);
					continue;
				}

				float x, y, z;
				if (niCamera->WorldPtToScreenPt3(it->second.point, x, y, z, 1e-5f) && z > 0.0f) {
					ImVec2 screen{ x * io->DisplaySize.x, (1.0f - y) * io->DisplaySize.y };
					const auto [color, name] = Style(it->second.mark);
					AddCircleFilled(drawList, screen, 6.0f, color, 12);
					AddText(drawList, ImVec2{ screen.x + 8.0f, screen.y - 22.0f }, color, name);
					AddText(drawList, ImVec2{ screen.x + 8.0f, screen.y - 8.0f }, color, it->second.label.c_str());
				}

				++it;
			}

			{
				std::lock_guard lock(markersMutex);
				// Entries recorded while the lock was released are newer than this stale copy, so they win.
				for (auto& [formID, marker] : local) {
					markers.try_emplace(formID, std::move(marker));
				}
			}
		}
	}

	void RecordAimPoint(RE::Actor* attacker, const RE::NiPoint3& point, Mark mark) {
		if (!enabled || !attacker) {
			return;
		}

		std::lock_guard lock(markersMutex);
		markers[attacker->GetFormID()] = { point, mark, fmt::format("{}", *attacker), std::chrono::steady_clock::now() };
	}

	void Install() {
		if (!SKSEMenuFramework::IsInstalled()) {
			logger::warn("SKSEMenuFramework is not installed; aim debug overlay disabled");
			return;
		}

		SKSEMenuFramework::AddHudElement(Render);
		logger::info("Installed aim debug overlay");
	}
}

#endif
