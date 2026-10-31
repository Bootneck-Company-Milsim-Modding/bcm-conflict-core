# Enforce operator/type-context compatibility — 2026-10-02

Status: REPORTED BCM EXPRESSION CORRECTED / STATIC CHECKS COMPLETED /
WORKBENCH GATE REQUIRED. NO new Enforce compile or gameplay PASS.

## Exact first causal expression

Workbench 1.8.0.13 / engine 192142 reported:
Scripts/Game/TB_PVE_M3_3/TB_PVE_M3_3_TacticalStagingPolicy.c(192),
Unknown operator '%'.

Before:

```c
float angle = direction * 0.392699; float radius = 60 + (attempt % 3) * 30;
```

The intended radius is 60, 90, 120 metres in a three-attempt cycle, not
angular normalization. attempt is an integer loop index, beginning at zero.
The direction is independently selected modulo sixteen and converted to
radians; the radius calculation must not change that selection.

Correction:

```c
int radiusBand = attempt % 3;
float angle = direction * 0.392699;
float radius = 60 + radiusBand * 30;
```

The integer remainder is resolved in an explicit integer declaration BEFORE
it participates in the float radius expression. No blind cast, rounded float
index, changed radius, new helper or new API is introduced.

## What the installed language actually supports

It would be incorrect to conclude that Enforce forbids every '%' expression.
The installed vanilla Game archive contains 109 executable '%' occurrences,
including:

- scripts/Game/AI/Behavior/ActivityFeatures/SCR_AIActivitySmokeCoverFeature.c(168):
  `int pair = i % 2;`
- scripts/Game/AI/Behavior/SCR_AIDefendActivity.c(67–70):
  `int directionAngle = Math.Round(angleToDefend + sector * index);`
  then `directionAngle = directionAngle % 360;`, then float trig math.
- SCR_AIGroupMovementComponent and SCR_AIFindResupplier: integer cyclic array
  indexing with group/count-based divisors.

The reported mixed radius expression is rejected by the actual compiler.
The error is therefore expression/type-context-specific, not proof that
integer remainder is unsupported. Separating the integer stage follows the
installed vanilla integer-first/trigonometric pattern. Expected numeric-type
propagation into the float formula is the explanation consistent with that
evidence; the compiler's internal inference is not independently observable.
The corrected statement still needs the normal compiler gate.

Installed Core Math.c also provides Math.Mod(float v,float div) and
Math.Repeat(float value,float range). They are real APIs, not guessed names.
They are not required here. Replacing general integer seed/index arithmetic
with floating-point Math.Mod could lose integer precision and is not justified.

## Whole-tree '%' classification and decisions

All 45 BCM Game sources were lexically scanned, distinguishing executable
code from double-quoted strings, escaped quotes, line comments and block
comments. There are 25 executable operators on 23 source lines and 2067
formatting occurrences in strings; zero other-string/comment occurrences.

Every executable expression appears in the inventory below. Apart from the
corrected float-context radius formula, all are integer-only:

- seed/start selection and deterministic variation (including nested integer
  modulo) use integer seeds, indices, clamped positive variation divisor;
- origins.Count() is positive before FIA source indexing;
- attempts, ordinals and serials are integer; positive periods 3/4/16;
- proof/diagnostic cadence uses integer ticks and positive periods 5/10;
- countdown seconds use integer remaining and divisor 60;
- physical actor placement uses integer index and divisor 4.

These are consistent with installed vanilla integer remainder/cyclic indexing.
No other unsupported remainder context was found. They were deliberately
NOT mechanically rewritten. Preserving valid installed-language patterns is
safer than replacing every percent sign or converting indices to float.

## Semantic checks: staging/scoring untouched

- Positive attempts: radius cycle remains 60/90/120; zero remains 60.
- Signed mathematical remainder: the explicit integer stage preserves the
  same signed result for negative operands; it does not silently introduce
  floor/positive modulo. Negative attempts cannot arise in the current loop.
  Static signed-boundary checks include INT_MIN/INT_MAX; these are model
  checks, not evidence of new runtime/compiler acceptance.
- Direction and origin indexing expressions are unchanged.
- Sixteen-sector permutations tested for 32 serials and six group ordinals:
  step five visits all sixteen sectors before repetition.
