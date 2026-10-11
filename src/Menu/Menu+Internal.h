#pragma once

#include "Options.h"
#include <functional>

namespace NLA::Menu
{
	using Config = Options::Config;
	using HeadshotSettings = Options::NPCConfig::HeadshotSettings;

	// ---- Pages ----

	struct PageContext
	{
		Config&       cfg;
		const Config& defaults;
		bool          isNPC;
	};

	using PageBody = void (*)(const PageContext&);

	/// With `masterSwitch` the page starts with the actor's Enable checkbox; otherwise it shows a notice while that is off.
	void RenderActorPage(bool isNPC, bool masterSwitch, PageBody body);

	template <bool IsNPC, PageBody Body, bool MasterSwitch = false>
	void ActorPage() {
		RenderActorPage(IsNPC, MasterSwitch, Body);
	}

	void RenderGeneralPage(const PageContext& ctx);
	void RenderBowsPage(const PageContext& ctx);
	void RenderCrossbowsPage(const PageContext& ctx);
	void RenderSpellsPage(const PageContext& ctx);
	void RenderConcentrationSpellsPage(const PageContext& ctx);
	void RenderStavesPage(const PageContext& ctx);
	void RenderConcentrationStavesPage(const PageContext& ctx);
	void RenderScrollsPage(const PageContext& ctx);
	void RenderActionsPage();

	// ---- Widgets ----

	class DisabledIf
	{
	public:
		explicit DisabledIf(bool disabled);
		~DisabledIf();
		DisabledIf(const DisabledIf&) = delete;
		DisabledIf& operator=(const DisabledIf&) = delete;

	private:
		bool disabled;
	};

	void ShowTooltip(const char* text);

	/// Begins a 3-column table with a fixed-width label column, optionally followed by the Accuracy/Precision header.
	/// Call EndTable() if it returns true.
	bool BeginPageTable(const char* id, bool hasAccuracy, bool withHeader);

	/// Begins a table with the (empty)/Accuracy/Precision header, renders `rows` and ends it.
	void RenderTable(const char* id, bool hasAccuracy, const std::function<void()>& rows);

	/// Lays a category out as consecutive tables, each titled by a separator. The label column has a fixed width,
	/// so sliders line up across tables, and only the first table has the Accuracy/Precision header.
	class TableSections
	{
	public:
		explicit TableSections(bool hasAccuracy);
		~TableSections();
		TableSections(const TableSections&) = delete;
		TableSections& operator=(const TableSections&) = delete;

		/// Starts a new table below `separator`, or continues the current one when it is null (except for the first call).
		/// Returns false if the table isn't visible, in which case the rows must be skipped.
		bool Section(const char* separator);

	private:
		bool hasAccuracy;
		bool open = false;
		int  count = 0;
	};

	void HeaderRow(const char* first, const char* second);
	void EmptyRow();

	/// Starts a new row with the button in the first column.
	bool ResetButton(const char* label);

	void ToggleColumn(const char* label, bool& value, const char* tooltip);
	/// With `spanRow` the label may run into the following columns, which must stay empty.
	void ExclusiveToggleColumn(const char* label, bool& value, bool& other, const char* tooltip, bool spanRow = false);
	void SliderColumn(const char* id, float& value, const char* tooltip, float min = 0.01f, float max = 10.0f, const char* format = "x%.2f");
	void SecondsSliderColumn(const char* id, float& value, const char* tooltip, float min, float max);
	void PercentSliderColumn(const char* id, float& value, const char* tooltip);
	void NotApplicableColumn();

	// ---- Category parts ----
	// Each part renders and resets its own rows. `aiming` is the state of the category's "Learn X" toggle.

	struct HeadshotRows
	{
		bool HeadshotSettings::*  enabled;
		float HeadshotSettings::* minChance;
		float HeadshotSettings::* maxChance;
		const char*               label;
		const char*               tooltip;
		const char*               shooter;

		void Render(const PageContext& ctx, bool aiming, TableSections& sections) const;
		void Reset(const PageContext& ctx) const;
	};

	struct RangeRows
	{
		bool Config::*  enabled;
		float Config::* min;
		float Config::* max;
		const char*     tooltip;
		const char*     minTooltip;
		const char*     maxTooltip;

		/// Separator title for the rows; null keeps them in the current group.
		const char* group = nullptr;

		void Render(const PageContext& ctx, bool aiming, TableSections& sections) const;
		void Reset(const PageContext& ctx) const;
	};

