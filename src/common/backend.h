
#ifndef BACKEND_H
#define BACKEND_H

#include "backendiface.h" // UsesBackendCtor*
#include "withexplicitinit.h" // WithExplicitInit

#include <QtCore/QScopedPointer>

#include <cassert> // assert
#include <functional>  // function
#include <memory> // unique_ptr, make_unique
#include <type_traits> // is_{same,base_of}, enable_if, {add_lvalue,remove}_reference, {false,true}_type
#include <utility> // forward, declval


/**
 * Parametrised types @c UsesBackend and @c WithBackend ship with a reusable
 * solution that enables testable private part of the Qt D-Pointer architecture
 * (a variation of the PIMPL idiom).
 *
 * The public part of the PIMPL (the one that used to store @c d_ptr and to use
 * @c Q_D macro) derives publicly from @c UsesBackend, while the private part
 * (used to store @c q_ptr to the public part of the PIMPL, and to use @c Q_Q
 * macro) derives from @c WithBackend. The "with" part manages (or references
 * if created externally and passed to it) the "backend". The "backend" is an
 * object of an instance of @c BackendBase parametrised type. If "backend" is
 * created externally and passed through reference to one of the @c UsesBackend
 * constructors, its lifetime is not a subject of the private part. Otherwise,
 * private part allocates required memory and forwards (if specified) arguments
 * from "uses", through "with", to the "backend"'s constructor. The type of the
 * "backend" to be created is selected via @c BackendSelectorTag that carries
 * the "backend" type. If no tag specified, "backend" of type instantiated from
 * @c BackendDefaultBase is created. The two required "backend" type generators,
 * i.e. @c BackendBase and @c BackendDefaultBase are templates parametrised by
 * @c Subject which is a tag used to distinguish the "backend" types.
 *
 * NOTE: There must be an specialisation of @c BackendBase<Subject> and of
 *       @c BackendDefaultBase<Subject> available for every used @c Subject.
 *
 * Type that derives from "uses" or "with" template must bring their constructors
 * in its public scope through @c using declaration (i.e. inherit constructors),
 * and put as a last member macro @c EXPLICIT_INIT to handle the class init.
 *
 * NOTE: Types derived from @c UsesBackend and @c WithBackend MUST use so called
 *       member-initializer to initialise their members directly in the class's
 *       body. That's due to fact, no direct access to the constructor of the
 *       derived type is possible. Any other initialisation code can be injected
 *       through "uses" and "with" constructors.
 *
 * Constructors in "uses" types use tag dispatch, i.e. constructor is selected
 * through its first argument that is one of the @c UsesBackendCtor_* types.
 * Constraints are executed against passed arguments during compile-time.
 *
 * Constructor in "with" types are executed by "uses" constructors, there shall
 * be no need to run them explicitly in the user code.
 *
 * Following constructors are inherited from the @c UsesBackend instance:
 * - Default   : creates "backend" of type @c BackendDefaultBase<Subject>.
 * - Non-owning: @b references "backend" of type (or of type derived from)
 *               @c BackendBase<Subject>.
 * - ActionQ   : creates "backend" of type @c BackendDefaultBase<Subject> and
 *               forwards passed arguments to it, and takes and action to init
 *               the "public part" of the PIMPL (i.e. class derived from "uses").
 * - ActionD   : same as ActionQ, but the passed action initialises the
 *               "private part" of the PIMPL.
 * - Selector  : takes a selector object that is of type which is an instance
 *               of @c BackendSelectorTag object, and arguments to be passed
 *               to the "backend" of type carried by the selector that will be
 *               created internally.
 * - Actions   : creates "backend" as Default ctor does, and takes actions
 *               and arguments as ActionQ and ActionD do.
 * - Init-only : @b references passed "backend" and executes passed action
 *               in the "public part" of the PIMPL.
 * - Explicit  : takes actions as ActionQ and ActionD ctors, and args and
 *               a selector as Selector ctor.
 * - Args      : creates "backend" of type as Default and forwards passed
 *               arguments to i.
 *
 * Following public member functions are exposed:
 * - UsesBackend::impl()    -- returns reference to the PIMPL's "private part".
 * - WithBackend::backend() -- returns reference to the associated "backend".
 *
 * @see UsesUIBackend and WithUIBackend for examples of "uses" and "with" instances.
 * @see UIBackendSelectorTag and UIBackendSelector for example selector utility.
 * @see UIBackend and UIBackendDefault for example required "backend" generators.
 *
 * @{
 */
