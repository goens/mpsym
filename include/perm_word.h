#ifndef _GUARD_PERM_WORD_H
#define _GUARD_PERM_WORD_H

#include <cstddef>
#include <ostream>
#include <vector>

#include "perm.h"

/**
 * @file perm_word.h
 * @brief Defines `PermWord`.
 *
 * @author Timo Nicolai
 */

namespace cgtl
{

class PermWord;

inline std::size_t hash_value(Perm const &perm)
{
  return std::hash<cgtl::Perm>()(perm);
}

} // namespace cgtl

namespace std
{

template<>
struct hash<cgtl::PermWord>
{
  std::size_t operator()(cgtl::PermWord const &perm_word) const;
};

} // namespace std

namespace cgtl
{

class PermWord
{
friend std::size_t std::hash<PermWord>::operator()(PermWord const &permWord) const;

public:
  PermWord(Perm const &perm);
  PermWord(unsigned degree = 1) : PermWord(Perm(degree)) {};
  PermWord(std::vector<unsigned> const &perm) : PermWord(Perm(perm)) {};
  PermWord(unsigned n, std::vector<std::vector<unsigned>> const &cycles)
    : PermWord(Perm(n, cycles)) {};

  unsigned operator[](unsigned const i) const;
  PermWord operator~() const;
  bool operator==(PermWord const &rhs) const;
  bool operator!=(PermWord const &rhs) const;
  PermWord& operator*=(PermWord const &rhs);

  unsigned degree() const { return _n; }
  bool id() const;
  Perm perm() const;

private:
  unsigned _n;
  std::vector<Perm> _perms;
  std::vector<Perm> _invperms;
};

std::ostream& operator<<(std::ostream& stream, PermWord const &pw);
PermWord operator*(PermWord const &lhs, PermWord const &rhs);

} // namespace cgtl

#endif // _GUARD_PERM_WORD_H
