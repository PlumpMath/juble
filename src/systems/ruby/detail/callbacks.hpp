#pragma once

#include <functional>

#include "conversions.hpp"

namespace script
{
  namespace ruby_detail
  {
    /* TODO: Don't use these. Use a callback type templated on some format or something. */
    /* value_type func(value_type const self, value_type const arg...) */
    inline int callback_variadic(int const n)
    { return n; }
    /* value_type func(int const argc, value_type * const argv, value_type const self) */
    int constexpr const callback_argc{ -1 };
    /* value_type func(value_type const self, value_type const args_arr) */
    int constexpr const callback_array{ -2 };

    template <typename Class, typename F>
    class mem_func_wrapper;
    template <typename Class, typename R, typename... Args>
    class mem_func_wrapper<Class, R (Args...)> final
    {
      public:
        mem_func_wrapper() = delete;
        mem_func_wrapper(std::function<R (Class&, Args...)> const &func)
        { func_ = func; }

        /* TODO: All of these call functions are the same; refactor. */
        static value_type call(int const argc, value_type * const argv, value_type const self)
        {
          juble_assert(func_, "invalid function call");
          juble_assert(sizeof...(Args) == argc, "invalid argument count");

          using uptr_t = std::unique_ptr<detail::bare_t<Class>>;
          uptr_t * data{};
          Data_Get_Struct(self, uptr_t, data);
          juble_assert(data && data->get(), "invalid object data");

          return call_impl(**data, std::index_sequence_for<Args...>{}, argv);
        }

      private:
        template <size_t... Ns>
        static value_type call_impl(Class &c, std::index_sequence<Ns...> const,
                                value_type * const argv)
        {
          (void)argv; /* XXX: When Ns is 0, argv is not used. */
          return build_return_value<R>(func_, c, from_ruby<Args>(argv[Ns])...);
        }

        static std::function<R (Class&, Args...)> func_;
    };
    template <typename Class, typename R, typename... Args>
    class mem_func_wrapper<Class, R (Args...) const> final
    {
      public:
        mem_func_wrapper() = delete;
        mem_func_wrapper(std::function<R (Class&, Args...)> const &func)
        { func_ = func; }

        static value_type call(int const argc, value_type * const argv, value_type const self)
        {
          juble_assert(func_, "invalid function call");
          juble_assert(sizeof...(Args) == argc, "invalid argument count");

          using uptr_t = std::unique_ptr<detail::bare_t<Class>>;
          uptr_t * data{};
          Data_Get_Struct(self, uptr_t, data);
          juble_assert(data && data->get(), "invalid object data");

          return call_impl(**data, std::index_sequence_for<Args...>{}, argv);
        }

      private:
        template <size_t... Ns>
        static value_type call_impl(Class &c, std::index_sequence<Ns...> const,
                                value_type * const argv)
        {
          (void)argv; /* XXX: When Ns is 0, argv is not used. */
          return build_return_value<R>(func_, c, from_ruby<Args>(argv[Ns])...);
        }

        static std::function<R (Class&, Args...)> func_;
    };
    template <typename Class, typename R, typename... Args>
    std::function<R (Class&, Args...)> mem_func_wrapper<Class, R (Args...)>::func_;
    template <typename Class, typename R, typename... Args>
    std::function<R (Class&, Args...)> mem_func_wrapper<Class, R (Args...) const>::func_;

    template <typename F>
    class ctor_wrapper;
    template <typename Class, typename... Args>
    class ctor_wrapper<Class (Args...)> final
    {
      public:
        ctor_wrapper() = delete;
        ctor_wrapper(std::function<Class* (Args...)> const &func)
        { func_ = func; }

        static value_type call(int const argc, value_type * const argv, value_type const self)
        {
          using uptr_t = std::unique_ptr<detail::bare_t<Class>>;

          juble_assert(func_, "invalid ctor call");
          juble_assert(sizeof...(Args) == argc, "invalid argument count");

          auto * const data(new uptr_t
          { call_impl(std::index_sequence_for<Args...>{}, argv) });

          auto const deleter([](void * const data)
          { delete static_cast<uptr_t*>(data); });

          value_type const obj{ Data_Wrap_Struct(self, nullptr, deleter, data) };
          return obj;
        }

      private:
        template <size_t... Ns>
        static Class* call_impl(std::index_sequence<Ns...> const,
                                value_type * const argv)
        {
          (void)argv; /* XXX: When Ns is 0, argv is not used. */
          return func_(from_ruby<Args>(argv[Ns])...);
        }

        static std::function<Class* (Args...)> func_;
    };
    template <typename Class, typename... Args>
    std::function<Class* (Args...)> ctor_wrapper<Class (Args...)>::func_;
  }
}
