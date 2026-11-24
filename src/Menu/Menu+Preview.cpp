#include "ConcentrationSpells.h"
#include "Fatigue.h"
#include "Menu+Internal.h"
#include "SkillUsage.h"
#include <SKSEMCP/SKSEMenuFramework.hpp>

using namespace ImGuiMCP;
using namespace ImGuiMCP::ImDrawListManager;

namespace NLA::Menu
{
	namespace
	{
		// ---- Proportions of an average NPC, in meters. Heights are from the feet, x is from the middle of the body. ----

		/// An average NPC is about 128 game units tall.
		constexpr float kNpcHeight = 128.0f / 70.0f;

		/// The head is as tall as the crosshair is wide at this distance (measured: the crosshair matches an NPC's head at about 7 m).
		constexpr float kHeadMatchesCrosshairAt = 7.0f;
		constexpr float kHeadHeight = kNpcHeight * kHeadMatchesCrosshairAt / 60.0f;
		constexpr float kHeadWidth = 0.17f;
		/// Measured against a real NPC: the top of the head is a little below the full height.
		constexpr float kHeadCenterY = kNpcHeight - kHeadHeight * 0.5f - 0.04f;

		constexpr float kShoulderY = 1.50f;
		constexpr float kHipY = 0.92f;

		// ---- Reference to the game's own crosshair ----

		/// Measured in game: at this distance the vanilla crosshair is as tall as an NPC. The apparent size shrinks in proportion to the distance, so an NPC is `kCrosshairMatchesHeightAt / distance` crosshairs tall.
		constexpr float kCrosshairMatchesHeightAt = 60.0f;

		/// The vanilla crosshair is about this many pixels across at 2560x1440 (measured in game).
		constexpr float kCrosshairPx = 60.0f;
		constexpr float kReferenceSide = 150.0f;

		/// The pane is never wider than this across.
		constexpr float kMaxCanvasSide = 520.0f;

		/// What the border of the preview takes, with a little to spare.
		constexpr float kPaneChrome = 2.0f;

		/// The width the preview has to use, whether or not its scrollbar shows. Going by what is left in the window instead would make the canvas, which is square,
		/// change its height with the scrollbar, and so bring the scrollbar and take it away again, frame after frame.
		float UsableWidth() {
			return GetWindowWidth() - kPaneChrome - GetStyle()->ScrollbarSize - GetStyle()->WindowPadding.x * 2;
		}

		/// The progress bar of a hold, in pixels.
		constexpr float kHoldBarWidth = 110.0f;

		constexpr float kMinDistance = 5.0f;
		constexpr float kMaxDistance = 100.0f;
		/// The most shots, or steps of a stream, the preview can show.
		constexpr int kMaxShots = 100;

		/// While the target is held with Interactive on, shots fly at this rate.
		constexpr float kShotsPerSecond = 15.0f;

		/// The distance of the reference, shared by every preview and by the reference drawn on the screen.
		float& SharedDistance() {
			static float distance = 20.0f;
			return distance;
		}

		/// Holding the right button over the pane zooms in this much, and the zoom eases at this rate per second.
		constexpr float kZoom = 2.0f;
		constexpr float kZoomRate = 14.0f;

		constexpr float kMoveRate = 9.0f;  // How fast the dots follow a change of the spread scale, per second.

		/// The pane shows the NPC at a fixed size: this many meters from its middle to each edge, whatever the distance or the spread.
		constexpr float kView = kNpcHeight / 1.4f;  // The NPC is 70% of the pane's height, which leaves room to see how far the misses went.

		/// The wander runs at this many ticks per second, like the game's updates.
		constexpr float kTicksPerSecond = 60.0f;

		/// What a click fires, to begin with.
		constexpr int kShotsPerClick = 10;
		constexpr int kLegTicks = static_cast<int>(ConcentrationSpells::Details::Wander::travelTicks + ConcentrationSpells::Details::Wander::pauseTicks);

		// ---- What the target is made of ----

		/// The standard archery target: ten rings in five colors of two rings each, from the middle out: gold, red, blue, black and white.
		/// The target is half as tall as an NPC, like the archery targets in the game, and its gold circle (the two inner rings, of equal width) is as big as an average head; the eight rings outside
		/// the gold are of equal width.
		constexpr int   kRings = 10;
		constexpr float kTargetRadius = kNpcHeight * 0.25f;
		constexpr float kGoldRadius = kHeadHeight * 0.5f;

		struct ZoneStyle
		{
			int   score;
			ImU32 color;
		};
		constexpr std::array<ZoneStyle, 5> kZones{ {
			{ 10, IM_COL32(222, 192, 80, 255) },
			{ 8, IM_COL32(196, 84, 70, 255) },
			{ 6, IM_COL32(88, 128, 176, 255) },
			{ 4, IM_COL32(52, 56, 58, 255) },
			{ 2, IM_COL32(236, 236, 228, 255) },
		} };

		/// The style of a ring, 0 in the middle.
		const ZoneStyle& StyleOfRing(int ring) {
			return kZones[static_cast<std::size_t>(ring / 2)];
		}

		/// How far from the middle a ring reaches, in meters.
		constexpr float RingRadius(int ring) {
			if (ring < 2)
				return kGoldRadius * static_cast<float>(ring + 1) / 2.0f;
			return kGoldRadius + (kTargetRadius - kGoldRadius) * static_cast<float>(ring - 1) / static_cast<float>(kRings - 2);
		}

		/// The ring at this distance from the middle, 0 in the middle, or nothing outside the target.
		std::optional<int> RingAtDistance(float meters) {
			for (int ring = 0; ring < kRings; ++ring) {
				if (meters < RingRadius(ring))
					return ring;
			}
			return std::nullopt;
		}

		struct Pt
		{
			float x;
			float y;
		};

		using Polygon = std::vector<Pt>;

		Polygon Ellipse(float cx, float cy, float rx, float ry) {
			constexpr int segments = 28;
			Polygon       out;
			for (int i = 0; i < segments; ++i) {
				const float angle = 2.0f * std::numbers::pi_v<float> * static_cast<float>(i) / segments;
				out.push_back({ cx + std::cos(angle) * rx, cy + std::sin(angle) * ry });
			}
			return out;
		}

		/// A humanoid built of convex pieces.
		const std::vector<Polygon>& Silhouette() {
			static const std::vector<Polygon> parts = [] {
				std::vector<Polygon> out;

				const auto addBothSides = [&](const Polygon& points) {
					out.push_back(points);
					Polygon mirrored;
					for (const auto& p : points) {
						mirrored.push_back({ -p.x, p.y });
					}
					out.push_back(std::move(mirrored));
				};

				// Legs: the thigh, the calf and the foot.
				addBothSides({ { 0.02f, kHipY }, { 0.20f, kHipY }, { 0.18f, 0.50f }, { 0.05f, 0.50f } });
				addBothSides({ { 0.05f, 0.50f }, { 0.18f, 0.50f }, { 0.145f, 0.06f }, { 0.06f, 0.06f } });
				addBothSides({ { 0.05f, 0.06f }, { 0.19f, 0.06f }, { 0.21f, 0.0f }, { 0.05f, 0.0f } });

				// Arms hanging down: the upper arm, the forearm and the hand.
				addBothSides({ { 0.23f, 1.49f }, { 0.30f, 1.46f }, { 0.31f, 1.17f }, { 0.245f, 1.17f } });
				addBothSides({ { 0.245f, 1.17f }, { 0.31f, 1.17f }, { 0.305f, 0.93f }, { 0.25f, 0.93f } });
				addBothSides({ { 0.25f, 0.93f }, { 0.305f, 0.93f }, { 0.31f, 0.80f }, { 0.26f, 0.78f } });

				// Torso: the chest, the belly and hips, and the neck.
				out.push_back({ { -0.22f, kShoulderY }, { 0.22f, kShoulderY }, { 0.17f, 1.12f }, { -0.17f, 1.12f } });
				out.push_back({ { -0.17f, 1.12f }, { 0.17f, 1.12f }, { 0.20f, kHipY }, { -0.20f, kHipY } });
				// NPCs have hardly any neck: the shoulders slope up to the head (the head's bottom is at about 1.58).
				out.push_back({ { -0.055f, 1.60f }, { 0.055f, 1.60f }, { 0.22f, kShoulderY }, { -0.22f, kShoulderY } });

				out.push_back(Ellipse(0.0f, kHeadCenterY, kHeadWidth * 0.5f, kHeadHeight * 0.5f));
				return out;
			}();
			return parts;
		}

