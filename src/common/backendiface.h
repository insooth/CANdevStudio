
#ifndef BACKENDIFACE_H
#define BACKENDIFACE_H

/** Tags usesd to select given UsesBackend constructor. @{ */
template<unsigned> struct UsesBackendCtorTag {};

using UsesBackendCtorTag_Explicit = UsesBackendCtorTag<111>;
using UsesBackendCtorTag_ActionQ  = UsesBackendCtorTag<222>;
using UsesBackendCtorTag_ActionD  = UsesBackendCtorTag<333>;
using UsesBackendCtorTag_Selector = UsesBackendCtorTag<444>;
using UsesBackendCtorTag_Actions  = UsesBackendCtorTag<556>;
using UsesBackendCtorTag_Args     = UsesBackendCtorTag<666>;  // fallback: fully variadic

constexpr UsesBackendCtorTag_Explicit UsesBackendCtor_Explicit{};
constexpr UsesBackendCtorTag_ActionQ  UsesBackendCtor_ActionQ{};  // runs "in" q_ptr ctor
constexpr UsesBackendCtorTag_ActionD  UsesBackendCtor_ActionD{};  // runs "in" d_ptr ctor
constexpr UsesBackendCtorTag_Selector UsesBackendCtor_Selector{};
constexpr UsesBackendCtorTag_Actions  UsesBackendCtor_Actions{};
constexpr UsesBackendCtorTag_Args     UsesBackendCtor_Args{};
/** @} */

#endif