template<template<class> class BackendSelectorTag>
struct BackendTraits
{
#if __cplusplus <= 201402L  // not in C++17?
    template<class... Ts> struct make_void { using type = void; };
    template<class... Ts> using void_t = typename make_void<Ts...>::type;

    // works for signle arg only -- param packs must be at the end of params
    template<class F, class A, class = void_t<>>
    struct is_invocable : std::false_type {};

    template<class F, class A>
    struct is_invocable<F, A, void_t<
            decltype(std::declval<F>()(std::declval<A>()))
        >> : std::true_type {};
#else
 #define void_t       std::void_t
 #define is_invocable std::is_invocable
#endif

    template<class T> struct is_selector : std::false_type {};
    template<class T> struct is_selector<BackendSelectorTag<T>> : std::true_type {};

    template<class A, class F>
    struct is_init
      : std::conditional_t<
            is_selector<F>::value
          , std::false_type
          , std::conditional_t<
                is_invocable<F, std::add_lvalue_reference_t<A>>::value
              , std::true_type
              , std::false_type
              >
          >
    {};
};



template<class, class, class, template<class> class, template<class> class, class>
class WithBackend;



template<
    class Derived
  , class PrivateWithBackend  // d_ptr's element_type
  , class Subject  // the tag passed to Backend*Base
  , template<class> class BackendBase
  , template<class> class BackendSelectorTag
  , template<class> class BackendDefaultBase
  , class Traits = BackendTraits<BackendSelectorTag>
  >