		bool Inside(const Polygon& polygon, float x, float y) {
			bool positive = false;
			bool negative = false;
			for (std::size_t i = 0; i < polygon.size(); ++i) {
				const auto& a = polygon[i];
				const auto& b = polygon[(i + 1) % polygon.size()];

				const float cross = (b.x - a.x) * (y - a.y) - (b.y - a.y) * (x - a.x);
				if (cross > 0.0f)
					positive = true;
				else if (cross < 0.0f)
					negative = true;
			}
			return !(positive && negative);
		}

		/// The dummy is the standard target on its own, centered on the middle of the NPC's height.
		constexpr float kDummyCenterY = kNpcHeight * 0.5f;

		struct Hit
		{
			int  score;
			bool gold;
		};

		/// What a shot that lands at (x, y) (x to the right, y up from the feet) scores when aiming at (aimX, aimY), or nothing if it missed.
		std::optional<Hit> Evaluate(bool humanoid, float x, float y, float aimX, float aimY) {
			if (!humanoid) {
				aimX = 0.0f;
				aimY = kDummyCenterY;
			} else if (!std::ranges::any_of(Silhouette(), [&](const Polygon& part) { return Inside(part, x, y); }))
				return std::nullopt;

			const auto ring = RingAtDistance(std::hypot(x - aimX, y - aimY));
			if (!ring)
				return std::nullopt;

			return Hit{ kRings - *ring, *ring < 2 };
		}

		// ---- State ----

		struct Roll
		{
			float radius;  // 0-1
			float angle;   // radians
		};

		enum Shape : int
		{
			kDummy,
			kHumanoid,
		};

		std::mt19937& Rng() {
			static std::mt19937 rng{ std::random_device{}() };
			return rng;
		}

		/// Where the beam was, as an offset in meters from the aim.
		struct TrailPoint
		{
			ImVec2 meters;
			int    tick;
		};

		struct Preview
		{
			/// The skill and attribute the Simulated tab tries, which start from the attacker's.
			bool  seeded = false;
			float skill = 25.0f;
			float maxAttribute = 100.0f;

			int shape = kHumanoid;

			/// Which tab the page last went by, to tell when another page has changed it.
			bool seenSimulated = false;

			/// Whether the shots are gone, until the next click: after the target has changed, they would be around an aim that is no longer there.
			bool cleared = false;

			/// Where the shooter aims: the first click on the target sets it, in meters (x to the right, y up from the feet).
			float aimX = 0.0f;
			float aimY = kHeadCenterY;

			/// Whether the target was held down: the shot is being held.
			bool  holding = false;
			bool  wasHolding = false;
			float holdSeconds = 0.0f;

			/// How many shots were out of view when last drawn, which the readout above the pane goes by.
			int outOfView = 0;

			/// What is on screen. It eases towards the value the settings call for, so a change glides instead of jumping.
			float shownScale = 0.0f;

			/// The zoom of the pane: held with the right button around `zoomFocus`, and eased like the scale.
			bool   zooming = false;
			float  shownZoom = 1.0f;
			ImVec2 zoomFocus{ 0.0f, 0.0f };

			std::vector<Roll> rolls;

			/// The beam of a concentration spell, wandering in real time.
			ConcentrationSpells::Details::Wander wander;
			float                                tickRemainder = 0.0f;
			int                                  tick = 0;
			std::deque<TrailPoint>               trail;

			/// The shots flying while the target is held, the newest last; the serial counts them, and only the last `shots` are kept.
			std::deque<TrailPoint> flow;
			int                    flowSerial = 0;
			float                  flowRemainder = 0.0f;

			Preview() {
				Reroll();
			}

			void Reroll() {
				std::uniform_real_distribution<float> radius(0.0f, 1.0f);
				std::uniform_real_distribution<float> angle(0.0f, 2.0f * std::numbers::pi_v<float>);

				rolls.resize(kMaxShots);
				for (auto& roll : rolls) {
					roll = { radius(Rng()), angle(Rng()) };
				}
			}

			bool Humanoid() const { return shape == kHumanoid; }

			/// Forgets the shots that were fired.
			void ClearShots() {
				cleared = true;
				trail.clear();
				wander = {};
				tickRemainder = 0.0f;
				flow.clear();
				flowSerial = 0;
				flowRemainder = 0.0f;
			}

			/// The aim goes to the middle of the head, or to the middle of the dummy.
			void ResetAim() {
				aimX = 0.0f;
				aimY = Humanoid() ? kHeadCenterY : kDummyCenterY;
			}
		};

		/// What the previews of all the pages have in common: the tab, and how the shots are fired. The skill and attribute they try, the shots and the statistics
		/// are each page's own.
		struct Shared
		{
			/// Current shows the attacker as they are, Simulated uses the skill and attribute the page was given.
			bool simulated = false;

			/// How many shots a click fires, or stay on screen with Interactive; for a concentration spell, how many steps of the stream.
			int shots = kShotsPerClick;

			bool interactive = true;
		};

		Shared& SharedSettings() {
			static Shared shared;
			return shared;
		}

		/// One for each kind and shooter, so every page keeps its own.
		Preview& PreviewFor(const void* key) {
			static std::unordered_map<const void*, Preview> previews;
			return previews[key];
		}

		struct Attacker
		{
			RE::NiPointer<RE::TESObjectREFR> keepAlive;
			RE::Actor*                       actor{ nullptr };
			bool                             selected{ false };
		};

		/// The actor selected in the console, or else the player.
		Attacker CurrentAttacker() {
			Attacker attacker;
			if (auto ref = RE::Console::GetSelectedRef(); ref) {
				if (auto* actor = ref->As<RE::Actor>(); actor) {
					attacker.keepAlive = std::move(ref);
					attacker.actor = actor;
					attacker.selected = true;
					return attacker;
				}
			}
			attacker.actor = RE::PlayerCharacter::GetSingleton();
			return attacker;
		}

		/// The colors of a slider that stands for something: a skill or an attribute.
		struct Tint
		{
			ImU32 background;
			ImU32 hovered;
			ImU32 active;
			ImU32 grab;
		};

		// Muted takes on the colors of the game's own HUD and skill constellations.
		constexpr Tint kRedTint{ IM_COL32(96, 34, 32, 235), IM_COL32(124, 44, 40, 235), IM_COL32(146, 54, 48, 235), IM_COL32(196, 76, 68, 255) };
		constexpr Tint kBlueTint{ IM_COL32(30, 58, 104, 235), IM_COL32(40, 76, 132, 235), IM_COL32(50, 92, 154, 235), IM_COL32(82, 138, 206, 255) };
		constexpr Tint kGreenTint{ IM_COL32(34, 80, 38, 235), IM_COL32(46, 102, 50, 235), IM_COL32(56, 120, 60, 235), IM_COL32(92, 168, 96, 255) };
		constexpr Tint kVioletTint{ IM_COL32(66, 42, 104, 235), IM_COL32(84, 54, 130, 235), IM_COL32(100, 66, 150, 235), IM_COL32(150, 112, 212, 255) };

