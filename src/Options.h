#pragma once


namespace NLA
{
	enum SkillUsageContext
	{
		/// The widest angle a projectile may deviate from the aim.
		kAccuracy,
		/// How tightly shots cluster towards the aim, within the accuracy radius.
		kPrecision,
		/// The step length of a concentration spell's wander.
		kRetarget,

		kChargeTime,
		kRange,
		kSkillShot
	};
}

namespace NLA::Options
{
	/// The skill that scales a kind's aiming, charge speed or range.
	enum class SkillSource : std::uint8_t
	{
		/// The skill of the magic school of the spell's costliest effect.
		kSpellSkill,
		/// The highest of the actor's magic skills, regardless of the spell.
		kHighest,
		kEnchanting,
		/// The Archery skill. The default for bows and crossbows, but any kind can use it.
		kArchery,

		kTotal
	};

	/// The allowed range of an option.
	struct Limit
	{
		float min;
		float max;
	};

	namespace Limits
	{
		inline constexpr Limit kSkillMultiplier{ 0.01f, 5.0f };
		/// How a fully blind shooter's Accuracy (max spread) is scaled: x1.00 is no effect, above it the spread widens, below it narrows.
		inline constexpr Limit kAccuracyBlindness{ 0.0f, 5.0f };
		/// How a fully blind shooter's Precision (focus) is scaled: x1.00 is no effect, above it the focus drops, below it grows. At x2.00 it is gone.
		inline constexpr Limit kPrecisionBlindness{ 0.0f, 2.0f };
		/// How a fully blind shooter's retarget step (concentration spells/staves) is scaled: above x1.00 the stream jumps further per step.
		inline constexpr Limit kRetargetBlindness{ 0.0f, 5.0f };
		inline constexpr Limit kSkillShotChance{ 0.0f, 1.0f };
		inline constexpr Limit kRangeMultiplier{ 0.5f, 5.0f };
		inline constexpr Limit kSpeedMultiplier{ 0.1f, 3.0f };
		inline constexpr Limit kAccuracyFactor{ 0.0f, 3.0f };
		/// The exponent of the spread radius distribution: 0.5 spreads shots evenly over the disc, higher pulls them towards the center.
		inline constexpr Limit kFocus{ 0.25f, 3.0f };
		/// The longest step between wander points as a fraction of the spread radius. 2 or more never limits it: the disc is only 2 radii wide.
		inline constexpr Limit kRetarget{ 0.05f, 2.0f };
		inline constexpr Limit kSteadyingReduction{ 0.0f, 1.0f };
		inline constexpr Limit kFatigueThreshold{ 0.0f, 10.0f };
		inline constexpr Limit kFatiguePenalty{ 0.0f, 3.0f };
		inline constexpr Limit kHalfDuration{ 0.1f, 5.0f };

		/// The lowest `to` of a skill-scaled value as a fraction of its `from`. The skill curve overshoots `to` by up to e^-saturation of the range
		/// for skill above 100, so anything lower would make the value negative there. Keep in sync with `SkillProgress`.
		inline constexpr float kMinToFromRatio = 0.061f;

		/// Narrows `limit` so that a `to` read next to the given `from` can't go negative along the skill curve.
		inline constexpr Limit ToLimit(Limit limit, float from) {
			return { std::max(limit.min, from * kMinToFromRatio), limit.max };
		}
	}

	struct Bounds
	{
		float from{ 0.0f };
		float to{ 0.0f };

		constexpr bool operator==(const Bounds&) const = default;
	};

	struct SkillBasedValue
	{
		float skillMultiplier = 1.0f;

		constexpr bool operator==(const SkillBasedValue&) const = default;
	};

	/// A value that can be switched off.
	struct ToggleableValue
	{
		bool enabled = false;

		constexpr bool operator==(const ToggleableValue&) const = default;
	};

	/// An INI key name. Fixed-capacity so that keys can be built at compile time.
	struct Key
	{
		static constexpr std::size_t kCapacity = 64;

		std::array<char, kCapacity> chars{};

		constexpr Key() = default;

		constexpr Key(const char* text) :
			Key({ std::string_view{ text } }) {}

