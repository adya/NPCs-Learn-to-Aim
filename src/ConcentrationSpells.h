#pragma once

namespace NLA::ConcentrationSpells
{
	void Install();

	void Reset();

	/// Internals of the free-aiming beam, open only so that the menu preview can replay the same wander.
	namespace Details
	{
		/// How the free-aiming beam wanders: it rests at a point, glides to the next one, rests there, and so on. The offsets are angles in radians.
		struct Wander
		{
			RE::NiPoint2 start{ 0.0f, 0.0f };
			RE::NiPoint2 finish{ 0.0f, 0.0f };
			RE::NiPoint2 current{ 0.0f, 0.0f };

			std::uint32_t elapsedTicks = pauseTicks;
			bool          paused = true;

#ifndef NDEBUG
			/// Set by `Advance` when it picked a new target point, so the log doesn't repeat every tick.
			bool retargeted = false;
#endif

			/// Duration in which animation should glide from start to finish.
			constexpr static std::uint32_t travelTicks = 45;

			/// Time that beam would stay at the same point before starting to glide to the next one.
			constexpr static std::uint32_t pauseTicks = 20;

			/// Moves the beam one tick. `pickNext` is given the point the beam is at and returns the next one to wander to.
			/// Returns false, leaving the beam where it is, if there is no spread (`maxSpread` is not above 0) to wander within.
			template <class PickNext>
			bool Advance(float maxSpread, PickNext&& pickNext) {
#ifndef NDEBUG
				retargeted = false;
#endif

				if (maxSpread <= 0.0f) {
					return false;
				}

				if (!paused && elapsedTicks >= travelTicks) {
					paused = true;
					elapsedTicks = 0;
				} else if (paused && elapsedTicks >= pauseTicks) {
					start = finish;

					finish = pickNext(start);

					paused = false;
					elapsedTicks = 0;
#ifndef NDEBUG
					retargeted = true;
#endif
				}

				++elapsedTicks;

				if (paused) {
					current = finish;
				} else {
					float t = static_cast<float>(elapsedTicks) / travelTicks;
					t = t * t * (3.0f - 2.0f * t);
					current.x = start.x + (finish.x - start.x) * t;
					current.y = start.y + (finish.y - start.y) * t;
				}

				return true;
			}
		};

		/// The next point of the wander when the beam may move at most `maxStep` (radians) from `from` towards the `candidate` it picked.
		RE::NiPoint2 BoundedStep(const RE::NiPoint2& from, const RE::NiPoint2& candidate, float maxStep);
	}
}