class UsesBackend
  : public WithExplicitInit<Derived>
{

 public:

    template<class A, class F>
    static constexpr bool is_init_v =
            Traits::template is_init< std::remove_reference_t<A>
                                    , std::remove_reference_t<F> >::value;

    template<class T>
    static constexpr bool is_selector_v =
             Traits::template is_selector<std::remove_reference_t<T>>::value;



    /** Acceses d_ptr from the actions passed to constructors. @{ */
    QScopedPointer<PrivateWithBackend>&       impl()       { return d_ptr; }
    const QScopedPointer<PrivateWithBackend>& impl() const { return d_ptr; }
    /** @} */



    /** Creates and manages lifetime of the default backend. */
    UsesBackend()
      :
        UsesBackend{ UsesBackendCtor_Explicit
                   , [](Derived&){}
                   , [](PrivateWithBackend&){}
                   , makeSelector<BackendDefaultBase<Subject>>() }
    {}

    /** Just references the backend object. */
    explicit UsesBackend(BackendBase<Subject>& backend)
      :
        UsesBackend{ [](Derived&){}, backend }
    {}

    /**
     * Creates and manages backend object of default backend type and runs the passed
     * action of siganture convertible to void(Derived&) type in this constructor's body,
     * or in the d_ptr constructor's body if @c T is convertible to void(PrivateWithBackend&)
     * type.
     *
     * @{
     */
    template<
        class T
      , class... As
      , class = std::enable_if_t<is_init_v<Derived, T>>
      >
    UsesBackend(const UsesBackendCtorTag_ActionQ&
//                                       ^^^^^^^ runs "in" the q_ptr ctor body
              , T&& t, As&&... args)
      :
        UsesBackend{ UsesBackendCtor_Explicit
                   , std::forward<T>(t)
                   , [](PrivateWithBackend&){}
                   , makeSelector<BackendDefaultBase<Subject>>()
                   , std::forward<As>(args)... }
    {}

    template<
        class T
      , class... As
      , class = std::enable_if_t<is_init_v<PrivateWithBackend, T>>
      >
    UsesBackend(const UsesBackendCtorTag_ActionD&
//                                       ^^^^^^^ runs "in" the d_ptr ctor body
              , T&& t, As&&... args)
      :
        UsesBackend{ UsesBackendCtor_Explicit
                   , [](Derived&){}
                   , std::forward<T>(t)
                   , makeSelector<BackendDefaultBase<Subject>>()
                   , std::forward<As>(args)... }
    {}

    /** @} */

    /**
     * Creates and manages backend object of given selected backend passed as @c T.
     */
    template<
        class T
      , class... As
      , class = std::enable_if_t<is_selector_v<T>>
      >
    UsesBackend(const UsesBackendCtorTag_Selector&
              , T&& t, As&&... args)
      :
        UsesBackend{ UsesBackendCtor_Explicit
                   , [](Derived&){}
                   , [](PrivateWithBackend&){}
                   , std::forward<T>(t)
                   , std::forward<As>(args)... }
    {}

    /**
     * Takes a user-specified action @c F of signature void(Derived&) that
     * is executed in the body  of the this constructor as a first argument.
     * As a second argument, takes an action of a signature
     * void(PrivateWithBackend&) to be executed in the d_ptr's constructor
     * body. Manages the memory of the selected backend.
     */
    template<
        class F
      , class G
      , class... As
      , class = std::enable_if_t< is_init_v<Derived, F>
                               && is_init_v<PrivateWithBackend, G> >
      >
    UsesBackend(const UsesBackendCtorTag_Actions&
              , F&& init, G&& initMember, As&&... args)
      :
        UsesBackend{ UsesBackendCtor_Explicit
                   , std::forward<F>(init)
                   , std::forward<G>(initMember)
                   , makeSelector<BackendDefaultBase<Subject>>()
                   , std::forward<As>(args)... }
    {}

    /**
     * Just references the backend object and executes passed action
     * of signature convertible to void(Derived&) in this constructor's body.
     */
    template<
        class F
      , class = std::enable_if_t<is_init_v<Derived, F>>
      >
    UsesBackend(F&& init, BackendBase<Subject>& backend)
      :
        WithExplicitInit<Derived>{std::forward<F>(init)}
      , d_ptr{new PrivateWithBackend{ * static_cast<Derived*>(this), backend}}
    {}

    /**
     * Constructs @c d_ptr with @c initMember action (convertible to
     * void(PrivateWithBackend&) type) passed to its constructor, executes
     * @c F action of type convertible to void(Derived&) in this constructor's
     * body.
     */
    template<
        class F
      , class G
      , class ImplSelector
      , class... As
      , class = std::enable_if_t< is_init_v<Derived, F>
                               && is_init_v<PrivateWithBackend, G>
                               && is_selector_v<ImplSelector> >
      >
    UsesBackend(const UsesBackendCtorTag_Explicit&
              , F&& init, G&& initMember, const ImplSelector& selector, As&&... args)
      :
        WithExplicitInit<Derived>{std::forward<F>(init)}
      , d_ptr{new PrivateWithBackend{ std::forward<G>(initMember)
                                    , selector
                                    , * static_cast<Derived*>(this)
                                    , std::forward<As>(args)... }}
    {}

    /** Creates an manages backend object of the default type. */
    template<class... As>
    explicit UsesBackend(const UsesBackendCtorTag_Args&
                       , As&&... args)
      :
        UsesBackend{ UsesBackendCtor_Explicit
                   , [](Derived&){}
                   , [](PrivateWithBackend&){}
                   , makeSelector<BackendDefaultBase<Subject>>()
                   , std::forward<As>(args)... }
    {}


 protected:

    QScopedPointer<PrivateWithBackend> d_ptr;


    static_assert(std::is_base_of<WithBackend<
                                        PrivateWithBackend
                                      , Derived
                                      , Subject
                                      , BackendBase
                                      , BackendSelectorTag
                                      , Traits
                                      >
                                , PrivateWithBackend>::value
                , "PrivateWithBackend must be derived from WithBackend");

 private:

    /** @see uibackendiface.h */
    template<class T>
    constexpr auto makeSelector() const
    {
        return BackendSelectorTag<T>{};
    }
};




