# H3 — Genesis v2 (proposed, 2026-09-22)

**Context.** Closure toward a four-layer split. Question every assumption.
Foundation solid, not merely small. Explicit rejected-assumption list.

**Reasoning.** Foundation / Silicon / Board / Research import rules. Research
names must not enter foundation because they sound unique. Promote only if they
add a property beyond memory + MMU + `nxu_cpu_exec_context` + signals + time.

**Proposals.** A1–A12; 25 invariants; load-bearing interrogation tables;
S0–S14 unchanged in spirit; `nxu_cpu_exec_context` as execution abstraction
(still glossed as “atom” in places).

**Experiments.** None (design-only). Two fact sources required per later stage.

**Decisions (proposed, not accepted).** Four-layer map; DTB as candidate not
truth; stay at entry EL; no function-pointer dispatch; single-CPU signal route.

**Rejected.** Same as v1 plus: UART-first as architecture; Linux boot protocol
as trust; `present == online`; GICv3 *is* the architecture; identity-map all RAM.

**Mistakes / still open at end of v2.** See Genesis v2 closure (H4): EL2 timer
PPI; running vs waiting; MMU-on entry; chronicle sharing; probe vs ownership;
FDT decoder placement.

**Evolution.** v2 is the architectural baseline. H4 is the closure delta, not a
redesign.
