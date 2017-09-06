
#ifndef UIBACKEND_H
#define UIBACKEND_H

#include "backend.h" // {Uses,With}Backend
#include "uibackendiface.h" // UIBackend{,Default}, UsesUIBackendCtor*
#include "uibackendimpls.h" // UIBackend, UIBackendDefault specialisatons


/**
 * To be derived publicly by @c Derived type that is going to be composed
 * with the *Private type (as used in the Qt D-Pointer infrastructure).
 *
 * @warning Q_DECLARE_PRIVATE_D macro is used (note the suffix @b _D).
 *
 * NOTE: This base class is not intended to be used in polymorphic delete.
 *
 * @see https://wiki.qt.io/D-Pointer for @c d_ptr
 *
 * Example:
 *
 * @see WithUIBackend for CanRawViewPrivate example -- NOTE the constructor!
 *
 * @code
 *  class CanRawView
 *    : public QObject                  // *MUST* be specified as a first one
 *    , public UsesUIBackend<
 *                  CanRawView         // this type
 *                , CanRawViewPrivate  // type referenced by Q_DECLARE_PRIVATE_D()
 *                , CanRawView         // tag for UIBackend<>
 *                >
 *  {
 *      Q_OBJECT
 *      Q_DECLARE_PRIVATE_D(UsesBackend::d_ptr.data(), CanRawView)
 *  //                   ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 *
 *   public:
 *
 *      using UsesBackend::UsesBackend;  // brings ctors in scope
 *  //        ^~~ not Uses*UI*Backend
 *
 *      void foo()
 *      {
 *          Q_D(CanRawView);
 *
 *          d->bar(); // calls CanRawViewPrivate::bar
 *      }
 *
 *   private:
 *
 *      EXPLICIT_INIT(CanRawView)  // MUST be at the very end of the class!
 *  //  ^^^^^^^^^^^^^^^^^^^^^^^^^
 *  };
 *
 * @endcode
 */
template<class Derived, class PrivateWithUIBackend, class Subject = Derived>
using UsesUIBackend =
            UsesBackend<
                Derived
              , PrivateWithUIBackend
              , Subject
              , UIBackend
              , UIBackendSelectorTag
              , UIBackendDefault
              >;



/**
 * To be derived publicly by the *Private types (as used in the Qt D-Pointer
 * infrastructure). As in @c UsesUIBackend, there is a possiblity to pass
 * @c init function object of signature @c void(Derived&).
 *
 * NOTE: This base class is not intended to be used in polymorphic delete.
 *
 * @see https://wiki.qt.io/D-Pointer for @c q_ptr
 *
 * Example:
 *
 * @code
 *  class CanRawViewPrivate
 *    : public QObject                  // *MUST* be specified as a first one
 *    , public WithUIBackend<
 *                  CanRawViewPrivate  // this type
 *                , CanRawView         // type referenced by Q_DECLARE_PUBLIC()
 *                , CanRawView         // tag for UIBackend<>
 *                >
 *  {
 *      Q_OBJECT
 *      Q_DECLARE_PUBLIC(CanRawView)
 *
 *   public:
 *
 *      using WithBackend::WithBackend;  // brings ctors in scope
 *  //        ^~~ not With*UI*Backend
 *
 *
 *      void bar()
 *      {
 *          Q_Q(CanRawView);
 *
 *          q->foo();  // calls CanRawView::foo
 *
 *          backend().updateScroll();  // calls function from UIBackend<Subject> or from dervied
 *      }
 *
 *
 *   private:
 *
 *      EXPLICIT_INIT(CanRawViewPrivate)  // MUST be at the very end of the class!
 *  //  ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 *  };
 * @endcode
 */
template<class Derived, class UIBackendUser, class Subject = UIBackendUser>
using WithUIBackend =
            WithBackend<
                Derived
              , UIBackendUser
              , Subject
              , UIBackend
              , UIBackendSelectorTag
              >;

#endif