- Accepted USSR multi-axis thresholds remain 80, then 45, then safe relaxation;
  group separation, candidate bounds, terrain/navigation, player exclusion,
  visibility predicates and degraded fallback are unchanged.
- Bearing remains atan2 -> degrees; negative bearings add 360. Separation
  greater than 180 becomes 360-separation. Tests include 359/1, 350/10,
  0/180 and identical bearings. No new wrap-domain change was made.
- Legacy and territorial seed variation expressions, weights and tie logic
  are unchanged. Model rankings and all earlier checks are rerun.
- Existing eventual integer serial overflow limitations are not redesigned
  in a compile-only correction.

## Enforce-specific syntax review

Reviewed the recent staging, scoring, recapture, consolidation, occupation,
garrison, campaign and capture changes alongside all compiled proof consumers.

| Construct | Finding / action |
|---|---|
| Integer '%' | Supported installed patterns; retain typed integer uses |
| Remainder embedded in float formula | Corrected with explicit int stage |
| += / -= / ++ / -- | Installed vanilla examples; retain |
| Logical && / \|\| / ! | Installed vanilla control-flow patterns; retain |
| Bitwise &, \|, \|=, << | Existing integer/flag usage; installed vanilla examples; retain |
| >> token | The BCM occurrences are nested generic closers, not right-shift expressions |
| Ternary / nested ternary | No executable '?' found |
| C-style casts / foreign C++ cast keywords / === / ?? / => / -> / ** | None found |
| Casts / enum conversions | Existing .Cast and installed implicit enum/int patterns; no new cast introduced |
| String formulas | Whole-tree bounded-concatenation screening still passes |
| Array indexing | Integer indices/counts; no float modulo index found |
| Default / out / ref parameters | Current signatures retained; whole override/caller audit rerun |
| inout | No newly introduced inout dependency |
| Null comparisons | Existing entity/object guards retained |
| switch / foreach | Existing Enforce forms; no foreign for-of/map destructuring introduced |
| Overloads / visibility | 54 overrides match; no implicit inherited same-name mismatch found |
| Complex expressions | No other demonstrated compiler-incompatible expression found; actual compiler budget remains authoritative |

No other source change was justified by this review. In particular no vanilla
Tuple/UI/ScenarioFramework error was modified: those remain downstream until
the BCM Game module compiles.

## Validator improvements and rerun

Tools/Test-ThreeFactionCorrectiveStatic.py now:

- classifies every '%' occurrence and records every executable use;
- rejects remainder in float-destination/float-operand/floating-literal contexts;
- screens a concrete set of foreign-language operators/casts;
- regression-tests the EXACT old line-192 expression and confirms rejection;
- regression-tests valid integer staging and comments/string placeholders,
  avoiding a blanket '%' ban;
- verifies the installed SCR_AIDefendActivity integer-first modulo pattern;
- checks the unchanged radius sequence, sector permutations and angle seam cases.

The lexical screen is conservative and file-local, NOT a full typed Enforce
parser. Float argument propagation, shadowing, arbitrary overload resolution,
integer overflow, implicit conversions and expression budgets cannot all be
certified this way. A passing validator is explicitly not an Enforce compile.

Final exit code 0: 45 files, 54 override comparisons, 137 audited direct
calls, 479 PrintFormat calls, 371 local enum references, 94 unique BCM
class definitions, unchanged 13-field serializer/parser, valid installed FIA
GUIDs and characters, capture-superiority/scoring/package model checks.

## Better actual compiler feedback

The installed Tools directory has Workbench and its DLLs/docs, not a separate
standalone Enforce compiler executable. Its docs link to the official wiki.

