#include <cassert>
#include <memory>
#include <vector>

#include "bsgs.h"
#include "dbg.h"
#include "perm.h"
#include "pr_randomizer.h"
#include "schreier_generator_queue.h"
#include "schreier_structure.h"
#include "timer.h"

/**
 * @file bsgs_schreier_sims.cc
 * @brief Implements schreier sims algorithm variants.
 *
 * @author Timo Nicolai
 */

namespace cgtl
{

void BSGS::schreier_sims(PermSet const &generators)
{
  DBG(TRACE) << "Executing schreier sims algorithm";

  generators.assert_not_empty();

  // initialize
  std::vector<PermSet> strong_generators;
  std::vector<Orbit> fundamental_orbits;

  schreier_sims_init(generators, strong_generators, fundamental_orbits);

  // run algorithm
  schreier_sims(strong_generators, fundamental_orbits);
}

void BSGS::schreier_sims(std::vector<PermSet> &strong_generators,
                         std::vector<Orbit> &fundamental_orbits)
{
  std::vector<SchreierGeneratorQueue> schreier_generator_queues(base_size());

  DBG(TRACE) << "=== Iterating over Schreier Generators";

  // main loop
  unsigned i = base_size();
  while (i >= 1u) {
    DBG(TRACE) << "== i = " << i;
top:
    schreier_generator_queues[i - 1].update(strong_generators[i - 1],
                                            fundamental_orbits[i - 1],
                                            schreier_structure(i - 1));

    for (Perm const &schreier_generator : schreier_generator_queues[i - 1]) {
      DBG(TRACE) << "Schreier Generator: " << schreier_generator;

      // strip
      TIMER_START("strip");

      Perm strip_perm;
      unsigned strip_level;

      std::tie(strip_perm, strip_level) = strip(schreier_generator, i);

      DBG(TRACE) << "Strips to: " << strip_perm << ", " << strip_level;

      TIMER_STOP("strip");

      // check whether to update base and strong generators
      if (strip_level < base_size() - i || !strip_perm.id()) {
        bool do_extend_base = i == base_size();

        if (do_extend_base) {
          TIMER_START("extend base");

          // extend base
          unsigned bp = 1u;
          for (;;) {
            auto it = std::find(_base.begin(), _base.end(), bp);

            if (it == _base.end() && strip_perm[bp] != bp)
              break;

            ++bp;

            assert(bp <= degree());
          }

          extend_base(bp);

          DBG(TRACE) << "Adjoined new basepoint:";
          DBG(TRACE) << "B = " << _base;

          strong_generators.emplace_back();
          fundamental_orbits.emplace_back();

          TIMER_STOP("extend base");
        }

        // update strong generators and fundamental orbits
        TIMER_START("update strong gens");

        DBG(TRACE) << "Updating strong generators:";

        schreier_sims_update_strong_gens(
          i, {strip_perm}, strong_generators, fundamental_orbits);

        DBG(TRACE) << "S(" << i + 1 << ") = " << strong_generators[i];
        DBG(TRACE) << "O(" << i + 1 << ") = " << fundamental_orbits[i];

        TIMER_STOP("update strong gens");

        // update schreier generator queue
        if (do_extend_base)
          schreier_generator_queues.emplace_back();
        else
          schreier_generator_queues[i].invalidate();

        ++i;

        goto top;
      }
    }

    --i;
  }

  schreier_sims_finish();
}

void BSGS::schreier_sims_random(PermSet const &generators,
                                Options const *options)
{
  DBG(TRACE) << "Executing (random) schreier sims algorithm";

  generators.assert_not_empty();

  std::vector<PermSet> strong_generators;
  std::vector<Orbit> fundamental_orbits;

  if (!options->schreier_sims_random_guarantee) {
    schreier_sims_init(generators, strong_generators, fundamental_orbits);
    schreier_sims_random(strong_generators, fundamental_orbits, options);

  } else {
    // run algorithm (possible repeatedly) until correctness has been achieved
    bool correct = false;

    for (unsigned i = 0u; i <= options->schreier_sims_random_retries; ++i) {
      schreier_sims_init(generators, strong_generators, fundamental_orbits);
      schreier_sims_random(strong_generators, fundamental_orbits, options);

      // we assume that if the BSGS is correct if it has the correct order
      if (options->schreier_sims_random_known_order > 0ULL
          && order() == options->schreier_sims_random_known_order) {
        correct = true;
        break;
      }

      // TODO: use TCSS/Verify to check BSGS for small base groups of large degree
      // if (...) {
      //  correct = true;
      //  break;
      //}
    }

    // force correctness by running the deterministic Schreier Sims algorithm
    if (!correct) {
      DBG(TRACE) << "Executing Schreier Sims algorithm to guarantee correctness";
      schreier_sims(strong_generators, fundamental_orbits);
    }
  }

  schreier_sims_finish();
}

void BSGS::schreier_sims_random(std::vector<PermSet> &strong_generators,
                                std::vector<Orbit> &fundamental_orbits,
                                Options const *options)
{
  // random group element generator
  PrRandomizer pr(_strong_generators);

  unsigned c = 0u;
  while (c < options->schreier_sims_random_w) {
    // generate random group element
    Perm rand_perm = pr.next();
    DBG(TRACE) << "Random group element: " << rand_perm;

    // strip
    Perm strip_perm;
    unsigned strip_level;

    std::tie(strip_perm, strip_level) = strip(rand_perm);

    DBG(TRACE) << "Strips to: " << strip_perm << ", " << strip_level;

    // check whether to update base and strong generators
    bool update_strong_generators = false;

    if (strip_level <= base_size()) {
      update_strong_generators = true;

    } else if (!strip_perm.id()) {
      update_strong_generators = true;

      // extend base
      for (unsigned bp = 1u; bp <= degree(); ++bp) {
        if (strip_perm[bp] != bp) {
          extend_base(bp);

          strong_generators.emplace_back();
          fundamental_orbits.emplace_back();

          DBG(TRACE) << "Adjoined new basepoint:";
          DBG(TRACE) << "B = " << _base;

          break;
        }
      }
    }

    if (update_strong_generators) {
      DBG(TRACE) << "Updating strong generators:";

      // update strong generators
      for (unsigned i = 1u; i < strip_level; ++i) {
        schreier_sims_update_strong_gens(
          i, {strip_perm}, strong_generators, fundamental_orbits);

        DBG(TRACE) << "S(" << (i + 1u) << ") = " << strong_generators[i];
        DBG(TRACE) << "O(" << (i + 1u) << ") = " << fundamental_orbits[i];
      }

      c = 0u;

    } else {
      ++c;
    }
  }
}

void BSGS::schreier_sims_init(PermSet const &generators,
                              std::vector<PermSet> &strong_generators,
                              std::vector<Orbit> &fundamental_orbits)
{
  _base.clear();
  _transversals->clear();
  _strong_generators = generators;

  strong_generators.clear();
  fundamental_orbits.clear();

  // add initial base points
  auto it = _strong_generators.begin();

  while (it != _strong_generators.end()) {
    Perm gen = *it;

    if (gen.id()) {
      it = _strong_generators.erase(it);

    } else {
      ++it;

      if (gen.stabilizes(_base.begin(), _base.end())) {
#ifndef NDEBUG
        bool extended_base = false;
#endif
        for (unsigned bp = 1u; bp <= degree(); ++bp) {
          if (gen[bp] != bp) {
            extend_base(bp);
#ifndef NDEBUG
            extended_base = true;
#endif
            break;
          }
        }

        assert(extended_base && "no generator fixes all base elements");
      }
    }
  }

  strong_generators.resize(base_size());
  fundamental_orbits.resize(base_size());

  // calculate initial strong generator sets
  for (unsigned i = 0u; i < base_size(); ++i) {
    PermSet new_strong_generators;

    for (Perm const &gen : _strong_generators) {
      if (gen.stabilizes(_base.begin(), _base.begin() + i))
        new_strong_generators.insert(gen);
    }

    schreier_sims_update_strong_gens(
      i, new_strong_generators, strong_generators, fundamental_orbits);
  }

  DBG(TRACE) << "=== Initial values";
  DBG(TRACE) << "B = " << _base;
  for (unsigned i = 0u; i < base_size(); ++i) {
    DBG(TRACE) << "S(" << (i + 1u) << ") = " << strong_generators[i];
    DBG(TRACE) << "O(" << (i + 1u) << ") = " << fundamental_orbits[i];
  }
}

void BSGS::schreier_sims_update_strong_gens(
  unsigned i,
  PermSet const &new_strong_generators,
  std::vector<PermSet> &strong_generators,
  std::vector<Orbit> &fundamental_orbits)
{
  for (Perm const &gen : new_strong_generators)
    strong_generators[i].insert(gen);

  update_schreier_structure(i, strong_generators[i]);

  fundamental_orbits[i] = orbit(i);
}

void BSGS::schreier_sims_finish()
{
  _strong_generators.clear();

  for (unsigned i = 0u; i < base_size(); ++i) {
    auto stabilizers(schreier_structure(i)->labels());
    _strong_generators.insert(stabilizers.begin(), stabilizers.end());
  }

  _strong_generators.make_unique();
}

} // namespace cgtl