	struct UseHighestMagicSkillRow
	{
		bool Config::* enabled;
		const char*    tooltip;

		void Render(const PageContext& ctx, bool aiming, TableSections& sections) const;
		void Reset(const PageContext& ctx) const;
	};

	/// Mutually exclusive Use Enchanting Skill / Use Highest Magic Skill toggles.
	struct SkillSourceRows
	{
		bool Config::* useEnchantingSkill;
		const char*    useEnchantingSkillTooltip;
		bool Config::* useHighestMagicSkill;
		const char*    useHighestMagicSkillTooltip;

		void Render(const PageContext& ctx, bool aiming, TableSections& sections) const;
		void Reset(const PageContext& ctx) const;
	};

	struct SpeedRows
	{
		Config::SpeedOptions Config::* options;
		const char*                    label;
		const char*                    tooltip;

		/// Separator title for the rows; null keeps them in the current group.
		const char* group = nullptr;

		void Render(const PageContext& ctx, bool aiming, TableSections& sections) const;
		void Reset(const PageContext& ctx) const;
	};

	struct SpreadRows
	{
		Config::SpreadOptions Config::* options;
		const char*                     label;
		const char*                     tooltip;
		const char*                     maxReductionTooltip;
		const char*                     halfDurationTooltip;

		void Render(const PageContext& ctx, bool aiming, TableSections& sections) const;
		void Reset(const PageContext& ctx) const;
	};

	struct FatigueRows
	{
		Config::FatigueOptions Config::* options;
		const char*                      label;
		const char*                      tooltip;
		const char*                      minThresholdTooltip;
		const char*                      maxThresholdTooltip;
		const char*                      maxPenaltyTooltip;

		void Render(const PageContext& ctx, bool aiming, TableSections& sections) const;
		void Reset(const PageContext& ctx) const;
	};

	// ---- Categories ----

	/// The "Learn X" toggle with its skill multipliers, shared by every category.
	struct CategoryBase
	{
		const char*                      id;
		const char*                      label;
		const char*                      tooltip;
		bool Config::*                   aiming;
		float Config::SkillMultiplier::* multiplier;
		const char*                      accuracyTooltip;
		const char*                      precisionTooltip;
		const char*                      resetLabel;
	};

	/// Parts are rendered below the "Learn X" row in the order they are listed.
	template <class... Parts>
	struct Category : CategoryBase, Parts...
	{};

	void RenderCategoryTable(const CategoryBase& category, const PageContext& ctx, const char* separator,
		const std::function<void(bool aiming, TableSections& sections)>& renderParts, const std::function<void()>& resetParts);

	/// Renders the category below `separator`. Parts that have a group of their own (skill source, range, speed, steadying, fatigue) start
	/// a new section with their title; the rest stay in the section of the "Learn X" row.
	template <class... Parts>
	void RenderCategory(const Category<Parts...>& category, const PageContext& ctx, const char* separator = "Aiming") {
		RenderCategoryTable(
			category, ctx, separator,
			[&](bool aiming, TableSections& sections) { (static_cast<const Parts&>(category).Render(ctx, aiming, sections), ...); },
			[&] { (static_cast<const Parts&>(category).Reset(ctx), ...); });
	}

	inline constexpr const char* kScaleAimLabel = "Scale Aim With Skill";
	inline constexpr const char* kScaleDrawSpeedLabel = "Scale Draw Speed With Skill";
	inline constexpr const char* kScaleChargeSpeedLabel = "Scale Charge Speed With Skill";
	inline constexpr const char* kOverchargeLabel = "Overcharge Steadies Aim";
	inline constexpr const char* kDrawSpeedGroup = "Draw Speed";
	inline constexpr const char* kChargeSpeedGroup = "Charge Speed";
	inline constexpr const char* kRangeGroup = "Range";
	inline constexpr const char* kSkillSourceGroup = "Skill Source";

	/// Shared by spells, staves and scrolls; only the toggle's tooltip differs.
	constexpr FatigueRows CastFatigue(Config::FatigueOptions Config::* options, const char* tooltip) {
		return {
			options,
			"Overcharge Fatigues Aim",
			tooltip,
			"Seconds a spell/staff/scroll can be held overcharged at 0 skill before fatigue sets in.",
			"Seconds a spell/staff/scroll can be held overcharged at 100 skill before fatigue sets in.",
			"The most spread that prolonged fatigue can add. Note, that in fatigued state, the bonus from overcharging is negated before adding extra spread."
		};
	}
}
