# Greenfield - V2.0+ Enterprise Vision

This folder contains documents describing the target v2.0+ architecture with enterprise-grade patterns. These represent the **future direction** of the ERP system, not the current v1.0 implementation.

## Purpose

While **v1.0 (proposed schema)** prioritizes pragmatism and speed-to-market, **v2.0+ (greenfield)** adds enterprise patterns:

- Full Event Sourcing (state reconstruction from events)
- CQRS (Command Query Responsibility Segregation)
- Rich Fulfillment lifecycle (explicit order fulfillment tracking)
- Immutable OrderItem records (pristine customer intent)
- Advanced audit and compliance features

## Documents

### 1. [01-design-greenfield.md](./01-design-greenfield.md)
**Complete v2.0 system design** with all enterprise features.

**Contains:**
- Philosophy and design principles
- Bounded contexts and domain model
- Order lifecycle with explicit fulfillment
- Stock management with complex state machines
- Delivery and logistics integration
- Fiscal and financial processing
- Complete event architecture (full ES/CQRS)
- State machines for all major entities
- Testing strategy for event-sourced system

**When to read:**
- Understand the complete vision before diving into implementation
- Reference for v2.0 planning and architectural decisions
- Long-term strategy discussion with stakeholders

**Status:** Reference / Future Vision

---

### 2. [02-v1-v2-evolution-roadmap.md](./02-v1-v2-evolution-roadmap.md)
**Strategic roadmap for evolving from v1 to v2+** with phased implementation.

**Contains:**
- 8-part evolution strategy
- Phase-by-phase implementation (2.1 through 2.5)
- Technical debt assessment
- Implementation timeline estimates
- Risk mitigation strategies
- Migration checklists and prerequisites
- Feature mapping (how v1 foundations enable v2 capabilities)

**When to read:**
- Planning the v1.1 → v2.0 migration
- Understanding what stays vs what changes
- Assessing team readiness for advanced patterns
- Risk planning for Event Sourcing migration

**Status:** Strategic Roadmap / When to Proceed to v2

---

### 3. [03-greenfield-vs-schema-comparison.md](./03-greenfield-vs-schema-comparison.md)
**Detailed comparison between greenfield (v2.0) and proposed (v1.0) designs.**

**Contains:**
- 7 deep-dive sections on architectural differences:
  1. Fulfillment Model: 1:N (greenfield) vs M:N Allocation (v1)
  2. Stock Management: Complex state machine vs simplified quantity
  3. Event Sourcing: Full vs Hybrid
  4. Reservation vs Consumption: Explicit vs implicit
  5. Order Item Mutability: Immutable vs mutable
  6. Delivery Integration: Direct vs indirect
  7. Returns Handling: Explicit StockReturn vs is_estornado flag

- Strategic assessment table (feature completeness)
- Immediate gaps to close in v1
- Recommendations and trade-off analysis

**When to read:**
- Understand why v1 makes certain simplifications
- Decision context for v1.0 schema choices
- Strategic reference for v2.0 planning
- Architecture review meetings

**Status:** Decision Reference / Feature Comparison

---

## Reading Path

### New to the project?
1. Read `02-v1-v2-evolution-roadmap.md` for strategic overview
2. Read `03-greenfield-vs-schema-comparison.md` to understand trade-offs
3. Read `01-design-greenfield.md` for complete v2.0 vision

### Planning v2.0 implementation?
1. Review `02-v1-v2-evolution-roadmap.md` phases 2.1-2.5
2. Check `03-greenfield-vs-schema-comparison.md` for feature mapping
3. Reference specific sections of `01-design-greenfield.md` for details

### Implementing Event Sourcing in v2+?
- Focus on `01-design-greenfield.md` event architecture section
- Review `02-v1-v2-evolution-roadmap.md` Phase 2.3 (Full Event Sourcing)
- Cross-reference with `../rascunhos/event-sourcing-analise.md` for patterns

---

## Relationship to Other Documents

### V1.0 (Current - in 03-decisoes/)
- **02-schema-redesenhado.md** - Active implementation schema for v1.0
- **02-schema-visual-overview.md** - Visual patterns and diagrams for v1.0

### V2.0+ (This folder)
- **01-design-greenfield.md** - Complete v2.0 vision
- **02-v1-v2-evolution-roadmap.md** - Path from v1 to v2
- **03-greenfield-vs-schema-comparison.md** - Feature comparison

### Implementation Patterns (in rascunhos/)
- **alocacao-m2n-workflow.md** - M:N allocation (v1.0 pattern, foundation for v2 Fulfillment)
- **event-sourcing-analise.md** - Event Sourcing overview (hybrid in v1, full in v2)

---

## Key Insight: Evolution, Not Revolution

The v1.0 proposed schema is designed to **layer new capabilities** rather than replace:

```text
v1 to v2 Evolution (Additive, Non-Destructive)

Alocacao (v1) ───────────────► Alocacao + Fulfillment (v2)
VendaItem (v1) ──────────────► VendaItem + OrderItem + Adjustments (v2)
*_events tables (v1) ────────► EventStore + Projectors + Replay (v2)
is_estornado flag (v1) ──────► EstoqueDevolucoes entity (v2)
```

This means:
- ✅ No massive rewrites when v2 launches
- ✅ Backward compatibility maintained during transition
- ✅ Gradual team onboarding to advanced patterns
- ✅ Risk mitigation via feature flags and parallel systems
- ✅ Reversible decisions if v2 patterns don't work

---

## Decision Records

All architectural decisions comparing v1 vs v2 are documented in:
- `03-greenfield-vs-schema-comparison.md` - Feature-by-feature comparison
- `02-v1-v2-evolution-roadmap.md` - Reasoning for phased approach
- `01-design-greenfield.md` - Complete rationale for v2.0 architecture

---

## Status Tracking

| Document | Status | Last Updated |
|----------|--------|--------------|
| 01-design-greenfield.md | ✅ Reference / v2.0 Vision | 2026-01-10 |
| 02-v1-v2-evolution-roadmap.md | ✅ Strategic Roadmap | 2026-01-10 |
| 03-greenfield-vs-schema-comparison.md | ✅ Decision Reference | 2026-01-10 |

**Next Steps:**
- v1.0 ships with proposed schema + hybrid ES foundation
- v1.2-v1.3: Prepare Event Store infrastructure
- v1.4: Gradual entity migration to full ES
- v2.0: Launch with complete enterprise patterns
