#include "Menu+Internal.h"
#include <SKSEMCP/SKSEMenuFramework.hpp>

using namespace ImGuiMCP;

namespace NLA::Menu
{
	namespace
	{
		/// The least width left to the preview next to the settings.
		constexpr float kMinPreviewWidth = 300.0f;

		constexpr const char* kAtZeroSkillHeader = "At 0 Skill";
		constexpr const char* kAtMaxSkillHeader = "At 100 Skill";
		constexpr const char* kBlindMultHeader = "Blind Mult";

		/// What the skill picks for `part`: null for a weapon, whose skill is fixed (Archery). Only the magic kinds choose.
		Options::SkillSource* SourceFor(const KindContext& kind, Options::SkillSource Config::SkillSources::*field) {
			return kind.info.magic ? &(kind.cfg.skills.*field) : nullptr;
		}

		/// `what` is the thing the skill drives, e.g. "draw speed".
		std::string SkillMultiplierTooltip(std::string_view what) {
			return fmt::format("How much skill matters for {}. x1.00 is normal, higher makes skill count for more.", what);
		}
	}

	void SkillShotRows::Render(const KindContext& kind, Sections& sections) const {
		if (!kind.page.isNPC || !sections.Section("Skill Shots")) {
			return;
		}
		auto& skillShot = kind.cfg.skillShot;

		ScaleRows("SkillShot", &skillShot.enabled, tooltip, SourceFor(kind, &Config::SkillSources::skillShot), "The skill shot chance", skillShot.skillMultiplier, SkillMultiplierTooltip("the skill shot chance").c_str());

		DisabledIf off{ !skillShot.enabled };
		HeaderRow(kAtZeroSkillHeader, kAtMaxSkillHeader);
		const auto minTooltip = fmt::format("Chance that {} by a {} with 0 {} is a skill shot.", kind.info.concentration ? "a beam cast" : "a shot", shooter, skill);
		const auto maxTooltip = fmt::format("Chance that {} by a {} with 100 {} is a skill shot.", kind.info.concentration ? "a beam cast" : "a shot", shooter, skill);
		ValueRow(nullptr, nullptr,
		         { "##MinSkillShotChance", &skillShot.bounds.from, Options::Limits::kSkillShotChance, ValueFormat::kPercent, minTooltip.c_str() },
		         { "##MaxSkillShotChance", &skillShot.bounds.to, Options::Limits::kSkillShotChance, ValueFormat::kPercent, maxTooltip.c_str() });
	}

	void RangeRows::Render(const KindContext& kind, Sections& sections) const {
		if (!sections.Section(group)) {
			return;
		}
		auto& range = kind.cfg.range;

		ScaleRows("Range", &range.enabled, nullptr, SourceFor(kind, &Config::SkillSources::range), group ? group : "Range", range.skillMultiplier, SkillMultiplierTooltip("how far the spell reaches").c_str());

		DisabledIf off{ !range.enabled };
		HeaderRow(kAtZeroSkillHeader, kAtMaxSkillHeader);
		ValueRow(nullptr, nullptr,
		         { "##MinRange", &range.bounds.from, Options::Limits::kRangeMultiplier, ValueFormat::kMultiplier, minTooltip },
		         { "##MaxRange", &range.bounds.to, Options::Limits::kRangeMultiplier, ValueFormat::kMultiplier, maxTooltip });
	}

	void SpeedRows::Render(const KindContext& kind, Sections& sections) const {
		if (!sections.Section(group)) {
			return;
		}
		auto& speed = kind.cfg.speed;

		ScaleRows("Speed", &speed.enabled, nullptr, SourceFor(kind, &Config::SkillSources::speed), group ? group : "Speed", speed.skillMultiplier, SkillMultiplierTooltip("the speed").c_str());

		DisabledIf off{ !speed.enabled };
		HeaderRow(kAtZeroSkillHeader, kAtMaxSkillHeader);
		ValueRow(nullptr, nullptr,
		         { "##MinSpeed", &speed.bounds.from, Options::Limits::kSpeedMultiplier, ValueFormat::kMultiplier, "Speed at 0 skill. x1.00 is normal, higher is faster." },
		         { "##MaxSpeed", &speed.bounds.to, Options::Limits::kSpeedMultiplier, ValueFormat::kMultiplier, "Speed at 100 skill. x1.00 is normal, higher is faster." });
	}