		/// Concatenates the parts. Fails to compile when evaluated at compile time if they don't fit.
		constexpr Key(std::initializer_list<std::string_view> parts) {
			std::size_t size = 0;
			for (const auto part : parts) {
				for (const char c : part) {
					if (size + 1 >= kCapacity) {
						throw "INI key is too long";
					}
					chars[size++] = c;
				}
			}
		}

		constexpr const char* c_str() const { return chars.data(); }
	};

	/// Names of the INI keys that hold an attack kind's options, laid out like `Config::Attack`. Each kind has its own set, e.g. `Keys::kBow`.
	struct Keys
	{
		struct SkillSources
		{
			Key aiming;
			Key speed;
			Key range;
		};

		struct Aim
		{
			Key skillMultiplier;
			Key from;
			Key to;

			constexpr std::array<Key, 3> All() const { return { skillMultiplier, from, to }; }
		};

		struct Scaling
		{
			Key enabled;
			Key from;
			Key to;
			Key skillMultiplier;

			constexpr std::array<Key, 4> All() const { return { enabled, from, to, skillMultiplier }; }
		};

		struct Steadying
		{
			Key enabled;
			Key maxSpreadMultiplier;
			Key halfDuration;
		};

		struct Fatigue
		{
			Key enabled;
			Key from;
			Key to;
			Key maxSpreadMultiplier;
			Key halfDuration;
		};

		Key aiming;

		Aim accuracy;
		Aim precision;

		Key accuracyBlindness;
		Key precisionBlindness;
		Key retargetBlindness;

		Aim retarget;

		SkillSources skills;

		Scaling speed;
		Scaling range;
		Scaling skillShot;

		Steadying steadying;
		Fatigue   fatigue;

		/// `name` is the attack kind's name, e.g. "Bow".
		static constexpr Keys Build(std::string_view name) {
			return {
				.aiming = { "b", name, "Aiming" },
				.accuracy = AimKeys(name, "Accuracy"),
				.precision = AimKeys(name, "Precision"),
				.accuracyBlindness = { "f", name, "AccuracyBlindness" },
				.precisionBlindness = { "f", name, "PrecisionBlindness" },
				.retargetBlindness = { "f", name, "RetargetBlindness" },
				.retarget = AimKeys(name, "Retarget"),
				.skills = { { "s", name, "AimingSkill" }, { "s", name, "SpeedSkill" }, { "s", name, "RangeSkill" } },
				.speed = ScalingKeys(name, "Speed"),
				.range = ScalingKeys(name, "Range"),
				.skillShot = {
					{ "b", name, "SkillShots" },
					{ "f", name, "MinSkillShotChance" },
					{ "f", name, "MaxSkillShotChance" },
					{ "f", name, "SkillShotSkillMultiplier" },
				},
				.steadying = {
					{ "b", name, "Steadying" },
					{ "f", name, "SteadyingMaxSpreadReduction" },
					{ "f", name, "SteadyingHalfDuration" },
				},
				.fatigue = {
					{ "b", name, "Fatigue" },
					{ "f", name, "FatigueMinThreshold" },
					{ "f", name, "FatigueMaxThreshold" },
					{ "f", name, "FatigueMaxPenalty" },
					{ "f", name, "FatigueHalfDuration" },
				},
			};
		}

		static constexpr const Keys& For(const struct AttackKind& kind);

		static const Keys kBow;
		static const Keys kCrossbow;
		static const Keys kSpell;
		static const Keys kConcentrationSpell;
		static const Keys kStaff;
		static const Keys kConcentrationStaff;
		static const Keys kScroll;

	private:
		static constexpr Aim AimKeys(std::string_view name, std::string_view what) {
			return {
				{ "f", name, what, "SkillMultiplier" },
				{ "f", name, what, "MinMultiplier" },
				{ "f", name, what, "MaxMultiplier" },
			};
		}

		static constexpr Scaling ScalingKeys(std::string_view name, std::string_view what) {
			return {
				{ "bScale", name, what, "WithSkill" },
				{ "f", name, what, "MinMultiplier" },
				{ "f", name, what, "MaxMultiplier" },
				{ "f", name, what, "SkillMultiplier" },
			};
		}
	};