The official [startup parameters](https://community.bistudio.com/wiki/Arma_Reforger:Startup_Parameters)
document ScriptEditor `-validate` (failure -1 / success 0), in addition to
`-wbsilent`. This is a supported, more targeted compile-feedback entry point,
but still launches the same Workbench executable and is not evidence that
the known 0xc0000142 startup environment is repaired. It was NOT executed.

The official [Workbench NET API](https://community.bistudio.com/wikidata/external-data/arma-reforger/EnfusionScriptAPIPublic/Page_NetApi.html)
provides built-in ValidateScripts with Configuration and structured
Errors/Warnings/Success. This can validate through an ALREADY normally
started Workbench, avoiding the failing automated process startup. Installed
Workbench binary markers include ValidateScripts, JsonRPC and
IsWorkbenchRunning, supporting the presence of this interface.

Get-Process found no running Workbench at the investigation time. Therefore
there was no live instance to validate through. No NET API listener was
enabled, no Workbench preference was changed, no guessed port was contacted.
Endpoint availability/configuration and a successful actual compile remain
unverified. Normal Workbench F7 remains the immediate reliable route; NET API
validation is a supported future automation option once a live instance and
its configured endpoint are available.

No GUI-startup retry, ServerDiag/gameplay, publishing or commit was performed.

## Complete files changed this pass

1. Scripts/Game/TB_PVE_M3_3/TB_PVE_M3_3_TacticalStagingPolicy.c
2. Tools/Test-ThreeFactionCorrectiveStatic.py
3. Documentation/TB-Conflict-Core-Three-Faction-Enforce-Operators.md
4. Documentation/TB-Conflict-Core-Three-Faction-Compile-Compatibility.md
5. Documentation/TB-Conflict-Core-Three-Faction-Corrective-Pass.md

Only the typed radius-expression staging was changed in gameplay source.
No weights, packages, recapture design, occupation/consolidation policy,
capture superiority, HQ logic, radio, native AI, world/prefab hierarchy or
Creative Director-authored manager was changed.

WORKBENCH GATE REQUIRED. All static findings are documented; another real
Game-module compile must establish whether any further compiler error remains.

## Executable remainder and final audit inventory

```text
PrintFormat calls checked: 479
Unique BCM class definitions: 94
13-field snapshot / invalid activation reference checks PASS
Capture-superiority boundary checks PASS
Complete override return/type/name/order checks PASS: 54
No gameplay ownership writes / string-formula checks PASS
Whole-Game changed/new contract call-arity checks PASS: 137
Whole-Game local enum member checks PASS: 371
Implicit inherited same-name declaration checks PASS: 0
Whole-Game FIA reference GUID/resource-type checks PASS: 5
Whole-Game conditional / oversized string expression checks PASS
INTEGER REMAINDER REVIEW TB_CC_M4X/TB_CC_M4XP_IntegratedProof.c 262 if (m_iTicks % 5 == 0)
INTEGER REMAINDER REVIEW TB_CC_M4X/TB_CC_M4X_PhysicalProof.c 47 int column = index % 4;
INTEGER REMAINDER REVIEW TB_CC_M4X/TB_CC_M4X_PhysicalProof.c 62 if (m_iElapsed % 10 == 0)
INTEGER REMAINDER REVIEW TB_CC_M4_2A/TB_CC_M4_2A_CampaignStart.c 76 int index = seed % candidateIds.Count();
INTEGER REMAINDER REVIEW TB_CC_M4_3/TB_CC_M4_3_StrategicEvaluator.c 256 int variation = ((seed % 997) * 31 + i * 17) % 7; if (variation < 0) variation = -variation;
INTEGER REMAINDER REVIEW TB_CC_M4_3/TB_CC_M4_3_StrategicEvaluator.c 295 int variation = ((seed % 997) * 31 + index * 17) % (m_iTieVariation + 1);
INTEGER REMAINDER REVIEW TB_CC_M4_4/TB_CC_M4_4H_VictoryPressureProof.c 74 if (s_Ticks % 5 == 0 && s_ObservedSeizing)
INTEGER REMAINDER REVIEW TB_CC_M4_4/TB_CC_M4_4H_VictoryPressureProof.c 79 if (s_Ticks % 5 == 0) PrintFormat(                                                                                                          , s_Phase, s_Attacker, s_Target, Owner(s_Target), s_Campaign.GetOutcome(), s_Campaign.GetPressureRemainingSeconds(), s_Campaign.GetRevision());
INTEGER REMAINDER REVIEW TB_CC_M4_4/TB_CC_M4_4_PhysicalAuthorityProof.c 41 if (s_Ticks % 5 == 0) LogPhysical();
INTEGER REMAINDER REVIEW TB_CC_M4_4/TB_CC_M4_4_RemainingAuthorityProof.c 74 if (s_Ticks % 5 == 0) PrintFormat(                                                                                         , s_Attacker, s_Target, Owner(s_Target), base.GetCaptureState(), s_Campaign.GetRevision(), s_Ticks);
INTEGER REMAINDER REVIEW TB_CC_M4_4/TB_CC_VictoryPressurePresentation.c 21 if (remaining < 0 || remaining % 10 == 0) PrintFormat(                                                                                               , outcome, remaining, duration, foothold);
INTEGER REMAINDER REVIEW TB_CC_M4_4/TB_CC_VictoryPressurePresentation.c 67 int seconds = remaining % 60;
INTEGER REMAINDER REVIEW TB_PVE_M1_1/TB_PVE_M1_1_CaptureProof.c 330 if (captureState != m_TB_PVE_M1_1_LastCaptureState || ownerKey != m_TB_PVE_M1_1_LastOwnerKey || (m_TB_PVE_M1_1_DiagnosticTicks % 10) == 0)
INTEGER REMAINDER REVIEW TB_PVE_M1_1/TB_PVE_M1_1_CaptureProof.c 617 if (livingInside != m_TB_PVE_M1_1_LastLivingInside || captureState != m_TB_PVE_M1_1_LastCaptureState || ownerKey != m_TB_PVE_M1_1_LastOwnerKey || (m_TB_PVE_M1_1_DiagnosticTicks % 10) == 0)
INTEGER REMAINDER REVIEW TB_PVE_M2_1/TB_PVE_M2_1_VerticalSliceProof.c 38 if ((s_Phase == 4 || s_Phase == 5 || s_Phase == 6) && s_Ticks % 5 == 0)
INTEGER REMAINDER REVIEW TB_PVE_M2_5/TB_PVE_M2_5_CampaignOutcomeProof.c 267 if (state != s_LastM2_5BDiagnostic || s_Ticks == 1 || (s_Ticks % 10) == 0)
INTEGER REMAINDER REVIEW TB_PVE_M3_3/TB_PVE_M3_3_TacticalStagingPolicy.c 70 int distanceBand = (attempt + attempt / 16) % 4;
INTEGER REMAINDER REVIEW TB_PVE_M3_3/TB_PVE_M3_3_TacticalStagingPolicy.c 73 directionIndex = directionIndex % 16;
INTEGER REMAINDER REVIEW TB_PVE_M3_3/TB_PVE_M3_3_TacticalStagingPolicy.c 189 string source = origins[(attempt + ordinal) % origins.Count()];
INTEGER REMAINDER REVIEW TB_PVE_M3_3/TB_PVE_M3_3_TacticalStagingPolicy.c 191 int direction = (attempt * 5 + ordinal * 7 + m_iQuerySerial * 3) % 16;
INTEGER REMAINDER REVIEW TB_PVE_M3_3/TB_PVE_M3_3_TacticalStagingPolicy.c 192 int radiusBand = attempt % 3;
INTEGER REMAINDER REVIEW TB_PVE_M3_3/TB_PVE_M3_3_TacticalStagingPolicy.c 297 int directionIndex = (m_iQuerySerial * 3 + groupOrdinal * 7 + attempt * 5) % 16;
INTEGER REMAINDER REVIEW TB_PVE_M3_3/TB_PVE_M3_3_TacticalStagingPolicy.c 298 int band = (attempt + attempt / 16 + groupOrdinal) % 4;
Whole-Game percent classification: {'executable': 25, 'string-format': 2067, 'string-other': 0, 'comment': 0}
Whole-Game executable operator inventory: {'&&': 650, '||': 481, '++': 179, '+=': 71, '--': 3, '>>': 2, '|': 7, '&': 3, '<<': 4, '|=': 1, '-=': 2}
Installed Enforce integer-remainder pattern verified: scripts/game/ai/behavior/scr_aidefendactivity.c
Integer radius / sector permutations / wrap separation mathematical checks PASS
Repository-wide static checks complete: 45 Game source files; NOT Enforce compile PASS
```