template<
    class Derived
  , class BackendUser // WithBackend<...> instance
  , class Subject  // the tag passed to BackendBase
  , template<class> class BackendBase  // base class for backends
  , template<class> class BackendSelectorTag
  , class Traits = BackendTraits<BackendSelectorTag>
  >
class WithBackend
  : public WithExplicitInit<Derived>
{

 public:

    template<class A, class F>
    static constexpr bool is_init_v =
            Traits::template is_init< std::remove_reference_t<A>
                                    , std::remove_reference_t<F> >::value;

    template<class T>
    static constexpr bool is_selector_v =
            Traits::template is_selector<std::remove_reference_t<T>>::value;



    template<
        class ImplSelector
      , class... As
      , class = std::enable_if_t<is_selector_v<ImplSelector>>
      >
    WithBackend(const ImplSelector& selector, BackendUser& user, As&&... args)
      : WithBackend{ [](Derived&){}
                     , selector
                     , user
                     , std::forward<As>(args)... }
    {}

    WithBackend(BackendUser& user, BackendBase<Subject>& backend)
      : WithBackend{[](Derived&){}, user, backend}
    {}



    /**
     * Creates an object with implementation of @c BackendBase<Subject>
     * of type @c Impl is specified, otherwise default (must exist)
     * implementation of type @c BackendDefaultBase<Subject> is created.
     * Lifetime of the object is managed by object of this class.
     * Arguments passed to this constructor are forwarded to the Impl
     * constructor.
     */
    template<
        class F
      , class ImplSelector
      , class... As
      , class = std::enable_if_t<is_init_v<Derived, F> && is_selector_v<ImplSelector>>
      >
    WithBackend(F&& init, const ImplSelector&, BackendUser& user, As&&... args)
      :
        WithExplicitInit<Derived>{std::forward<F>(init)}
      , rep{std::make_unique<
                    typename std::remove_reference_t<ImplSelector>::type
                  >(std::forward<As>(args)...)}
      , handle{rep.get()}
      , q_ptr{&user}
    {
        static_assert(std::is_base_of< BackendBase<Subject>
                                     , typename std::remove_reference_t<ImplSelector>::type
                                     >::value
                    , "Impl does not match interface");
    }

    /** DOES NOT manage lifetime of @c backend variable! */
    template<
        class F
      , class = std::enable_if_t<is_init_v<Derived, F>>
      >
    WithBackend(F&& init, BackendUser& user, BackendBase<Subject>& backend)
      :
        WithExplicitInit<Derived>{std::forward<F>(init)}
      , handle{&backend}
      , q_ptr{&user}
    {}



    BackendBase<Subject>& backend()
    {
        assert(nullptr != handle);

        return *handle;
    }

    const BackendBase<Subject>& backend() const
    {
        assert(nullptr != handle);

        return *handle;
    }

 protected:

    /** @{ If backend is passed explicitly in constructor, handle stores
     *     pointer to it, and backend memory is not managed by rep.
     *     Otherwise, rep manages memory.
     *
     *     NEVER perform delete on handle.
     *     DO NOT reorder rep and handle.
     *
     * */
    std::unique_ptr<BackendBase<Subject>> rep;    /**< NOTE: stores BackendBase<Subject> subclass */
    BackendBase<Subject>*                 handle; /**< rep observer. */
    /** @} */

    BackendUser* const q_ptr; /**< Respective *Public type for the *Private one. */
};
/** @} */

#endif

