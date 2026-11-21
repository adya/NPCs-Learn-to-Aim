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

		constexpr float kMinDistance = 5.0f;
		constexpr float kMaxDistance = 100.0f;
		constexpr int   kMaxShots = 200;

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

		/// A trail is kept for this many wander legs (a glide to the next point and the rest there), then it has faded away.
		constexpr int kTrailLegs = 5;
		constexpr int kTrailTicks = kTrailLegs * static_cast<int>(ConcentrationSpells::Details::Wander::travelTicks + ConcentrationSpells::Details::Wander::pauseTicks);

		// ---- What the target is made of ----

		/// The standard archery target: ten rings in five colors of two rings each, from the middle out: gold, red, blue, black and white.
		/// The target is as tall as an NPC and its gold circle (the two inner rings, of equal width) is as big as an average head; the eight rings outside
		/// the gold are of equal width.
		constexpr int   kRings = 10;
		constexpr float kTargetRadius = kNpcHeight * 0.5f;
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
			/// Current shows the attacker as they are, Simulated uses the values below, which start from the attacker's.
			bool  simulated = false;
			bool  seeded = false;
			float skill = 25.0f;
			float maxAttribute = 100.0f;

			int   shots = 40;
			int   shape = kHumanoid;
			bool  interactive = true;

			/// Where the shooter aims: the first click on the target sets it, in meters (x to the right, y up from the feet).
			float aimX = 0.0f;
			float aimY = kHeadCenterY;

			/// Whether the target was held down: the shot is being held.
			bool  holding = false;
			bool  wasHolding = false;
			float holdSeconds = 0.0f;

			/// What is on screen. It eases towards the value the settings call for, so a change glides instead of jumping.
			float shownScale = 0.0f;

			/// The zoom of the pane: held with the right button around `zoomFocus`, and eased like the scale.
			bool   zooming = false;
			float  shownZoom = 1.0f;
			ImVec2 zoomFocus{ 0.0f, 0.0f };

			std::vector<Roll> rolls;

			/// The beam of a concentration spell, wandering in real time.
			ConcentrationSpells::Details::Wander wander;
			float                       tickRemainder = 0.0f;
			int                         tick = 0;
			std::deque<TrailPoint>      trail;

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

			/// The aim goes to the middle of the head, or to the middle of the dummy.
			void ResetAim() {
				aimX = 0.0f;
				aimY = Humanoid() ? kHeadCenterY : kDummyCenterY;
			}
		};

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
			case 0: r = 1; g = x; break;
			case 1: r = x; g = 1; break;
			case 2: g = 1; b = x; break;
			case 3: g = x; b = 1; break;
			case 4: r = x; b = 1; break;
			default: r = 1; b = x; break;
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

						const auto half = [&](float offset) { return std::fabs(offset) < r ? std::sqrt(r * r - offset * offset) : 0.0f; };
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
		void DrawFigure(ImDrawList* drawList, bool humanoid, float ringX, float ringY, float anchorX, float anchorY, const ImVec2& anchorScreen, float pxPerMeter, float top, float bottom) {
			const auto toScreen = [&](float x, float y) { return ImVec2{ anchorScreen.x + (x - anchorX) * pxPerMeter, anchorScreen.y - (y - anchorY) * pxPerMeter }; };
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
		}

		/// The aim point stands out from the shots: a bright dot with a dark ring.
		void DrawAimMark(ImDrawList* drawList, const ImVec2& aim) {
			AddCircleFilled(drawList, aim, 6.5f, IM_COL32(0, 0, 0, 255), 20);
			AddCircleFilled(drawList, aim, 4.5f, IM_COL32(0, 229, 255, 255), 20);
			AddCircle(drawList, aim, 7.5f, IM_COL32(255, 255, 255, 220), 20, 1.2f);
		}

		/// A four-pointed star with a small ring in the middle, like the crosshair of the HUD; `radius` reaches its tips.
		void DrawCrosshair(ImDrawList* drawList, const ImVec2& center, float radius) {
			constexpr float pinch = 0.17f;
			const float     k = radius * pinch;
			const ImVec2    star[] = {
				{ center.x, center.y - radius }, { center.x + k, center.y - k }, { center.x + radius, center.y }, { center.x + k, center.y + k },
				{ center.x, center.y + radius }, { center.x - k, center.y + k }, { center.x - radius, center.y }, { center.x - k, center.y - k },
			};
			AddPolyline(drawList, star, 8, IM_COL32(0, 0, 0, 220), ImDrawFlags_Closed, 3.5f);
			AddPolyline(drawList, star, 8, IM_COL32(214, 214, 204, 255), ImDrawFlags_Closed, 1.6f);
			AddCircle(drawList, center, radius * 0.2f, IM_COL32(0, 0, 0, 220), 16, 3.0f);
			AddCircle(drawList, center, radius * 0.2f, IM_COL32(214, 214, 204, 255), 16, 1.4f);
		}

		const char* ZonesText(bool humanoid) {
			return humanoid ?
			           "An average NPC, 1.8 m tall and 0.6 m wide. The standard target's ten rings are drawn around the aim point, as tall as the NPC, and the gold circle is as big as a head: gold for 10 and 9, red for 8 and 7, blue for 6 and 5, black for 4 and 3, white for 2 and 1. Only what lands on the NPC counts." :
			           "A standard archery target as tall as an NPC with ten rings, its gold circle as big as an average NPC's head: gold for 10 and 9, red for 8 and 7, blue for 6 and 5, black for 4 and 3, white for 2 and 1.";
		}

		void RenderReference(const Preview& preview) {
			const ImVec2 origin = GetCursorScreenPos();
			const ImVec2 corner{ origin.x + kReferenceSide, origin.y + kReferenceSide };
			InvisibleButton("##Reference", ImVec2(kReferenceSide, kReferenceSide));

			const float crosshairs = kCrosshairMatchesHeightAt / SharedDistance();
			const std::string centering = preview.Humanoid() ?
			                                  fmt::format("From afar the crosshair is centered on the whole body; the closer you get the more it moves towards the head, and at about {:.0f} m, where it is as big as the head, it is centered on it.", kHeadMatchesCrosshairAt) :
			                                  "The crosshair is centered on the target.";
			ShowTooltip(fmt::format("The vanilla crosshair next to {} at {:.0f} m. {} {}\n\nAt this distance an NPC is {:.1f} crosshairs tall. The crosshair matches the NPC's head at about {:.0f} m, the torso at 20 m, the full height at 60 m, and is twice as big as the NPC at 120 m.", preview.Humanoid() ? "an average NPC" : "a target as tall as one", SharedDistance(), centering, ZonesText(preview.Humanoid()), crosshairs, kHeadMatchesCrosshairAt).c_str());

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

	void RenderSpreadPreview(const KindContext& kind) {
		const auto& attack = kind.cfg;
		const auto& info = kind.info;
		auto&       preview = PreviewFor(&attack);

		const float dt = std::clamp(GetIO()->DeltaTime, 0.0f, 0.1f);

		SeparatorText("Preview");

		// ---- Attacker ----

		const auto  attacker = CurrentAttacker();
		auto* const actor = attacker.actor;
		if (!actor) {
			TextDisabled("No attacker: the game isn't loaded.");
			return;
		}

		const auto        skillValue = Details::SkillForSource(attack.skills.aiming, nullptr, info.magic, actor);
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
		const float leftWidth = std::max(200.0f, GetContentRegionAvail().x - kReferenceSide - spacing);

		// The skill is named after what it is for this kind (Archery, Destruction, Enchanting...), and tinted like it: red for the warrior's, blue for magic, violet for enchanting.
		const std::string skillFormat = fmt::format("{} %.0f", skillName && *skillName ? skillName : "Skill");
		const std::string attributeFormat = info.magic ? "Max Magicka %.0f" : "Max Stamina %.0f";
		const Tint&       skillTint = skillValue == RE::ActorValue::kArchery ? kRedTint : skillValue == RE::ActorValue::kEnchanting ? kVioletTint : kBlueTint;
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
				ShowTooltip("The skill that the settings turn into aim: the Skill Mult is applied to it.");
			}

			if (attack.fatigue.enabled) {
				SameLine();
				float maxAttribute = editable ? preview.maxAttribute : actorMaxAttribute;
				SetNextItemWidth(sliderWidth);
				if (TintedSlider("##PreviewMaxAttribute", &maxAttribute, 0.0f, 500.0f, attributeFormat.c_str(), attributeTint) && editable) {
					preview.maxAttribute = maxAttribute;
				}
				if (editable) {
					ShowTooltip(info.magic ? "The max Magicka, which sets how long a charge can be held before fatigue sets in." : "The max Stamina, which sets how long a draw can be held before fatigue sets in.");
				}
			}
		};

		// The first tab is the attacker as they are now, named after them: whoever is selected in the console, or the player. The second one takes any skill and max Stamina or Magicka.
		if (BeginTabBar("##PreviewMode")) {
			const std::string currentLabel = fmt::format("{}###Current", name && *name ? name : "?");
			const bool        currentOpen = BeginTabItem(currentLabel.c_str());
			if (attacker.selected) {
				ShowTooltip("Currently selected in console");
			}
			if (currentOpen) {
				preview.simulated = false;
				renderValues(false);
				EndTabItem();
			}

			const bool simulatedOpen = BeginTabItem("Simulated");
			ShowTooltip("Try any skill and max Stamina or Magicka.");
			if (simulatedOpen) {
				preview.simulated = true;
				renderValues(true);
				EndTabItem();
			}
			EndTabBar();
		}

		const bool streamed = info.concentration && preview.interactive;
		{
			// A stream that is cast live has no fixed number of points.
			DisabledIf off{ streamed };
			SetNextItemWidth(rowWidth);
			SliderInt("##PreviewShots", &preview.shots, 1, kMaxShots, "Shots %d");
		}
		ShowTooltip(info.concentration ? "How many points the stream wanders to when they are all shown at once." : "How many shots are fired by a click.");

		AlignTextToFramePadding();
		TextUnformatted("Target");
		SameLine();
		if (RadioButton("Dummy", &preview.shape, kDummy)) {
			preview.ResetAim();
		}
		ShowTooltip("A standard archery target, as tall as an NPC.");
		SameLine();
		if (RadioButton("Humanoid", &preview.shape, kHumanoid)) {
			preview.ResetAim();
		}
		ShowTooltip("A silhouette of an average NPC. Only shots that land on it count, and its rings are drawn around the aim.");

		if (info.concentration) {
			Checkbox("Interactive", &preview.interactive);
			ShowTooltip("Checked: hold the target to cast, and the stream wanders in real time; what it passed fades away after a few steps. Unchecked: the points of the stream are shown at once, as trails.");
		}

		if (Button("Reset Aim")) {
			preview.ResetAim();
		}
		ShowTooltip(preview.Humanoid() ? "Aims at the middle of the head again." : "Aims at the middle of the target again.");
		EndChild();

		SameLine();
		BeginGroup();
		RenderReference(preview);
		SetNextItemWidth(kReferenceSide);
		SliderFloat("##PreviewDistance", &SharedDistance(), kMinDistance, kMaxDistance, "Distance %.0f m");
		ShowTooltip("How far the target is. The picture above shows how big an NPC looks at this distance next to the crosshair.");
		EndGroup();

		// ---- Model: the very same functions the shots use ----

		preview.holdSeconds = preview.holding ? preview.holdSeconds + dt : 0.0f;

		const float skill = preview.simulated ? preview.skill : actorSkill;
		const float maxAttribute = preview.simulated ? preview.maxAttribute : actorMaxAttribute;
		const float effectiveAccuracy = skill * attack.accuracy.skillMultiplier;
		const float effectivePrecision = skill * attack.precision.skillMultiplier;
		const float effectiveRetarget = skill * attack.retarget.skillMultiplier;

		const float maxSpread = attack.aiming ? Details::MaxSpreadFor(Details::SkillFactorFor(attack.accuracy.bounds, effectiveAccuracy)) : 0.0f;
		const float focus = attack.aiming ? Details::SkillFactorFor(attack.precision.bounds, effectivePrecision) : 0.5f;
		const float maxStep = maxSpread * Details::SkillFactorFor(attack.retarget.bounds, effectiveRetarget);

		const float threshold = attack.fatigue.enabled ? Fatigue::Details::Threshold(attack.fatigue, maxAttribute) : 0.0f;
		const auto  hold = Fatigue::Hold(attack.steadying, attack.fatigue, threshold, preview.holdSeconds);

		if (preview.shownScale <= 0.0f) {
			preview.shownScale = hold.scale;
		}
		preview.shownScale += (hold.scale - preview.shownScale) * (1.0f - std::exp(-kMoveRate * dt));

		const bool interactive = info.concentration && preview.interactive;
		const bool humanoid = preview.Humanoid();

		// Where the shots land, as offsets in meters from the aim. A concentration stream picks each point from the previous one, as the beam does when it wanders.
		std::vector<ImVec2> hits;
		std::vector<float>  weights;  // How much each shot still shows: the trail fades, the rest is fully there.
		if (interactive) {
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
			while (!preview.trail.empty() && preview.trail.front().tick + kTrailTicks < preview.tick) {
				preview.trail.pop_front();
			}

			for (const auto& point : preview.trail) {
				hits.push_back(point.meters);
				weights.push_back(1.0f - std::clamp(static_cast<float>(preview.tick - point.tick) / static_cast<float>(kTrailTicks), 0.0f, 1.0f));
			}
		} else {
			// Out of Interactive, so that it doesn't show where the beam was before.
			preview.trail.clear();
			preview.wander = {};

			const int                 count = std::clamp(preview.shots, 1, kMaxShots);
			std::vector<RE::NiPoint2> offsets(count);
			RE::NiPoint2              previous{ 0.0f, 0.0f };
			for (int i = 0; i < count; ++i) {
				const auto& roll = preview.rolls[i];
				const auto  candidate = attack.aiming ? Details::SpreadOffset(maxSpread, focus, roll.radius, roll.angle) : RE::NiPoint2{ 0.0f, 0.0f };
				offsets[i] = info.concentration ? ConcentrationSpells::Details::BoundedStep(previous, candidate, maxStep) : candidate;
				previous = offsets[i];
			}
			for (const auto& offset : offsets) {
				hits.push_back(OnTarget({ offset.x * preview.shownScale, offset.y * preview.shownScale }, SharedDistance()));
			}
		}

		preview.wasHolding = preview.holding;

		// ---- Readout ----

		const int total = static_cast<int>(hits.size());
		int       onTarget = 0;
		int       gold = 0;
		int       score = 0;
		for (const auto& hit : hits) {
			// Only what lands on the target counts. y is up on the target and down on the screen.
			if (const auto result = Evaluate(humanoid, preview.aimX + hit.x, preview.aimY - hit.y, preview.aimX, preview.aimY); result) {
				++onTarget;
				score += result->score;
				if (result->gold)
					++gold;
			}
		}

		const float widestNow = maxSpread * preview.shownScale;
		if (attack.aiming) {
			int outOfView = 0;
			for (const auto& hit : hits) {
				// The pane's middle is at x = 0 and half the NPC's height; hit.y is down on the screen.
				if (std::fabs(preview.aimX + hit.x) > kView || std::fabs(preview.aimY - kNpcHeight * 0.5f - hit.y) > kView)
					++outOfView;
			}
			std::string text = fmt::format("Widest {:.2f}° ({:.2f} m), focus {:.2f}.", RE::rad_to_deg(widestNow), SharedDistance() * std::tan(std::min(widestNow, 1.2f)), focus);
			if (outOfView > 0) {
				text += fmt::format(" {} out of view.", outOfView);
			}
			TextDisabled("%s", text.c_str());
		} else {
			TextDisabled("Aiming is off: shots go exactly where they are aimed.");
		}
		if (preview.holding) {
			std::string text = fmt::format("Held {:.1f}s: spread x{:.2f}", preview.holdSeconds, hold.scale);
			if (attack.fatigue.enabled) {
				text += fmt::format(" (fatigue sets in after {:.1f}s{})", threshold, hold.fatigueSeconds > 0.0f ? ", fatigued" : "");
			}
			TextDisabled("%s", text.c_str());
		} else {
			TextDisabled("Click the target to shoot at that point, and hold to hold the shot (for a concentration spell, to cast it).");
		}

		// ---- Canvas ----

		const float  side = std::clamp(GetContentRegionAvail().x, 200.0f, 520.0f);
		const ImVec2 origin = GetCursorScreenPos();
		const ImVec2 corner{ origin.x + side, origin.y + side };
		const ImVec2 paneCenter{ origin.x + side * 0.5f, origin.y + side * 0.5f };
		InvisibleButton("##Canvas", ImVec2(side, side));
		ShowTooltip(fmt::format("{}\n\nClick to shoot there: the click sets the aim point, marked by the bright dot, and the shots are simulated around it. Keep holding the button, moving the mouse out of the way if you like, to hold the shot and see steadying and fatigue at work. The pane always shows the NPC at the same size, {:.1f} m across, so a wider spread, or a longer distance, sends shots further out of it. The thin circle is the widest a shot can land right now. A glow on the edge of the pane marks shots that landed out of view, stronger when there are more of them.\n\nHold the right button over the pane to zoom in on the spot under the mouse, to see a head or a limb closer.",ZonesText(humanoid), kView * 2.0f).c_str());

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
		}
		preview.holding = IsItemActive();

		auto*        drawList = GetWindowDrawList();
		const ImVec2 aim{ center.x + preview.aimX * pxPerMeter, center.y - (preview.aimY - kNpcHeight * 0.5f) * pxPerMeter };

		PushClipRect(drawList, origin, corner, true);
		AddRectFilled(drawList, origin, corner, IM_COL32(18, 20, 22, 255), 0.0f, 0);
		DrawFigure(drawList, humanoid, preview.aimX, preview.aimY, preview.aimX, preview.aimY, aim, pxPerMeter, origin.y, corner.y);

		if (attack.aiming) {
			AddCircle(drawList, aim, SharedDistance() * std::tan(std::min(widestNow, 1.2f)) * pxPerMeter, IM_COL32(255, 255, 255, 150), 64, 1.0f);
		}

		const auto screen = [&](const ImVec2& meters) { return ImVec2{ aim.x + meters.x * pxPerMeter, aim.y + meters.y * pxPerMeter }; };

		if (interactive) {
			// Newer parts of the trail are drawn over older ones, and every segment has a dark outline below its color,
			// so that where many cross each one can still be told apart. What is older than a few legs has faded away.
			for (std::size_t i = 1; i < preview.trail.size(); ++i) {
				const auto& from = preview.trail[i - 1];
				const auto& to = preview.trail[i];
				if (std::fabs(from.meters.x - to.meters.x) + std::fabs(from.meters.y - to.meters.y) < 1e-4f)
					continue;

				const float age = std::clamp(static_cast<float>(preview.tick - to.tick) / static_cast<float>(kTrailTicks), 0.0f, 1.0f);
				const float fade = 1.0f - age;
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
			const float dot = total > 150 ? 2.5f : total > 60 ? 3.2f : 4.0f;
			for (const auto& hit : hits) {
				const ImVec2 at = screen(hit);
				AddCircleFilled(drawList, at, dot, IM_COL32(166, 139, 250, 255), 12);
				AddCircle(drawList, at, dot, IM_COL32(10, 8, 20, 230), 12, 1.2f);
			}
		}

		// A glow on the edge toward the shots that landed out of view: the pane is divided into directions around its middle, and in each the glow is
		// stronger the more shots went that way.
		{
			constexpr int              buckets = 72;
			std::array<float, buckets> outside{};
			for (std::size_t n = 0; n < hits.size(); ++n) {
				const ImVec2 at = screen(hits[n]);
				if (at.x >= origin.x && at.x <= corner.x && at.y >= origin.y && at.y <= corner.y)
					continue;

				const float angle = std::atan2(at.y - paneCenter.y, at.x - paneCenter.x);
				const int   bucket = static_cast<int>(std::fmod(angle / (2.0f * std::numbers::pi_v<float>) + 1.0f, 1.0f) * buckets);
				// A shot of the trail counts as much as it still shows, like its line does, so the glow fades away with the trail.
				outside[std::clamp(bucket, 0, buckets - 1)] += n < weights.size() ? weights[n] : 1.0f;
			}

			// The circles are centered outside the pane, half their radius, so that only about a fifth of each shows.
			const float reach = std::max(18.0f, side * 0.085f);
			const float offset = reach * 0.5f;
			for (int i = 0; i < buckets; ++i) {
				if (outside[i] < 0.02f)
					continue;

				const float  angle = (static_cast<float>(i) + 0.5f) / buckets * 2.0f * std::numbers::pi_v<float>;
				const float  c = std::cos(angle);
				const float  s = std::sin(angle);
				const float  t = (side * 0.5f) / std::max(std::fabs(c), std::fabs(s));
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
					AddCircleFilled(drawList, edge, radius, IM_COL32(166, 139, 250, layerAlpha), 24);
				}
			}
		}

		DrawAimMark(drawList, aim);
		PopClipRect(drawList);
		AddRect(drawList, origin, corner, IM_COL32(255, 255, 255, 40), 0.0f, 0, 1.0f);

		// ---- Statistics ----

		if (total > 0) {
			const auto percent = [&](int n) { return 100.0f * static_cast<float>(n) / static_cast<float>(total); };
			const float average = static_cast<float>(score) / static_cast<float>(total);
			if (interactive) {
				Text("%s", fmt::format("On target {:.0f}%   Gold {:.0f}%   Off {:.0f}%   Avg score {:.1f}  (of the last {} steps)", percent(onTarget), percent(gold), percent(total - onTarget), average, kTrailLegs).c_str());
			} else {
				Text("%s", fmt::format("Hits {} ({:.0f}%)   Gold {} ({:.0f}%)   Misses {} ({:.0f}%)   Avg score {:.1f}", onTarget, percent(onTarget), gold, percent(gold), total - onTarget, percent(total - onTarget), average).c_str());
			}
		} else {
			TextDisabled("Nothing to count yet.");
		}
		ShowTooltip(humanoid ?
		                "Only shots that land on the silhouette count as hits. Gold is the center of a head. A shot scores 10 in the center of a head, 8 in the rest of it, 6 within reach of a torso, 4 within reach of the arms and legs, 2 on the rest of the NPC and 0 for a miss; the average is over all shots. In the Interactive display they are the positions of the stream over the last few steps, as shares." :
		                "Any shot that lands on the target counts as a hit. Gold is its two inner rings. A shot scores 10 in the middle ring down to 1 in the outer one, and 0 for a miss; the average is over all shots. In the Interactive display they are the positions of the stream over the last few steps, as shares.");
	}
}
