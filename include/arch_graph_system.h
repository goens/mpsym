#ifndef _GUARD_ARCH_GRAPH_SYSTEM_H
#define _GUARD_ARCH_GRAPH_SYSTEM_H

#include <memory>

#include "partial_perm_inverse_semigroup.h"
#include "perm_group.h"
#include "task_mapping.h"

namespace cgtl
{

using ProcessorLabel = char const *;
using ChannelLabel = char const *;
using SubsystemLabel = char const *;

#define DEFAULT_PROCESSOR_LABEL ""
#define DEFAULT_CHANNEL_LABEL ""
#define DEFAULT_SUBSYSTEM_LABEL ""

class ArchGraphSystem
{
public:
  ArchGraphSystem()
  : _automorphisms_valid(false)
  {}

  ArchGraphSystem(PermGroup const &automorphisms)
  : _automorphisms(automorphisms),
    _automorphisms_valid(true)
  {}

  virtual unsigned num_processors() const
  { throw std::logic_error("not implemented"); }

  virtual unsigned num_channels() const
  { throw std::logic_error("not implemented"); }

  virtual PermGroup automorphisms()
  {
    if (!_automorphisms_valid) {
      update_automorphisms();
      _automorphisms_valid = true;
    }

    return _automorphisms;
  }

  PermSet automorphisms_generators()
  { return automorphisms().bsgs().strong_generators(); }

  virtual PartialPermInverseSemigroup partial_automorphisms()
  { throw std::logic_error("not implemented"); }

  virtual TaskMapping mapping(TaskMappingRequest const &tmr);

protected:
  PermGroup _automorphisms;
  bool _automorphisms_valid;

private:
  virtual void update_automorphisms()
  { throw std::logic_error("not implemented"); }

  TaskAllocation min_elem_bruteforce(TaskAllocation const &tasks,
                                     unsigned min_pe,
                                     unsigned max_pe);

  TaskAllocation min_elem_approx(TaskAllocation const &tasks,
                                 unsigned min_pe,
                                 unsigned max_pe);
};

class ArchGraphSubsystem
{
public:
  ArchGraphSubsystem(std::shared_ptr<ArchGraphSystem> const &ag,
                     SubsystemLabel label = DEFAULT_SUBSYSTEM_LABEL)
  : _arch_graph_system(ag),
    _label(label)
  {}

  template<typename AG>
  ArchGraphSubsystem(AG const &arch_graph_system,
                     SubsystemLabel label = DEFAULT_SUBSYSTEM_LABEL)
  : _arch_graph_system(std::make_shared<AG>(arch_graph_system)),
    _label(label)
  {}

  ArchGraphSystem *operator->() const
  { return _arch_graph_system.get(); }

  SubsystemLabel label() const
  { return _label; }

private:
  std::shared_ptr<ArchGraphSystem> _arch_graph_system;
  SubsystemLabel _label;
};

// TODO: outstream operators for both of these...

} // namespace cgtl

#endif // _GUARD_ARCH_GRAPH_SYSTEM_H