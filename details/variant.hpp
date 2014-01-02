#pragma once

// Intended to be included by variant.hpp.  Don't include externally.

#include <cassert>
#include <new>

#include <boost/mpl/or.hpp>

namespace detail {

#include "data.inc"
#include "utility.inc"
#include "runtime_helpers.inc"

}