		bool TintedSlider(const char* id, float* value, float min, float max, const char* format, const Tint& tint) {
			PushStyleColor(ImGuiCol_FrameBg, tint.background);
			PushStyleColor(ImGuiCol_FrameBgHovered, tint.hovered);
			PushStyleColor(ImGuiCol_FrameBgActive, tint.active);
			PushStyleColor(ImGuiCol_SliderGrab, tint.grab);
			PushStyleColor(ImGuiCol_SliderGrabActive, tint.grab);
			const bool changed = SliderFloat(id, value, min, max, format);
			PopStyleColor(5);
			return changed;
		}

		ImU32 Hue(float hue, int alpha = 255) {
			const float h = std::fmod(hue, 1.0f) * 6.0f;
			const float x = 1.0f - std::fabs(std::fmod(h, 2.0f) - 1.0f);
			float       r = 0, g = 0, b = 0;
			switch (static_cast<int>(h)) {
			case 0:
				r = 1;
				g = x;
				break;
			case 1:
				r = x;
				g = 1;
				break;
			case 2:
				g = 1;
				b = x;
				break;
			case 3:
				g = x;
				b = 1;
				break;
			case 4:
				r = x;
				b = 1;
				break;
			default:
				r = 1;
				b = x;
				break;
			}
			return IM_COL32(static_cast<int>((0.25f + 0.75f * r) * 255), static_cast<int>((0.25f + 0.75f * g) * 255), static_cast<int>((0.25f + 0.75f * b) * 255), alpha);
		}

		/// Where, as an offset on the target plane in meters, a deviation of the given angles ends up at the distance.
		ImVec2 OnTarget(const RE::NiPoint2& angles, float distance) {
			const float length = angles.Length();
			if (length <= 0.0f)
				return { 0.0f, 0.0f };

			const float meters = distance * std::tan(std::min(length, 1.2f));
			return { angles.y / length * meters, angles.x / length * meters };
		}

		// ---- Drawing ----

		/// Fills the silhouette with the rings around the aim, a row of pixels at a time: ImGui can't clip to a shape, so each row is cut into the stretches
		/// that lie within the silhouette and, within those, into the ones between the rings. Only rows from `top` to `bottom` are drawn.
		void FillSilhouetteWithRings(ImDrawList* drawList, const std::vector<std::vector<ImVec2>>& parts, const ImVec2& aim, float pxPerMeter, float top, float bottom) {
			constexpr float rowHeight = 2.0f;

			float first = FLT_MAX;
			float last = -FLT_MAX;
			for (const auto& part : parts) {
				for (const auto& p : part) {
					first = std::min(first, p.y);
					last = std::max(last, p.y);
				}
			}
			first = std::max(first, top);
			last = std::min(last, bottom);

			using Span = std::pair<float, float>;
			std::vector<Span>  spans;
			std::vector<Span>  merged;
			std::vector<float> cuts;

			for (float y = first; y < last; y += rowHeight) {
				const float row = y + rowHeight * 0.5f;
				const float dy = row - aim.y;

				spans.clear();
				for (const auto& part : parts) {
					float left = FLT_MAX;
					float right = -FLT_MAX;
					for (std::size_t i = 0; i < part.size(); ++i) {
						const auto& a = part[i];
						const auto& b = part[(i + 1) % part.size()];
						if ((a.y <= row && b.y > row) || (b.y <= row && a.y > row)) {
							const float x = a.x + (row - a.y) / (b.y - a.y) * (b.x - a.x);
							left = std::min(left, x);
							right = std::max(right, x);
						}
					}
					if (left < right)
						spans.push_back({ left, right });
				}

				std::ranges::sort(spans);
				merged.clear();
				for (const auto& span : spans) {
					if (!merged.empty() && span.first <= merged.back().second)
						merged.back().second = std::max(merged.back().second, span.second);
					else
						merged.push_back(span);
				}

				for (const auto& [from, to] : merged) {
					cuts.assign({ from, to });
					for (int ring = 0; ring < kRings; ++ring) {
						const float r = RingRadius(ring) * pxPerMeter;
						if (std::fabs(dy) < r) {
							const float half = std::sqrt(r * r - dy * dy);
							for (const float x : { aim.x - half, aim.x + half }) {
								if (x > from && x < to)
									cuts.push_back(x);
							}
						}
					}
					std::ranges::sort(cuts);

					for (std::size_t i = 0; i + 1 < cuts.size(); ++i) {
						if (cuts[i + 1] - cuts[i] < 0.01f)
							continue;

						const float middle = (cuts[i] + cuts[i + 1]) * 0.5f;
						const auto  ring = RingAtDistance(std::hypot(middle - aim.x, dy) / pxPerMeter);
						const ImU32 color = ring ? StyleOfRing(*ring).color : IM_COL32(72, 74, 76, 255);
						AddRectFilled(drawList, ImVec2{ cuts[i], y }, ImVec2{ cuts[i + 1], y + rowHeight + 0.5f }, color, 0.0f, 0);
					}

					// The lines of the rings, like those of the dummy: where each ring runs through this row, from where it was in the row above to where it is in the
					// row below, so that the nearly flat parts of a ring are joined up too.
					for (int ring = 0; ring < kRings; ++ring) {
						const float r = RingRadius(ring) * pxPerMeter;
						const float dyTop = y - aim.y;
						const float dyBottom = y + rowHeight - aim.y;
						if (std::fabs(dyTop) >= r && std::fabs(dyBottom) >= r)
							continue;

						const auto  half = [&](float offset) { return std::fabs(offset) < r ? std::sqrt(r * r - offset * offset) : 0.0f; };
						const float nearer = std::min(half(dyTop), half(dyBottom));
						const float farther = std::max(half(dyTop), half(dyBottom));
						for (const float side : { -1.0f, 1.0f }) {
							float low = aim.x + side * (side < 0.0f ? farther : nearer);
							float high = aim.x + side * (side < 0.0f ? nearer : farther);
							if (high - low < 1.4f) {
								const float middle = (low + high) * 0.5f;
								low = middle - 0.7f;
								high = middle + 0.7f;
							}
							low = std::max(low, from);
							high = std::min(high, to);
							if (high > low)
								AddRectFilled(drawList, ImVec2{ low, y }, ImVec2{ high, y + rowHeight + 0.5f }, IM_COL32(0, 0, 0, 120), 0.0f, 0);
						}
					}
				}
			}
		}

		/// Draws the target with its rings around (ringX, ringY) on the target. `anchorScreen` is where the point (anchorX, anchorY) on the target is on the screen, and
		/// `pxPerMeter` places the rest. Only rows from `top` to `bottom` need drawing.
		void DrawFigure(ImDrawList* drawList, bool humanoid, float ringX, float ringY, float anchorX, float anchorY, const ImVec2& anchorScreen, float pxPerMeter, float top, float bottom, bool scores = false) {
			const auto   toScreen = [&](float x, float y) { return ImVec2{ anchorScreen.x + (x - anchorX) * pxPerMeter, anchorScreen.y - (y - anchorY) * pxPerMeter }; };
			const ImVec2 aim = toScreen(ringX, ringY);

			if (humanoid) {
				std::vector<std::vector<ImVec2>> parts;
				for (const auto& part : Silhouette()) {
					auto& screen = parts.emplace_back();
					for (const auto& p : part) {
						screen.push_back(toScreen(p.x, p.y));
					}
				}

				FillSilhouetteWithRings(drawList, parts, aim, pxPerMeter, top, bottom);
				for (const auto& screen : parts) {
					AddPolyline(drawList, screen.data(), static_cast<int>(screen.size()), IM_COL32(0, 0, 0, 120), ImDrawFlags_Closed, 1.0f);
				}
				return;
			}

			// Ten rings in the colors of the zones, two of each: gold, red, blue, black and white from the middle out.
			const ImVec2 center = toScreen(0.0f, kDummyCenterY);
			for (int i = kRings - 1; i >= 0; --i) {
				const float r = RingRadius(i) * pxPerMeter;
				AddCircleFilled(drawList, center, r, StyleOfRing(i).color, 64);
				AddCircle(drawList, center, r, IM_COL32(0, 0, 0, 110), 64, 1.0f);
			}

			// What a shot in each ring scores, in the middle of the ring's width above the center, once the rings are wide enough to hold it. The middle ring's is in the middle.
			if (scores) {
				for (int i = 0; i < kRings; ++i) {
					const float inner = i == 0 ? 0.0f : RingRadius(i - 1) * pxPerMeter;
					const float outer = RingRadius(i) * pxPerMeter;
					const float width = i == 0 ? outer * 2.0f : outer - inner;
					if (width < GetFontSize() * 0.7f)
						continue;

					const std::string text = std::to_string(kRings - i);
					const ImVec2      size = CalcTextSize(text.c_str());
					const float       offset = i == 0 ? 0.0f : (inner + outer) * 0.5f;
					const bool        lightBackground = i < 2 || i >= 8;
					AddText(drawList, ImVec2{ center.x - size.x * 0.5f, center.y - offset - size.y * 0.5f }, lightBackground ? IM_COL32(30, 30, 30, 255) : IM_COL32(240, 240, 240, 255), text.c_str());
				}
			}
		}