	struct Config
	{
		struct SkillSources
		{
			SkillSource aiming = SkillSource::kTotal;
			SkillSource speed = SkillSource::kTotal;
			SkillSource range = SkillSource::kTotal;

			static SkillSources FromIni(CSimpleIniA& ini, const char* section, const Keys::SkillSources& keys, const SkillSources& defaults);
		};

		/// A multiplier that shapes the shot and grows better with skill: the widest spread (Accuracy) or the focus of the spread (Precision).
		struct Aim : SkillBasedValue
		{
			/// The multiplier at 0 skill and at 100 skill.
			Bounds bounds{};

			constexpr Aim() = default;

			constexpr Aim(float skillMultiplier, Bounds bounds) :
				SkillBasedValue{ skillMultiplier }, bounds(bounds) {}

			static Aim FromIni(CSimpleIniA& ini, const char* section, const Keys::Aim& keys, const Aim& defaults, Limit limit);

			constexpr bool operator==(const Aim&) const = default;
		};

		struct Spread : ToggleableValue
		{
			/// Max multiplier for the spread approached as hold time increases.
			float maxSpreadMultiplier = 0.0f;

			/// Time needed to gain half of the max spread multiplier.
			/// Note, the multiplier is not linear, so 2 * halfDuration does not give the full multiplier.
			float halfDuration = 0.0f;

			static Spread FromIni(CSimpleIniA& ini, const char* section, const Keys::Steadying& keys, const Spread& defaults);
		};

		/// Controls how much holding a bow at full draw, a crossbow loaded or a spell/staff/scroll charged reduces spread.
		using Steadying = Spread;

		/// A value that grows with skill: speed, range or the chance of a skill shot.
		struct Scaling : ToggleableValue, SkillBasedValue
		{
			/// The value at 0 skill and at 100 skill.
			Bounds bounds{};

			constexpr Scaling() = default;
			constexpr Scaling(bool enabled, Bounds bounds, float skillMultiplier = 1.0f) :
				ToggleableValue{ enabled }, SkillBasedValue{ skillMultiplier }, bounds(bounds) {}

			/// For kinds that don't support the option.
			static const Scaling kNone;

			constexpr bool operator==(const Scaling&) const = default;

			static Scaling FromIni(CSimpleIniA& ini, const char* section, const Keys::Scaling& keys, const Scaling& defaults, Limit limit);
		};

		/// Controls how fast a spell/staff/scroll charges, a bow is drawn or a crossbow is loaded.
		using Speed = Scaling;

		/// Controls how far concentration spells/staves reach.
		using RangeScaling = Scaling;

		/// Controls how likely a shooter is to take a skill shot, i.e. to aim for a specific body part instead of the chest. NPCs only.
		/// Which parts it may pick is set by `Config::headshots`, `armshots` and `legshots`.
		using SkillShot = Scaling;

		struct Fatigue : Spread
		{
			/// Duration in seconds that must pass before fatigue sets in for an actor with 0 (`from`) and 100 (`to`) max Stamina (bows) or Magicka (spells).
			Bounds threshold{};

			static Fatigue FromIni(CSimpleIniA& ini, const char* section, const Keys::Fatigue& keys, const Fatigue& defaults);
		};

		struct Attack
		{
			bool aiming = true;

			Aim accuracy;
			Aim precision;

			/// What a fully blind shooter's Accuracy (max spread), Precision (focus) and Retargeting (step size, concentration spells/staves only) are
			/// multiplied by. x1.00 means blindness doesn't matter, above it aiming gets worse, below it better. Less blindness scales the change
			/// down linearly. See `Details::BlindedFactor`.
			float accuracyBlindness = 1.0f;
			float precisionBlindness = 1.0f;
			float retargetBlindness = 1.0f;

			Aim retarget;

			SkillSources skills;

			Speed        speed;
			RangeScaling range;
			SkillShot    skillShot;

			Steadying steadying;
			Fatigue   fatigue;

			inline const Aim& AimFor(SkillUsageContext context) const {
				assert(context == kAccuracy || context == kPrecision);
				return context == kAccuracy ? accuracy : precision;
			}

			inline float BlindnessFor(SkillUsageContext context) const {
				assert(context == kAccuracy || context == kPrecision || context == kRetarget);
				return context == kAccuracy ? accuracyBlindness : context == kPrecision ? precisionBlindness : retargetBlindness;
			}
		};