	void SpreadRows::Render(const KindContext& kind, Sections& sections) const {
		if (!sections.Section("Steadying")) {
			return;
		}
		auto& spread = kind.cfg.steadying;

		// Steadying scales the spread, so it has nothing to act on without Accuracy.
		DisabledIf noAccuracy{ !kind.cfg.aiming };
		HeaderRow("Spread", "Half Time");
		ToggleRow("Steady aim", spread.enabled, tooltip,
		          { "##MaxReduction", &spread.maxSpreadMultiplier, Options::Limits::kSteadyingReduction, ValueFormat::kPercent, maxReductionTooltip },
		          { "##SpreadHalfDuration", &spread.halfDuration, Options::Limits::kHalfDuration, ValueFormat::kSeconds, halfDurationTooltip });
	}

	void FatigueRows::Render(const KindContext& kind, Sections& sections) const {
		if (!sections.Section("Fatigue")) {
			return;
		}
		auto& fatigue = kind.cfg.fatigue;

		// Bows and crossbows tire the archer's Stamina; every magic kind depends on Magicka.
		const char* resource = kind.info.magic ? "Magicka" : "Stamina";
		const auto  minTip = fmt::format("Threshold when max {} is 0.", resource);
		const auto  maxTip = fmt::format("Threshold when max {} is 100.", resource);
		const auto  thresholdTip = fmt::format("How long the hold lasts before tiring sets in. It grows evenly with max {0}, and keeps growing past the Max value when there is more than 100 {0}.", resource);

		// Fatigue scales the spread, so it has nothing to act on without Accuracy.
		DisabledIf noAccuracy{ !kind.cfg.aiming };
		HeaderRow("Spread", "Half Time");
		ToggleRow("Tire aim", fatigue.enabled, tooltip,
		          { "##MaxPenalty", &fatigue.maxSpreadMultiplier, Options::Limits::kFatiguePenalty, ValueFormat::kPercent, maxPenaltyTooltip },
		          { "##FatigueHalfDuration", &fatigue.halfDuration, Options::Limits::kHalfDuration, ValueFormat::kSeconds, "Time after tiring sets in to reach half of the max fatigue. Shorter means the aim falls apart faster." });
		RowGap();

		DisabledIf off{ !fatigue.enabled };
		HeaderRow(HeaderLabel{ "Min", minTip.c_str() }, HeaderLabel{ "Max", maxTip.c_str() });
		ValueRow("Threshold", thresholdTip.c_str(),
		         { "##MinThreshold", &fatigue.threshold.from, Options::Limits::kFatigueThreshold, ValueFormat::kSeconds, minThresholdTooltip },
		         { "##MaxThreshold", &fatigue.threshold.to, Options::Limits::kFatigueThreshold, ValueFormat::kSeconds, maxThresholdTooltip });
	}