		/// The aim point stands out from the shots: a bright dot with a dark ring.
		void DrawAimMark(ImDrawList* drawList, const ImVec2& aim) {
			AddCircleFilled(drawList, aim, 6.5f, IM_COL32(0, 0, 0, 255), 20);
			AddCircleFilled(drawList, aim, 4.5f, IM_COL32(0, 229, 255, 255), 20);
			AddCircle(drawList, aim, 7.5f, IM_COL32(255, 255, 255, 220), 20, 1.2f);
		}

		/// What a shot looks like: arrows and bolts are small, spells big and glowing like a firebolt.
		enum class ShotLook
		{
			kArrow,
			kBolt,
			kSpell,
		};

		ShotLook LookOf(const Options::AttackKind& kind) {
			if (kind.magic)
				return ShotLook::kSpell;
			return &kind == &Options::AttackKind::kCrossbow ? ShotLook::kBolt : ShotLook::kArrow;
		}

		/// The color of the glow that marks shots out of view: that of the shots themselves.
		ImU32 GlowOf(ShotLook look, int alpha) {
			switch (look) {
			case ShotLook::kArrow:
				return IM_COL32(228, 208, 150, alpha);
			case ShotLook::kBolt:
				return IM_COL32(176, 188, 202, alpha);
			default:
				return IM_COL32(255, 140, 40, alpha);
			}
		}

		/// `scale` shrinks the dots when there are many, and `fade` (0-1) makes a shot see-through.
		void DrawShot(ImDrawList* drawList, const ImVec2& at, ShotLook look, float scale, float fade) {
			const auto alpha = [&](int value) { return static_cast<int>(static_cast<float>(value) * fade); };
			switch (look) {
			case ShotLook::kArrow:
				AddCircleFilled(drawList, at, 4.0f * scale, IM_COL32(228, 208, 150, alpha(255)), 12);
				AddCircle(drawList, at, 4.0f * scale, IM_COL32(48, 32, 12, alpha(230)), 12, 1.2f);
				break;
			case ShotLook::kBolt:
				AddCircleFilled(drawList, at, 4.0f * scale, IM_COL32(176, 188, 202, alpha(255)), 12);
				AddCircle(drawList, at, 4.0f * scale, IM_COL32(20, 26, 34, alpha(230)), 12, 1.2f);
				break;
			case ShotLook::kSpell:
				{
					const float radius = 6.0f * scale;
					AddCircleFilled(drawList, at, radius * 1.9f, IM_COL32(255, 110, 30, alpha(40)), 24);
					AddCircleFilled(drawList, at, radius * 1.4f, IM_COL32(255, 140, 40, alpha(90)), 24);
					AddCircleFilled(drawList, at, radius, IM_COL32(255, 186, 70, alpha(255)), 20);
					AddCircleFilled(drawList, at, radius * 0.5f, IM_COL32(255, 246, 206, alpha(255)), 16);
					break;
				}
			}
		}

		/// The bar of a hold: filled up to `fraction`, with `text` across it, outlined so that it reads on the fill as well as off it.
		void DrawBar(ImDrawList* drawList, const ImVec2& position, const ImVec2& size, float fraction, ImU32 fill, const char* text) {
			const ImVec2 end{ position.x + size.x, position.y + size.y };
			AddRectFilled(drawList, position, end, IM_COL32(36, 38, 42, 255), 3.0f, 0);
			if (fraction > 0.0f) {
				AddRectFilled(drawList, position, ImVec2{ position.x + size.x * fraction, end.y }, fill, 3.0f, 0);
			}
			AddRect(drawList, position, end, IM_COL32(255, 255, 255, 60), 3.0f, 0, 1.0f);

			const ImVec2 extent = CalcTextSize(text);
			const ImVec2 at{ position.x + (size.x - extent.x) * 0.5f, position.y + (size.y - extent.y) * 0.5f };
			for (const ImVec2& shift : { ImVec2{ -1.0f, 0.0f }, ImVec2{ 1.0f, 0.0f }, ImVec2{ 0.0f, -1.0f }, ImVec2{ 0.0f, 1.0f } }) {
				AddText(drawList, ImVec2{ at.x + shift.x, at.y + shift.y }, IM_COL32(0, 0, 0, 210), text);
			}
			AddText(drawList, at, IM_COL32(255, 255, 255, 255), text);
		}

		/// A four-pointed star with a small ring in the middle, like the crosshair of the HUD; `radius` reaches its tips.
		void DrawCrosshair(ImDrawList* drawList, const ImVec2& center, float radius) {
			constexpr float pinch = 0.17f;
			const float     k = radius * pinch;
			const ImVec2    star[] = {
				{ center.x, center.y - radius },
				{ center.x + k, center.y - k },
				{ center.x + radius, center.y },
				{ center.x + k, center.y + k },
				{ center.x, center.y + radius },
				{ center.x - k, center.y + k },
				{ center.x - radius, center.y },
				{ center.x - k, center.y - k },
			};
			AddPolyline(drawList, star, 8, IM_COL32(0, 0, 0, 220), ImDrawFlags_Closed, 3.5f);
			AddPolyline(drawList, star, 8, IM_COL32(214, 214, 204, 255), ImDrawFlags_Closed, 1.6f);
			AddCircle(drawList, center, radius * 0.2f, IM_COL32(0, 0, 0, 220), 16, 3.0f);
			AddCircle(drawList, center, radius * 0.2f, IM_COL32(214, 214, 204, 255), 16, 1.4f);
		}

		void RenderReference(const Preview& preview) {
			const ImVec2 origin = GetCursorScreenPos();
			const ImVec2 corner{ origin.x + kReferenceSide, origin.y + kReferenceSide };
			InvisibleButton("##Reference", ImVec2(kReferenceSide, kReferenceSide));

			const float crosshairs = kCrosshairMatchesHeightAt / SharedDistance();
			ShowTooltip(fmt::format("{} apparent size at {:.0f} m: {:.1f} crosshairs tall.", preview.Humanoid() ? "An NPC's" : "The target's", SharedDistance(), crosshairs).c_str());

			// The view is anchored on the middle of the body from the distance where the crosshair is as tall as the NPC, and slides up to the head by the distance where it is as big as the head.
			const float slide = std::clamp(std::log(kCrosshairMatchesHeightAt / SharedDistance()) / std::log(kCrosshairMatchesHeightAt / kHeadMatchesCrosshairAt), 0.0f, 1.0f);
			const float anchorY = preview.Humanoid() ? kNpcHeight * 0.5f + (kHeadCenterY - kNpcHeight * 0.5f) * slide : kDummyCenterY;

			auto*        drawList = GetWindowDrawList();
			const ImVec2 center{ origin.x + kReferenceSide * 0.5f, origin.y + kReferenceSide * 0.5f };

			PushClipRect(drawList, origin, corner, true);
			AddRect(drawList, origin, corner, IM_COL32(255, 255, 255, 40), 0.0f, 0, 1.0f);
			DrawFigure(drawList, preview.Humanoid(), 0.0f, kHeadCenterY, 0.0f, anchorY, center, kCrosshairPx * crosshairs / kNpcHeight, origin.y, corner.y);
			DrawCrosshair(drawList, center, kCrosshairPx * 0.5f);
			PopClipRect(drawList);
		}
	}