		/// Controls whether the mod works for given characters.
		bool enabled = true;

		/// The body parts a skill shot may aim at. NPCs only. Hands and legs are only offered to targets that have them.
		bool headshots = true;
		bool armshots = false;
		bool legshots = false;

		Attack bow;
		Attack crossbow;
		Attack spell;
		Attack concentrationSpell;
		Attack staff;
		Attack concentrationStaff;
		Attack scroll;

		/// Options that are `kNone` in the defaults are not supported: their keys are ignored with a warning.
		static Config FromIni(CSimpleIniA& ini, const char* section, const Config& defaults);

		const Attack& For(const struct AttackKind& kind) const;
	};

	/// What differs between kinds of ranged attacks, so that options can be walked generically instead of listed one by one.
	struct AttackKind
	{
		const char* name;

		Config::Attack Config::* config;

		/// Whether the kind is a concentration spell/staff, as opposed to its fire-and-forget counterpart.
		bool concentration;

		/// Whether the kind is magic (spells, staves, scrolls) rather than a bow or crossbow. The menu only offers magic skills for those.
		bool magic;

		/// Whether the kind supports range. Only concentration spells/staves do.
		bool range;

		/// Null if the mod doesn't handle this kind of spell.
		static const AttackKind* For(const RE::MagicItem* spell);

		static const AttackKind kBow;
		static const AttackKind kCrossbow;
		static const AttackKind kSpell;
		static const AttackKind kConcentrationSpell;
		static const AttackKind kStaff;
		static const AttackKind kConcentrationStaff;
		static const AttackKind kScroll;

		static const std::array<std::reference_wrapper<const AttackKind>, 7> kAll;
	};

	inline constexpr AttackKind AttackKind::kBow{ .name = "Bow", .config = &Config::bow, .concentration = false, .magic = false, .range = false };
	inline constexpr AttackKind AttackKind::kCrossbow{ .name = "Crossbow", .config = &Config::crossbow, .concentration = false, .magic = false, .range = false };
	inline constexpr AttackKind AttackKind::kSpell{ .name = "Spell", .config = &Config::spell, .concentration = false, .magic = true, .range = false };
	inline constexpr AttackKind AttackKind::kConcentrationSpell{ .name = "ConcentrationSpell", .config = &Config::concentrationSpell, .concentration = true, .magic = true, .range = true };
	inline constexpr AttackKind AttackKind::kStaff{ .name = "Staff", .config = &Config::staff, .concentration = false, .magic = true, .range = false };
	inline constexpr AttackKind AttackKind::kConcentrationStaff{ .name = "ConcentrationStaff", .config = &Config::concentrationStaff, .concentration = true, .magic = true, .range = true };
	inline constexpr AttackKind AttackKind::kScroll{ .name = "Scroll", .config = &Config::scroll, .concentration = false, .magic = true, .range = false };

	inline constexpr std::array<std::reference_wrapper<const AttackKind>, 7> AttackKind::kAll{ AttackKind::kBow, AttackKind::kCrossbow, AttackKind::kSpell, AttackKind::kConcentrationSpell, AttackKind::kStaff, AttackKind::kConcentrationStaff, AttackKind::kScroll };

	inline constexpr Keys Keys::kBow = Keys::Build(AttackKind::kBow.name);
	inline constexpr Keys Keys::kCrossbow = Keys::Build(AttackKind::kCrossbow.name);
	inline constexpr Keys Keys::kSpell = Keys::Build(AttackKind::kSpell.name);
	inline constexpr Keys Keys::kConcentrationSpell = Keys::Build(AttackKind::kConcentrationSpell.name);
	inline constexpr Keys Keys::kStaff = Keys::Build(AttackKind::kStaff.name);
	inline constexpr Keys Keys::kConcentrationStaff = Keys::Build(AttackKind::kConcentrationStaff.name);
	inline constexpr Keys Keys::kScroll = Keys::Build(AttackKind::kScroll.name);

