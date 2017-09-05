
#ifndef UIBACKENDIFACE_H
#define UIBACKENDIFACE_H

#include "backendiface.h" // UsesBackendCtor*

/**
 * All the UI backends per given subject must derive from UIBackend<Subject> interface.
 * User must implement at least one derived UIBackend type per unique Subject, i.e.
 * UIBackendDefault. If it is not desirable to use virtual member functions (as it
 * could be used in the UIBackend<Subject>), then UIBackend<Subject> shall implement
 * all the non-virtual functions *and* UIBackendDefault shall be empty and derive from
 * it publicly and use constructor inheritance. That's due to fact, that the following
 * implementation uses a reference to the object of type UIBackend<Subject> everywhere.
 *
 * NOTE: The actual shape, number of ctors, args taken by ctors is not forced by this
 *       implementation. Instances of UIBackend<> and the derived types can take any
 *       number of arguments that can be forwarded passed through UsesUIBackend and
 *       WithUIBackend to the selected type of UI backend.
 *
 * @see UIBackend<CanRawView> and UIBackendDefault<CanRawView> (aka CanRawViewBackend).
 *
 * @{
 */
/** UIBackend for given Subject tag (can be any type). */
template<class Subject>
class UIBackend;

/** Default UIBackend for given Subject tag, must be derived from UIBackend<Subject>. */
template<class Subject>
class UIBackendDefault;/* : public UIBackend<Subject>
{
    static_assert(false, "UIBackendDefault not implemented");
};*/
/** @} */


/** Tag generator used to select an implementation type to be created. @{ */
template<class T>
struct UIBackendSelectorTag { using type = T; };

#if ! defined(_MSC_VER)  // error C3376: only static data member templates are allowed
template<class Impl>
constexpr UIBackendSelectorTag<Impl> UIBackendSelector{};
#endif
/** @}  */

#endif