	float PreviewPaneWidth() {
		return kMaxCanvasSide + kPaneChrome + GetStyle()->ScrollbarSize + GetStyle()->WindowPadding.x * 2;
	}

	void RenderSpreadPreview(const KindContext& kind) {
		const auto& attack = kind.cfg;
		const auto& info = kind.info;
		auto&       preview = PreviewFor(&attack);
		auto&       shared = SharedSettings();

		const float dt = std::clamp(GetIO()->DeltaTime, 0.0f, 0.1f);

		SeparatorText("Preview");

		// ---- Attacker ----

		const auto  attacker = CurrentAttacker();
		auto* const actor = attacker.actor;
		if (!actor) {
			TextDisabled("No attacker: the game isn't loaded.");
			return;
		}

		const auto        skillValue = Details::SkillForSource(attack.skills.accuracy, nullptr, info.magic, actor);
		const char* const skillName = RE::ActorValueList::GetActorValueName(skillValue);
		const float       actorSkill = actor->AsActorValueOwner()->GetActorValue(skillValue);
		const auto        attribute = info.magic ? RE::ActorValue::kMagicka : RE::ActorValue::kStamina;
		const float       actorMaxAttribute = actor->GetActorValueMax(attribute);
		const char* const name = actor->GetDisplayFullName();

		if (!preview.seeded) {
			preview.seeded = true;
			preview.skill = actorSkill;
			preview.maxAttribute = actorMaxAttribute;
		}

		const float spacing = GetStyle()->ItemSpacing.x;
		const float leftWidth = std::max(200.0f, UsableWidth() - kReferenceSide - spacing);

		// The skill is named after what it is for this kind (Archery, Destruction, Enchanting...), and tinted like it: red for the warrior's, blue for magic, violet for enchanting.
		const std::string skillFormat = fmt::format("{} %.0f", skillName && *skillName ? skillName : "Skill");
		const std::string attributeFormat = info.magic ? "Max Magicka %.0f" : "Max Stamina %.0f";
		const Tint&       skillTint = skillValue == RE::ActorValue::kArchery ? kRedTint : skillValue == RE::ActorValue::kEnchanting ? kVioletTint :
		                                                                                                                              kBlueTint;
		const Tint&       attributeTint = info.magic ? kBlueTint : kGreenTint;

		// The settings take the left, and the reference with the distance slider below it the right, up in the corner.
		BeginChild("##PreviewControls", ImVec2(leftWidth, 0), ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_NoScrollbar);
		const float rowWidth = GetContentRegionAvail().x;
		const float sliderWidth = attack.fatigue.enabled ? (rowWidth - spacing) * 0.5f : rowWidth;

		// The sliders of both tabs, side by side: they only show the attacker's values in Current and are edited in Simulated.
		const auto renderValues = [&](bool editable) {
			DisabledIf off{ !editable };

			float skill = editable ? preview.skill : actorSkill;
			SetNextItemWidth(sliderWidth);
			if (TintedSlider("##PreviewSkill", &skill, 0.0f, 100.0f, skillFormat.c_str(), skillTint) && editable) {
				preview.skill = skill;
			}
			if (editable) {
				ShowTooltip("Try a different skill level.");
			}

			if (attack.fatigue.enabled) {
				SameLine();
				float maxAttribute = editable ? preview.maxAttribute : actorMaxAttribute;
				SetNextItemWidth(sliderWidth);
				if (TintedSlider("##PreviewMaxAttribute", &maxAttribute, 0.0f, 500.0f, attributeFormat.c_str(), attributeTint) && editable) {
					preview.maxAttribute = maxAttribute;
				}
				if (editable) {
					ShowTooltip(info.magic ? "More max Magicka lets a charge be held longer before tiring." : "More max Stamina lets a draw be held longer before tiring.");
				}
			}
		};

		// The first tab is the attacker as they are now, named after them: whoever is selected in the console, or the player. The second one takes any skill and max Stamina or Magicka.
		if (BeginTabBar("##PreviewMode")) {
			const std::string currentLabel = fmt::format("{}###Current", name && *name ? name : "?");

			// Another page may have switched the tab, in which case this one is made to follow. That takes a frame, and meanwhile what is open must not be taken
			// for a choice.
			const bool follow = preview.seenSimulated != shared.simulated;
			preview.seenSimulated = shared.simulated;
			const bool currentOpen = BeginTabItem(currentLabel.c_str(), nullptr, follow && !shared.simulated ? ImGuiTabItemFlags_SetSelected : 0);
			ShowTooltip(attacker.selected ? "Actor selected in the Console." : "You. Tip: select an actor in the Console to preview them instead.");
			if (currentOpen) {
				if (!follow)
					shared.simulated = false;
				renderValues(false);
				EndTabItem();
			}

			const bool simulatedOpen = BeginTabItem("Simulated", nullptr, follow && shared.simulated ? ImGuiTabItemFlags_SetSelected : 0);
			ShowTooltip(info.magic ? "Try any skill and max Magicka." : "Try any skill and max Stamina.");
			if (simulatedOpen) {
				if (!follow)
					shared.simulated = true;
				renderValues(true);
				EndTabItem();
			}
			EndTabBar();
		}

		AlignTextToFramePadding();
		TextUnformatted(info.concentration ? "Steps" : "Shots");
		SameLine();
		SetNextItemWidth(100.0f);
		SliderInt("##PreviewShots", &shared.shots, 1, kMaxShots, "%d");
		ShowTooltip(info.concentration ?
		                (shared.interactive ? "How many steps of the beam stay on screen; older ones fade away." : "How many points the beam visits when shown at once.") :
		                (shared.interactive ? "How many shots stay on screen while the target is held; older ones fade away." : "How many shots a click fires."));
		SameLine();
		Checkbox("Interactive", &shared.interactive);
		ShowTooltip(info.concentration ? "On: hold the target to cast live. Off: show a whole beam at once." : "On: hold the target to fire a steady flow of shots. Off: a click fires a batch at once.");

		AlignTextToFramePadding();
		TextUnformatted("Target");
		SameLine();
		if (RadioButton("Dummy", &preview.shape, kDummy)) {
			preview.ResetAim();
			preview.ClearShots();
		}
		ShowTooltip("A plain archery target, half as tall as an NPC.");
		SameLine();
		if (RadioButton("Humanoid", &preview.shape, kHumanoid)) {
			preview.ResetAim();
			preview.ClearShots();
		}
		ShowTooltip("A target shaped like an average NPC.");
		EndChild();

		SameLine();
		BeginGroup();
		RenderReference(preview);
		SetNextItemWidth(kReferenceSide);
		SliderFloat("##PreviewDistance", &SharedDistance(), kMinDistance, kMaxDistance, "Distance %.0f m");
		ShowTooltip("How far away the target is.");
		EndGroup();

		// ---- Model: the very same functions the shots use ----

		preview.holdSeconds = preview.holding ? preview.holdSeconds + dt : 0.0f;

		// Each part scales with its own skill. The Simulated tab has one skill slider, which they all follow.
		const auto skillOf = [&](Options::SkillSource source) {
			return shared.simulated ? preview.skill : actor->AsActorValueOwner()->GetActorValue(Details::SkillForSource(source, nullptr, info.magic, actor));
		};
		const float maxAttribute = shared.simulated ? preview.maxAttribute : actorMaxAttribute;
		const float effectiveAccuracy = skillOf(attack.skills.accuracy) * attack.accuracy.skillMultiplier;
		const float effectivePrecision = skillOf(attack.skills.precision) * attack.precision.skillMultiplier;
		const float effectiveRetarget = skillOf(attack.skills.retarget) * attack.retarget.skillMultiplier;

		const float maxSpread = attack.Aims() ? Details::MaxSpreadFor(Details::SkillFactorFor(attack.accuracy.bounds, effectiveAccuracy)) : 0.0f;
		const float focus = attack.Aims() && attack.precision.enabled ? Details::SkillFactorFor(attack.precision.bounds, effectivePrecision) : 0.5f;
		// Without Retargeting the beam picks its next point freely.
		const float maxStep = attack.retarget.enabled ? maxSpread * Details::SkillFactorFor(attack.retarget.bounds, effectiveRetarget) : std::numeric_limits<float>::max();

		const float threshold = attack.fatigue.enabled ? Fatigue::Details::Threshold(attack.fatigue, maxAttribute) : 0.0f;
		const auto  hold = Fatigue::Hold(attack.steadying, attack.fatigue, threshold, preview.holdSeconds);

		if (preview.shownScale <= 0.0f) {
			preview.shownScale = hold.scale;
		}
		preview.shownScale += (hold.scale - preview.shownScale) * (1.0f - std::exp(-kMoveRate * dt));

		// Interactive: a concentration spell is cast live as a stream, and anything else fires a flow of shots while the target is held.
		const bool stream = info.concentration && shared.interactive;
		const bool flow = !info.concentration && shared.interactive;
		const int  trailTicks = shared.shots * kLegTicks;
		const bool humanoid = preview.Humanoid();

		// Where the shots land, as offsets in meters from the aim. A concentration stream picks each point from the previous one, as the beam does when it wanders.
		std::vector<ImVec2> hits;
		std::vector<float>  weights;  // How much each shot still shows: what is held fades with age, and is whole again once let go; the rest is fully there.
		std::vector<float>  ages;     // How far along a stream's trail each point is, 0 for the newest and 1 for the oldest, for its color.
		if (stream) {
			// The beam wanders while the target is held, like a spell being cast: a tick at a time, with the same code as in the game. Each hold
			// starts afresh from the aim, and what the beam did stays on screen, fading as it goes on, when the button is let go.
			if (preview.holding && !preview.wasHolding) {
				preview.wander = {};
				preview.trail.clear();
				preview.tickRemainder = 0.0f;
			}
			preview.tickRemainder += preview.holding ? dt * kTicksPerSecond : 0.0f;
			const int ticks = std::min(static_cast<int>(preview.tickRemainder), 10);
			preview.tickRemainder -= std::floor(preview.tickRemainder);

			std::uniform_real_distribution<float> unit(0.0f, 1.0f);
			for (int i = 0; i < ticks; ++i) {
				const bool moved = preview.wander.Advance(maxSpread, [&](const RE::NiPoint2& from) {
					const auto candidate = Details::SpreadOffset(maxSpread, focus, unit(Rng()), unit(Rng()) * 2.0f * std::numbers::pi_v<float>);
					return ConcentrationSpells::Details::BoundedStep(from, candidate, maxStep);
				});
				if (moved) {
					++preview.tick;
					const RE::NiPoint2 scaled{ preview.wander.current.x * preview.shownScale, preview.wander.current.y * preview.shownScale };
					preview.trail.push_back({ OnTarget(scaled, SharedDistance()), preview.tick });
				}
			}
			while (!preview.trail.empty() && preview.trail.front().tick + trailTicks < preview.tick) {
				preview.trail.pop_front();
			}

			for (const auto& point : preview.trail) {
				const float age = std::clamp(static_cast<float>(preview.tick - point.tick) / static_cast<float>(trailTicks), 0.0f, 1.0f);
				hits.push_back(point.meters);
				ages.push_back(age);
				weights.push_back(preview.holding ? 1.0f - age : 1.0f);
			}
		} else if (flow) {
			preview.trail.clear();
			preview.wander = {};

			// Each hold starts afresh with a shot at once, then the shots fly at a steady rate. They fade as newer ones follow, and are gone after as many
			// as were asked for with the keys at the start. What was shot stays on screen, as it is, when the button is let go.
			if (preview.holding && !preview.wasHolding) {
				preview.flow.clear();
				preview.flowRemainder = 1.0f;
			}
			preview.flowRemainder += preview.holding ? dt * kShotsPerSecond : 0.0f;
			const int fired = std::min(static_cast<int>(preview.flowRemainder), 10);
			preview.flowRemainder -= std::floor(preview.flowRemainder);

			std::uniform_real_distribution<float> unit(0.0f, 1.0f);
			for (int i = 0; i < fired; ++i) {
				const auto offset = attack.Aims() ? Details::SpreadOffset(maxSpread, focus, unit(Rng()), unit(Rng()) * 2.0f * std::numbers::pi_v<float>) : RE::NiPoint2{ 0.0f, 0.0f };
				preview.flow.push_back({ OnTarget({ offset.x * preview.shownScale, offset.y * preview.shownScale }, SharedDistance()), ++preview.flowSerial });
			}
			while (!preview.flow.empty() && preview.flow.front().tick + shared.shots <= preview.flowSerial) {
				preview.flow.pop_front();
			}

			for (const auto& shot : preview.flow) {
				hits.push_back(shot.meters);
				weights.push_back(preview.holding ? 1.0f - std::clamp(static_cast<float>(preview.flowSerial - shot.tick) / static_cast<float>(shared.shots), 0.0f, 1.0f) : 1.0f);
			}
		} else {
			// Out of Interactive, so that it doesn't show where the beam or the flow was before.
			preview.trail.clear();
			preview.wander = {};
			preview.flow.clear();

			const int                 count = std::clamp(shared.shots, 1, kMaxShots);
			std::vector<RE::NiPoint2> offsets(count);
			RE::NiPoint2              previous{ 0.0f, 0.0f };
			for (int i = 0; i < count; ++i) {
				const auto& roll = preview.rolls[i];
				const auto  candidate = attack.Aims() ? Details::SpreadOffset(maxSpread, focus, roll.radius, roll.angle) : RE::NiPoint2{ 0.0f, 0.0f };
				offsets[i] = info.concentration ? ConcentrationSpells::Details::BoundedStep(previous, candidate, maxStep) : candidate;
				previous = offsets[i];
			}
			for (const auto& offset : offsets) {
				hits.push_back(OnTarget({ offset.x * preview.shownScale, offset.y * preview.shownScale }, SharedDistance()));
			}
			if (preview.cleared) {
				hits.clear();
			}
		}

		preview.wasHolding = preview.holding;

		// ---- Readout ----

		const int                   total = static_cast<int>(hits.size());
		int                         onTarget = 0;
		int                         gold = 0;
		int                         score = 0;
		std::array<int, kRings + 1> scoreCounts{};  // How many shots scored each, 0 for a miss.
		for (const auto& hit : hits) {
			// Only what lands on the target counts. y is up on the target and down on the screen.
			if (const auto result = Evaluate(humanoid, preview.aimX + hit.x, preview.aimY - hit.y, preview.aimX, preview.aimY); result) {
				++onTarget;
				score += result->score;
				++scoreCounts[static_cast<std::size_t>(result->score)];
				if (result->gold)
					++gold;
			} else {
				++scoreCounts[0];
			}
		}

		const float widestNow = maxSpread * preview.shownScale;

		// ---- Status line ----
		// On the left, how long the hold has gone on and what it does to the spread; on the right, how the shots do. Each has its tooltip, which also shows while the target is held.

		const float side = std::clamp(UsableWidth(), 200.0f, kMaxCanvasSide);
		const bool  steadies = attack.steadying.enabled;
		const bool  tires = attack.fatigue.enabled;
		const auto  angleText = [&](float scale) { return fmt::format("{:.2f}° ({:.2f} m)", RE::rad_to_deg(maxSpread * scale), SharedDistance() * std::tan(std::min(maxSpread * scale, 1.2f))); };
		const auto  percent = [&](int n) { return total > 0 ? 100.0f * static_cast<float>(n) / static_cast<float>(total) : 0.0f; };

		const float lineStart = GetCursorPosX();
		AlignTextToFramePadding();

		if (steadies || tires) {
			// The bar shows what the hold has done to the spread so far, from the spread of x1.00 at its start: green for what steadying has taken off, red for what
			// fatigue has put on. Fatigue first takes the steadying back, so the green runs out before the red starts to fill. Full is the max, which is the value
			// after twice the half duration.
			auto noFatigue = attack.fatigue;
			noFatigue.enabled = false;
			// Fatigue ends the steadying where it sets in, so steadying doesn't get to its max if that comes first.
			const float steadyingEnd = tires ? std::min(2.0f * attack.steadying.halfDuration, threshold) : 2.0f * attack.steadying.halfDuration;
			const float maxBonus = steadies ? 1.0f - Fatigue::Hold(attack.steadying, noFatigue, 0.0f, steadyingEnd).scale : 0.0f;
			const float maxDebuff = tires ? Fatigue::Hold(attack.steadying, attack.fatigue, threshold, threshold + 2.0f * attack.fatigue.halfDuration).scale - 1.0f : 0.0f;

			const bool  debuff = hold.scale > 1.0f;
			const float amount = debuff ? hold.scale - 1.0f : 1.0f - hold.scale;
			const float limit = debuff ? maxDebuff : maxBonus;

			// Without steadying there is no bonus to show before fatigue sets in, so the bar counts the time towards it, in white.
			const bool  counting = !steadies && tires && !debuff;
			const float fraction = counting ? std::clamp(preview.holdSeconds / std::max(threshold, 0.01f), 0.0f, 1.0f) : limit > 0.0f ? std::clamp(amount / limit, 0.0f, 1.0f) :
			                                                                                                                            0.0f;

			const ImVec2 barSize{ kHoldBarWidth, GetFrameHeight() };
			DrawBar(GetWindowDrawList(), GetCursorScreenPos(), barSize, fraction, counting ? IM_COL32(236, 236, 236, 255) : debuff ? kRedTint.grab :
			                                                                                                                         kGreenTint.grab,
			        fmt::format("{:.1f}s", preview.holdSeconds).c_str());
			Dummy(barSize);

			std::string tip = "How long the target has been held.";
			if (steadies) {
				tip += fmt::format("\n\nSteadying builds up from the start and is at its max after {:.1f}s.", steadyingEnd);
			}
			if (tires) {
				tip += fmt::format("{}Fatigue sets in after {:.1f}s and is at its max {:.1f}s later.", steadies ? "\n" : "\n\n", threshold, 2.0f * attack.fatigue.halfDuration);
			}
			ShowTooltip(tip.c_str(), true);

			SameLine();
			Text("%s", fmt::format("Spread x{:.2f}", hold.scale).c_str());
			if (attack.Aims()) {
				std::string spreadTip = fmt::format("Now: x{:.2f}, widest miss {}", hold.scale, angleText(hold.scale));
				if (steadies) {
					spreadTip += fmt::format("\nMax steadying: x{:.2f}, widest miss {}", 1.0f - maxBonus, angleText(1.0f - maxBonus));
				}
				if (tires) {
					spreadTip += fmt::format("\nMax fatigue: x{:.2f}, widest miss {}", 1.0f + maxDebuff, angleText(1.0f + maxDebuff));
				}
				ShowTooltip(spreadTip.c_str(), true);
			} else {
				ShowTooltip(info.concentration ? "Aiming is off: the beam goes exactly where aimed." : "Aiming is off: shots go exactly where aimed.", true);
			}
			SameLine();
		}

		// The two on the right end where the pane ends.
		const std::string hitsText = total > 0 ? fmt::format("Hits {:.0f}%", percent(onTarget)) : "Hits -";
		const std::string scoreText = total > 0 ? fmt::format("Score {:.1f}", static_cast<float>(score) / static_cast<float>(total)) : "Score -";
		const float       gap = GetStyle()->ItemSpacing.x * 2.0f;
		const float       rightWidth = CalcTextSize(hitsText.c_str()).x + gap + CalcTextSize(scoreText.c_str()).x;
		SetCursorPosX(std::max(GetCursorPosX(), lineStart + side - rightWidth));

		Text("%s", hitsText.c_str());
		if (total > 0) {
			ShowTooltip(fmt::format("Hits: {} of {} ({:.0f}%)\nMisses: {} ({:.0f}%)\nGold: {} ({:.0f}%)\nOut of view: {}", onTarget, total, percent(onTarget), total - onTarget, percent(total - onTarget), gold, percent(gold), preview.outOfView).c_str(), true);
		} else {
			ShowTooltip("Nothing to count yet.", true);
		}
		SameLine(0.0f, gap);
		Text("%s", scoreText.c_str());
		if (total > 0) {
			// Each score that came up, and how many times: "10 x 2".
			std::string tip;
			for (int ring = kRings; ring >= 0; --ring) {
				if (scoreCounts[static_cast<std::size_t>(ring)] > 0) {
					tip += fmt::format("{}{} x {}", tip.empty() ? "" : "\n", ring, scoreCounts[static_cast<std::size_t>(ring)]);
				}
			}
			ShowTooltip(tip.c_str(), true);
		} else {
			ShowTooltip("Nothing to count yet.", true);
		}

		// ---- Canvas ----

		const ImVec2 origin = GetCursorScreenPos();
		const ImVec2 corner{ origin.x + side, origin.y + side };
		const ImVec2 paneCenter{ origin.x + side * 0.5f, origin.y + side * 0.5f };
		InvisibleButton("##Canvas", ImVec2(side, side));
		{
			std::string help = stream ? "Click and hold to cast" : flow ? "Click and hold to fire" :
			                                                              fmt::format("Click to fire {} shots", shared.shots);
			if (steadies || tires) {
				help += steadies && tires ? "\nKeep holding to see steadying and fatigue at work." :
				        steadies          ? "\nKeep holding to see the aim steady." :
				                            "\nKeep holding to see fatigue set in.";
			}
			help += "\nRight Click to zoom in";
			ShowTooltip(help.c_str());
		}

		// Holding the right button zooms in around the spot that was under the mouse when it went down, and letting go eases back out.
		if (IsItemHovered() && IsMouseClicked(ImGuiMouseButton_Right)) {
			preview.zooming = true;
			preview.zoomFocus = GetMousePos();
		}
		if (preview.zooming && !IsMouseDown(ImGuiMouseButton_Right)) {
			preview.zooming = false;
		}
		preview.shownZoom += ((preview.zooming ? kZoom : 1.0f) - preview.shownZoom) * (1.0f - std::exp(-kZoomRate * dt));

		// What the pane shows, zoomed: a point at p of the unzoomed pane is at focus + (p - focus) * zoom.
		const float  pxPerMeter = side * 0.5f / kView * preview.shownZoom;
		const ImVec2 center{ preview.zoomFocus.x + (paneCenter.x - preview.zoomFocus.x) * preview.shownZoom, preview.zoomFocus.y + (paneCenter.y - preview.zoomFocus.y) * preview.shownZoom };

		// Only the first click sets the aim: the mouse may then move freely while the button stays down.
		if (IsItemActivated()) {
			const ImVec2 mouse = GetMousePos();
			preview.aimX = std::clamp((mouse.x - center.x) / pxPerMeter, -3.0f, 3.0f);
			preview.aimY = std::clamp(kNpcHeight * 0.5f - (mouse.y - center.y) / pxPerMeter, -1.0f, kNpcHeight + 1.0f);
			preview.Reroll();  // A click is a shot.
			preview.cleared = false;
		}
		preview.holding = IsItemActive();

		auto*        drawList = GetWindowDrawList();
		const ImVec2 aim{ center.x + preview.aimX * pxPerMeter, center.y - (preview.aimY - kNpcHeight * 0.5f) * pxPerMeter };

		PushClipRect(drawList, origin, corner, true);
		AddRectFilled(drawList, origin, corner, IM_COL32(18, 20, 22, 255), 0.0f, 0);
		DrawFigure(drawList, humanoid, preview.aimX, preview.aimY, preview.aimX, preview.aimY, aim, pxPerMeter, origin.y, corner.y, true);

		if (attack.Aims()) {
			AddCircle(drawList, aim, SharedDistance() * std::tan(std::min(widestNow, 1.2f)) * pxPerMeter, IM_COL32(255, 255, 255, 150), 64, 1.0f);
		}

		const auto look = LookOf(info);
		const auto screen = [&](const ImVec2& meters) { return ImVec2{ aim.x + meters.x * pxPerMeter, aim.y + meters.y * pxPerMeter }; };

		if (stream) {
			// Newer parts of the trail are drawn over older ones, and every segment has a dark outline below its color,
			// so that where many cross each one can still be told apart. What is older than a few legs has faded away.
			for (std::size_t i = 1; i < preview.trail.size(); ++i) {
				const auto& from = preview.trail[i - 1];
				const auto& to = preview.trail[i];
				if (std::fabs(from.meters.x - to.meters.x) + std::fabs(from.meters.y - to.meters.y) < 1e-4f)
					continue;

				const float age = std::clamp(static_cast<float>(preview.tick - to.tick) / static_cast<float>(trailTicks), 0.0f, 1.0f);
				const float fade = preview.holding ? 1.0f - age : 1.0f;
				AddLine(drawList, screen(from.meters), screen(to.meters), IM_COL32(0, 0, 0, static_cast<int>(235 * fade)), 4.0f);
				AddLine(drawList, screen(from.meters), screen(to.meters), Hue(0.55f + 0.45f * age, static_cast<int>(255 * fade)), 2.0f);
			}
			if (!preview.trail.empty()) {
				const ImVec2 head = screen(preview.trail.back().meters);
				AddCircleFilled(drawList, head, 4.0f, IM_COL32(255, 255, 255, 255), 12);
				AddCircle(drawList, head, 4.0f, IM_COL32(0, 0, 0, 255), 12, 1.5f);
			}
		} else if (info.concentration) {
			ImVec2 from = aim;
			for (const auto& hit : hits) {
				const ImVec2 to = screen(hit);
				AddLine(drawList, from, to, IM_COL32(0, 0, 0, 230), 4.0f);
				from = to;
			}
			from = aim;
			for (int i = 0; i < total; ++i) {
				const ImVec2 to = screen(hits[i]);
				AddLine(drawList, from, to, Hue(0.55f + 0.45f * static_cast<float>(i) / static_cast<float>(std::max(1, total - 1))), 2.0f);
				from = to;
			}
			AddCircleFilled(drawList, from, 4.0f, IM_COL32(255, 255, 255, 255), 12);
			AddCircle(drawList, from, 4.0f, IM_COL32(0, 0, 0, 255), 12, 1.5f);
		} else {
			const float scale = total > 60 ? 0.8f : 1.0f;
			for (std::size_t n = 0; n < hits.size(); ++n) {
				// The shots of a flow fade as newer ones follow.
				const float fade = n < weights.size() ? weights[n] : 1.0f;
				DrawShot(drawList, screen(hits[n]), look, scale, fade);
			}
		}

		// A glow on the edge toward the shots that landed out of view: the pane is divided into directions around its middle, and in each the glow is
		// stronger the more shots went that way.
		{
			constexpr int              buckets = 72;
			std::array<float, buckets> outside{};
			int                        outOfViewNow = 0;
			std::array<float, buckets> hueSum{};  // For a stream, where along the trail the shots were: the glow takes the color of the trail there.
			for (std::size_t n = 0; n < hits.size(); ++n) {
				const ImVec2 at = screen(hits[n]);
				if (at.x >= origin.x && at.x <= corner.x && at.y >= origin.y && at.y <= corner.y)
					continue;

				++outOfViewNow;
				const float angle = std::atan2(at.y - paneCenter.y, at.x - paneCenter.x);
				const int   bucket = static_cast<int>(std::fmod(angle / (2.0f * std::numbers::pi_v<float>)+1.0f, 1.0f) * buckets);
				// A shot of the trail counts as much as it still shows, like its line does, so the glow fades away with the trail.
				const float weight = n < weights.size() ? weights[n] : 1.0f;
				// The trail goes from blue (new) on to violet (old), the same way as its line is drawn.
				const float age = n < ages.size() ? ages[n] : static_cast<float>(n) / static_cast<float>(std::max<std::size_t>(1, hits.size() - 1));
				outside[std::clamp(bucket, 0, buckets - 1)] += weight;
				hueSum[std::clamp(bucket, 0, buckets - 1)] += weight * (0.55f + 0.45f * age);
			}

			preview.outOfView = outOfViewNow;

			// The circles are centered outside the pane, half their radius, so that only about a fifth of each shows.
			const float reach = std::max(18.0f, side * 0.085f);
			const float offset = reach * 0.5f;
			for (int i = 0; i < buckets; ++i) {
				if (outside[i] < 0.02f)
					continue;

				const float angle = (static_cast<float>(i) + 0.5f) / buckets * 2.0f * std::numbers::pi_v<float>;
				const float c = std::cos(angle);
				const float s = std::sin(angle);
				const float t = (side * 0.5f) / std::max(std::fabs(c), std::fabs(s));
				// Where the direction meets the pane's edge, moved straight out of that edge (not along the direction, which at a slant would move it
				// little in that edge's terms).
				const bool   horizontal = std::fabs(c) >= std::fabs(s);
				const ImVec2 edge{ paneCenter.x + c * t + (horizontal ? (c > 0.0f ? offset : -offset) : 0.0f),
					               paneCenter.y + s * t + (horizontal ? 0.0f : (s > 0.0f ? offset : -offset)) };

				// The glow has a floor of brightness, but it goes out with the last fading shot instead of staying at the floor.
				const float strength = 1.0f - std::exp(-outside[i] / 5.0f);
				const float alpha = std::min(0.9f, (0.12f + 0.78f * strength) * std::min(1.0f, outside[i] * 4.0f));

				// Nested circles, each faint, add up to a glow that fades with the distance from the edge.
				// Only what is bigger than the offset reaches into the pane, so the layers run from there to the full radius: the brightest is at the edge.
				constexpr int layers = 8;
				const int     layerAlpha = std::max(1, static_cast<int>(255.0f * alpha * 0.2f));
				for (int layer = layers; layer >= 1; --layer) {
					const float radius = offset + (reach - offset) * static_cast<float>(layer) / layers;
					AddCircleFilled(drawList, edge, radius, info.concentration ? Hue(hueSum[i] / outside[i], layerAlpha) : GlowOf(look, layerAlpha), 24);
				}
			}
		}

		DrawAimMark(drawList, aim);
		PopClipRect(drawList);
		AddRect(drawList, origin, corner, IM_COL32(255, 255, 255, 40), 0.0f, 0, 1.0f);

		// ---- Instructions ----

		TextDisabled(info.concentration ? "The thin circle is the widest the beam can wander right now." : "The thin circle is the widest a shot can land right now.");
		TextDisabled(info.concentration ? "A glow on the edge means the beam wandered out of view." : "A glow on the edge means shots landed out of view.");
	}
}