	constexpr const Keys& Keys::For(const AttackKind& kind) {
		if (&kind == &AttackKind::kBow) return kBow;
		if (&kind == &AttackKind::kCrossbow) return kCrossbow;
		if (&kind == &AttackKind::kSpell) return kSpell;
		if (&kind == &AttackKind::kConcentrationSpell) return kConcentrationSpell;
		if (&kind == &AttackKind::kStaff) return kStaff;
		if (&kind == &AttackKind::kConcentrationStaff) return kConcentrationStaff;
		return kScroll;
	}

	inline constexpr Config::Scaling Config::Scaling::kNone{ false, Bounds{}, 0.0f };

	inline const Config::Attack& Config::For(const AttackKind& kind) const {
		return this->*kind.config;
	}

	namespace Defaults
	{
		inline Config::Scaling Scale(float from, float to, float skillMultiplier = 1.0f) {
			return { true, { from, to }, skillMultiplier };
		}

		/// How tightly shots cluster towards the aim, at 0 and at 100 skill. See `RandomSpreadOffset`.
		inline Config::Aim Focus(float from = 0.75f, float to = 1.25f) {
			return { 1.0f, { from, to } };
		}

		inline Config::Steadying Steadying(float halfDuration) {
			return { { true }, 0.3f, halfDuration };
		}

		inline Config::Fatigue Fatigue(bool enabled, float halfDuration, float minThreshold, float maxThreshold, float maxPenalty = 1.0f) {
			return { { { enabled }, maxPenalty, halfDuration }, { minThreshold, maxThreshold } };
		}

		/// What a fully blind shooter's Accuracy, Precision and (concentration kinds) Retargeting are multiplied by, per kind. Placeholders still to be tuned.
		/// Sight finds the aim point, so Accuracy suffers most where hands and touch can't make up for it: staves worst, crossbows (mostly a trigger
		/// pull) and spells (hands) less. Precision is how consistently the shot is made, which touch can even improve. Scrolls auto-aim, and
		/// concentration kinds lose the sight of how unstable the stream is instead.
		inline void ApplyBlindnessDefaults(Config& config) {
			const auto set = [](Config::Attack& attack, float accuracy, float precision, float retarget = 1.0f) {
				attack.accuracyBlindness = accuracy;
				attack.precisionBlindness = precision;
				attack.retargetBlindness = retarget;
			};
			set(config.bow, 1.8f, 0.8f);
			set(config.crossbow, 1.5f, 1.0f);
			set(config.spell, 1.5f, 0.8f);
			set(config.staff, 1.8f, 1.3f);
			set(config.scroll, 1.0f, 1.0f);
			set(config.concentrationSpell, 1.0f, 1.0f, 1.5f);
			set(config.concentrationStaff, 1.0f, 1.0f, 1.5f);
		}