	void RenderAttackKindTable(const AttackKindBase& kind, const PageContext& ctx, const std::function<void(const KindContext&, Sections&)>& renderParts) {
		const auto& info = kind.kind;
		auto&       attack = ctx.cfg.*info.config;
		KindContext context{ ctx, attack, info };

		PushID(kind.id);

		// The preview keeps its size however wide the window is, and the settings take the rest, so that the effect of a change is visible without leaving the page.
		// When the window is narrow the preview gives way first, down to a minimum, so that the settings keep their table.
		const float spacing = GetStyle()->ItemSpacing.x;
		const float tableWidth = PageTableWidth() + GetStyle()->ScrollbarSize + GetStyle()->WindowPadding.x * 2;
		const float available = GetContentRegionAvail().x;
		const float previewWidth = std::clamp(available - tableWidth - spacing, kMinPreviewWidth, PreviewPaneWidth());
		const float settingsWidth = std::max(200.0f, available - previewWidth - spacing);
		const bool  settingsVisible = BeginChild("##Settings", ImVec2(settingsWidth, 0), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar);
		if (settingsVisible) {
			// A little air above the first row, less than there is between the rows of a section.
			Dummy(ImVec2(0.0f, GetFrameHeight() * 0.25f));

			Sections sections;

			// The one switch for the whole kind, with the button that resets the page beside it. Off, the mod leaves the kind alone and none of the settings
			// below apply.
			bool reset = false;
			if (sections.Section(nullptr)) {
				reset = KindRow(kind.toggleLabel, attack.enabled, "Turns everything below on or off for these. When off, the mod doesn't touch them and the settings below are ignored.");
			}
			// Everything below the first row is grey while the kind is off.
			{
				DisabledIf kindOff{ !attack.enabled };

				// A beam doesn't miss or fire shots: it wanders around the aim.
				const bool beam = info.concentration;

				if (sections.Section("Accuracy")) {
					// Its own switch is the aim itself: without Accuracy aim is perfect, and Precision, Retargeting, Steadying and Fatigue have nothing to act on.
					ScaleRows("Accuracy", &attack.aiming, "Accuracy is how far a shot can miss the aim. Off, aim is perfect, and Precision, Retargeting, Steadying and Fatigue have nothing left to change.",
					          SourceFor(context, &Config::SkillSources::accuracy), "Accuracy", attack.accuracy.skillMultiplier, kind.accuracyTooltip);

					DisabledIf off{ !attack.aiming };
					HeaderRow(kAtZeroSkillHeader, kAtMaxSkillHeader, kBlindMultHeader);
					ValueRow(nullptr, nullptr,
					         { "##AccuracyMin", &attack.accuracy.bounds.from, Options::Limits::kAccuracyFactor, ValueFormat::kMultiplier, "Accuracy multiplier at 0 skill. Lower is more accurate." },
					         { "##AccuracyMax", &attack.accuracy.bounds.to, Options::Limits::ToLimit(Options::Limits::kAccuracyFactor, attack.accuracy.bounds.from), ValueFormat::kMultiplier, "Accuracy multiplier at 100 skill. Lower is more accurate." },
					         { "##AccuracyBlindness", &attack.accuracyBlindness, Options::Limits::kAccuracyBlindness, ValueFormat::kMultiplier, "Accuracy multiplier at full blindness. Higher is less accurate." });
				}

				if (sections.Section("Precision")) {
					DisabledIf noAccuracy{ !attack.aiming };
					ScaleRows("Precision", &attack.precision.enabled, beam ? "Precision shapes how the beam's wander is spread within the wander area. Off, it spreads evenly." : "Precision shapes how shots are spread within the miss area. Off, they spread evenly.",
					          SourceFor(context, &Config::SkillSources::precision), "Precision", attack.precision.skillMultiplier, kind.precisionTooltip);

					DisabledIf        off{ !attack.precision.enabled };
					const char* const zero = beam ? "Precision at 0 skill. 0.50 spreads the wander evenly, higher keeps the beam near the aim, lower pushes it to the edge." : "Precision at 0 skill. 0.50 spreads shots evenly, higher bunches them near the aim, lower pushes them to the edge.";
					const char* const hundred = beam ? "Precision at 100 skill. 0.50 spreads the wander evenly, higher keeps the beam near the aim, lower pushes it to the edge." : "Precision at 100 skill. 0.50 spreads shots evenly, higher bunches them near the aim, lower pushes them to the edge.";
					HeaderRow(kAtZeroSkillHeader, kAtMaxSkillHeader, kBlindMultHeader);
					ValueRow(nullptr, nullptr,
					         { "##PrecisionMin", &attack.precision.bounds.from, Options::Limits::kFocus, ValueFormat::kPlain, zero },
					         { "##PrecisionMax", &attack.precision.bounds.to, Options::Limits::ToLimit(Options::Limits::kFocus, attack.precision.bounds.from), ValueFormat::kPlain, hundred },
					         { "##PrecisionBlindness", &attack.precisionBlindness, Options::Limits::kPrecisionBlindness, ValueFormat::kMultiplier, "Precision loss at full blindness. x1.50 is 50% less precise, x0.80 is 20% more." });
				}

				if (beam && sections.Section("Retargeting")) {
					DisabledIf noAccuracy{ !attack.aiming };
					ScaleRows("Retarget", &attack.retarget.enabled, "Retargeting limits how far the beam can move in one step. Off, it can jump anywhere in the wander area.",
					          SourceFor(context, &Config::SkillSources::retarget), "Retargeting", attack.retarget.skillMultiplier, "How strongly skill limits the beam's steps. Higher means smaller steps.");

					DisabledIf off{ !attack.retarget.enabled };
					HeaderRow(kAtZeroSkillHeader, kAtMaxSkillHeader, kBlindMultHeader);
					ValueRow(nullptr, nullptr,
					         { "##RetargetMin", &attack.retarget.bounds.from, Options::Limits::kRetarget, ValueFormat::kPlain, "Step size at 0 skill, as a share of the wander area. 2.00 or more means no limit. Lower is steadier." },
					         { "##RetargetMax", &attack.retarget.bounds.to, Options::Limits::ToLimit(Options::Limits::kRetarget, attack.retarget.bounds.from), ValueFormat::kPlain, "Step size at 100 skill, as a share of the wander area. Lower is steadier." },
					         { "##RetargetBlindness", &attack.retargetBlindness, Options::Limits::kRetargetBlindness, ValueFormat::kMultiplier, "Step size multiplier at full blindness. Higher is less steady." });
				}

				renderParts(context, sections);
			}

			if (reset) {
				attack = ctx.defaults.*info.config;
				Options::Save();
			}
		}
		EndChild();

		SameLine();
		if (BeginChild("##Preview", ImVec2(previewWidth, 0), ImGuiChildFlags_Border, 0)) {
			RenderSpreadPreview(context);
		}
		EndChild();
		PopID();
	}
}