		inline Config Player() {
			using Skills = Config::SkillSources;
			using enum SkillSource;

			Config config;
			config.bow = { .accuracy = { 1.1f, { 1.0272f, 0.0784f } }, .precision = { 1.0f, { 0.75f, 1.75f } }, .skills = Skills{ .aiming = kArchery, .speed = kArchery, .range = kTotal }, .speed = Scale(0.57f, 2.0f), .skillShot = Config::Scaling::kNone, .steadying = Steadying(2.0f), .fatigue = Fatigue(true, 1.5f, 0.5f, 2.5f, 0.5f) };
			config.crossbow = { .accuracy = { 1.5f, { 0.32f, 0.3f } }, .precision = { 1.0f, { 0.6f, 0.8f } }, .skills = Skills{ .aiming = kArchery, .speed = kArchery, .range = kTotal }, .speed = Scale(0.87f, 1.2f), .skillShot = Config::Scaling::kNone, .steadying = Steadying(3.0f), .fatigue = Fatigue(false, 1.5f, 0.5f, 2.5f, 0.5f) };
			config.spell = { .accuracy = { 1.0f, { 0.72f, 0.205f } }, .precision = { 1.0f, { 0.5f, 1.0f } }, .skills = Skills{ .aiming = kHighest, .speed = kSpellSkill, .range = kTotal }, .speed = Scale(0.35f, 2.0f), .skillShot = Config::Scaling::kNone, .steadying = Steadying(2.0f), .fatigue = Fatigue(false, 1.5f, 1.0f, 4.0f, 0.5f) };
			config.concentrationSpell = { .accuracy = { 1.0f, { 1.6f, 0.5f } }, .precision = { 1.0f, { 0.3f, 0.7f } }, .retarget = { 1.0f, { 1.4f, 0.4f } }, .skills = Skills{ .aiming = kHighest, .speed = kSpellSkill, .range = kSpellSkill }, .speed = Scale(0.35f, 2.0f), .range = Scale(0.8f, 3.0f), .skillShot = Config::Scaling::kNone, .steadying = Steadying(2.0f), .fatigue = Fatigue(true, 4.0f, 1.0f, 3.0f) };
			config.staff = { .accuracy = { 1.0f, { 0.88f, 0.3f } }, .precision = { 1.0f, { 0.5f, 0.75f } }, .skills = Skills{ .aiming = kHighest, .speed = kEnchanting, .range = kTotal }, .speed = Scale(0.74f, 1.4f, 1.3f), .skillShot = Config::Scaling::kNone, .steadying = Steadying(3.0f), .fatigue = Fatigue(false, 1.5f, 1.0f, 4.0f, 0.5f) };
			config.concentrationStaff = { .accuracy = { 1.0f, { 1.8f, 1.0f } }, .precision = { 1.0f, { 0.3f, 0.5f } }, .retarget = { 1.0f, { 0.6f, 0.2f } }, .skills = Skills{ .aiming = kHighest, .speed = kEnchanting, .range = kEnchanting }, .speed = Scale(0.74f, 1.4f, 1.3f), .range = Scale(1.45f, 2.0f), .skillShot = Config::Scaling::kNone, .steadying = Steadying(3.0f), .fatigue = Fatigue(false, 4.0f, 1.0f, 3.0f) };
			config.scroll = { .accuracy = { 0.01f, { 0.35f, 0.35f } }, .precision = { 0.01f, { 0.8f, 0.8f } }, .skills = Skills{ .aiming = kHighest, .speed = kSpellSkill, .range = kTotal }, .speed = Scale(1.4f, 1.65f), .skillShot = Config::Scaling::kNone, .steadying = { { false }, 0.3f, 2.0f }, .fatigue = Fatigue(true, 5.0f, 1.0f, 1.0f, 3.0f) };
			ApplyBlindnessDefaults(config);
			return config;
		}

		inline Config NPC() {
			using Skills = Config::SkillSources;
			using enum SkillSource;

			Config config;
			config.bow = { .accuracy = { 1.1f, { 1.2840254f, 0.0784f } }, .precision = { 1.0f, { 0.75f, 1.75f } }, .skills = Skills{ .aiming = kArchery, .speed = kArchery, .range = kTotal }, .speed = Scale(0.57f, 2.0f), .skillShot = { true, { 0.0f, 0.4f } }, .steadying = Steadying(2.0f), .fatigue = Fatigue(true, 1.5f, 0.5f, 2.5f, 0.5f) };
			config.crossbow = { .accuracy = { 1.5f, { 0.4f, 0.3f } }, .precision = { 1.0f, { 0.6f, 0.8f } }, .skills = Skills{ .aiming = kArchery, .speed = kArchery, .range = kTotal }, .speed = Scale(0.87f, 1.2f), .skillShot = { true, { 0.0f, 0.2f } }, .steadying = Steadying(3.0f), .fatigue = Fatigue(false, 1.5f, 0.5f, 2.5f, 0.5f) };
			config.spell = { .accuracy = { 1.0f, { 0.9f, 0.205f } }, .precision = { 1.0f, { 0.5f, 1.0f } }, .skills = Skills{ .aiming = kHighest, .speed = kSpellSkill, .range = kTotal }, .speed = Scale(0.35f, 2.0f), .skillShot = { false, { 0.0f, 0.35f } }, .steadying = Steadying(2.0f), .fatigue = Fatigue(false, 1.5f, 1.0f, 4.0f, 0.5f) };
			config.concentrationSpell = { .accuracy = { 1.0f, { 1.6f, 0.5f } }, .precision = { 1.0f, { 0.3f, 0.7f } }, .retarget = { 1.0f, { 1.4f, 0.4f } }, .skills = Skills{ .aiming = kHighest, .speed = kSpellSkill, .range = kSpellSkill }, .speed = Scale(0.35f, 2.0f), .range = Scale(0.8f, 3.0f), .skillShot = { false, { 0.0f, 0.35f } }, .steadying = Steadying(2.0f), .fatigue = Fatigue(true, 4.0f, 1.0f, 3.0f) };
			config.staff = { .accuracy = { 1.0f, { 1.1f, 0.3f } }, .precision = { 1.0f, { 0.5f, 0.75f } }, .skills = Skills{ .aiming = kHighest, .speed = kEnchanting, .range = kTotal }, .speed = Scale(0.74f, 1.4f, 1.6f), .skillShot = { false, { 0.0f, 0.35f } }, .steadying = Steadying(3.0f), .fatigue = Fatigue(false, 1.5f, 1.0f, 4.0f, 0.5f) };
			config.concentrationStaff = { .accuracy = { 1.0f, { 2.0f, 1.0f } }, .precision = { 1.0f, { 0.3f, 0.5f } }, .retarget = { 1.0f, { 0.6f, 0.2f } }, .skills = Skills{ .aiming = kHighest, .speed = kEnchanting, .range = kEnchanting }, .speed = Scale(0.74f, 1.4f, 1.6f), .range = Scale(1.45f, 2.0f, 1.6f), .skillShot = { false, { 0.0f, 0.35f } }, .steadying = Steadying(3.0f), .fatigue = Fatigue(false, 4.0f, 1.0f, 3.0f) };
			config.scroll = { .accuracy = { 0.01f, { 0.35f, 0.35f } }, .precision = { 0.01f, { 0.8f, 0.8f } }, .skills = Skills{ .aiming = kHighest, .speed = kSpellSkill, .range = kTotal }, .speed = Scale(1.4f, 1.65f), .skillShot = { false, { 0.0f, 0.35f } }, .steadying = { { false }, 0.3f, 2.0f }, .fatigue = Fatigue(true, 5.0f, 1.0f, 1.0f, 3.0f) };
			ApplyBlindnessDefaults(config);
			return config;
		}
	}

	struct PlayerConfig : Config
	{
		static PlayerConfig FromIni(CSimpleIniA& ini);

		static const PlayerConfig defaultConfig;
	};
	inline const PlayerConfig PlayerConfig::defaultConfig{ Defaults::Player() };

	struct NPCConfig : Config
	{
		std::vector<std::string> excludeKeywords{};
		std::vector<std::string> includeKeywords{};

		static NPCConfig FromIni(CSimpleIniA& ini);

		static const NPCConfig defaultConfig;
	};
	inline const NPCConfig NPCConfig::defaultConfig{ Defaults::NPC(), {}, { "ActorTypeNPC", "UpdateDuringArchery" } };

	inline NPCConfig    NPC{ NPCConfig::defaultConfig };
	inline PlayerConfig Player{ PlayerConfig::defaultConfig };

	/// Returns configuration for given actor. Note, that only common configuration is returned.
	inline const Config& For(const RE::Actor* actor) {
		if (actor && actor->IsPlayerRef()) {
			return Player;
		} else {
			return NPC;
		}
	}
	/// Splits a comma-separated list of keyword EditorIDs, trimming whitespace around each and dropping empty entries.
	std::vector<std::string> ParseKeywords(std::string_view list);

	/// Whether the mod applies to the actor. Cached per actor, so that hooks running every frame don't look up keywords each time.
	bool ShouldLearn(RE::Actor* actor);

	/// Forgets the cached results of `ShouldLearn`. Must be called whenever anything affecting them changes.
	void InvalidateActorCache();

	spdlog::level::level_enum LogLevel();

	void Load();
	void Save();

	/// Writes all options at their current values to the INI file.
	void Export();

	void ResetNPC();
	void ResetPlayer();

	/// An incremental identifier that can be used to track when Options are reloaded.
	inline unsigned int currentID = 0;
}

namespace NLA::Settings
{
	inline float fBowNPCSpreadAngle() {
		auto settings = RE::GameSettingCollection::GetSingleton();
		if (auto setting = settings->GetSetting("fBowNPCSpreadAngle")) {
			return setting->data.f;
		}
		return 4.0f;
	}
}
