﻿// WARNING: Please don't edit this file. It was generated by C++/WinRT v2.0.000000.0
#pragma once
#include "winrt/impl/Component.2.h"
#include "winrt/impl/Component.Fast.2.h"
#include "winrt/Component.h"
namespace winrt::impl
{
    template <typename D> hstring consume_Component_Fast_IFastClass<D>::First() const
    {
        void* result;
        check_hresult(WINRT_SHIM(Component::Fast::IFastClass)->First(&result));
        return { take_ownership_from_abi, result };
    }
    template <typename D> hstring consume_Component_Fast_IFastClass<D>::Second() const
    {
        void* result;
        check_hresult(WINRT_SHIM(Component::Fast::IFastClass)->Second(&result));
        return { take_ownership_from_abi, result };
    }
    template <typename D> hstring consume_Component_Fast_IFastClass2<D>::Third() const
    {
        void* result;
        check_hresult(WINRT_SHIM(Component::Fast::IFastClass2)->Third(&result));
        return { take_ownership_from_abi, result };
    }
    template <typename D> hstring consume_Component_Fast_IFastClass2<D>::Fourth() const
    {
        void* result;
        check_hresult(WINRT_SHIM(Component::Fast::IFastClass2)->Fourth(&result));
        return { take_ownership_from_abi, result };
    }
    template <typename D> hstring consume_Component_Fast_IFastClassStatics<D>::StaticMethod() const
    {
        void* result;
        check_hresult(WINRT_SHIM(Component::Fast::IFastClassStatics)->StaticMethod(&result));
        return { take_ownership_from_abi, result };
    }
    template <typename D> hstring consume_Component_Fast_ISlowClass<D>::First() const
    {
        void* result;
        check_hresult(WINRT_SHIM(Component::Fast::ISlowClass)->First(&result));
        return { take_ownership_from_abi, result };
    }
    template <typename D> hstring consume_Component_Fast_ISlowClass<D>::Second() const
    {
        void* result;
        check_hresult(WINRT_SHIM(Component::Fast::ISlowClass)->Second(&result));
        return { take_ownership_from_abi, result };
    }
    template <typename D> hstring consume_Component_Fast_ISlowClass2<D>::Third() const
    {
        void* result;
        check_hresult(WINRT_SHIM(Component::Fast::ISlowClass2)->Third(&result));
        return { take_ownership_from_abi, result };
    }
    template <typename D> hstring consume_Component_Fast_ISlowClass2<D>::Fourth() const
    {
        void* result;
        check_hresult(WINRT_SHIM(Component::Fast::ISlowClass2)->Fourth(&result));
        return { take_ownership_from_abi, result };
    }
    template <typename D> hstring consume_Component_Fast_ISlowClassStatics<D>::StaticMethod() const
    {
        void* result;
        check_hresult(WINRT_SHIM(Component::Fast::ISlowClassStatics)->StaticMethod(&result));
        return { take_ownership_from_abi, result };
    }
    template <typename D>
    struct produce<D, Component::Fast::IFastClass> : produce_base<D, Component::Fast::IFastClass>
    {
        int32_t WINRT_CALL First(void** result) noexcept final
        {
            try
            {
                *result = nullptr;
                typename D::abi_guard guard(this->shim());
                *result = detach_from<hstring>(this->shim().First());
                return 0;
            }
            catch (...) { return to_hresult(); }
        }
        int32_t WINRT_CALL Second(void** result) noexcept final
        {
            try
            {
                *result = nullptr;
                typename D::abi_guard guard(this->shim());
                *result = detach_from<hstring>(this->shim().Second());
                return 0;
            }
            catch (...) { return to_hresult(); }
        }
    };
    template <typename D>
    struct produce<D, Component::Fast::IFastClass2> : produce_base<D, Component::Fast::IFastClass2>
    {
        int32_t WINRT_CALL Third(void** result) noexcept final
        {
            try
            {
                *result = nullptr;
                typename D::abi_guard guard(this->shim());
                *result = detach_from<hstring>(this->shim().Third());
                return 0;
            }
            catch (...) { return to_hresult(); }
        }
        int32_t WINRT_CALL Fourth(void** result) noexcept final
        {
            try
            {
                *result = nullptr;
                typename D::abi_guard guard(this->shim());
                *result = detach_from<hstring>(this->shim().Fourth());
                return 0;
            }
            catch (...) { return to_hresult(); }
        }
    };
    template <typename D>
    struct produce<D, Component::Fast::IFastClassStatics> : produce_base<D, Component::Fast::IFastClassStatics>
    {
        int32_t WINRT_CALL StaticMethod(void** result) noexcept final
        {
            try
            {
                *result = nullptr;
                typename D::abi_guard guard(this->shim());
                *result = detach_from<hstring>(this->shim().StaticMethod());
                return 0;
            }
            catch (...) { return to_hresult(); }
        }
    };
    template <typename D>
    struct produce<D, Component::Fast::ISlowClass> : produce_base<D, Component::Fast::ISlowClass>
    {
        int32_t WINRT_CALL First(void** result) noexcept final
        {
            try
            {
                *result = nullptr;
                typename D::abi_guard guard(this->shim());
                *result = detach_from<hstring>(this->shim().First());
                return 0;
            }
            catch (...) { return to_hresult(); }
        }
        int32_t WINRT_CALL Second(void** result) noexcept final
        {
            try
            {
                *result = nullptr;
                typename D::abi_guard guard(this->shim());
                *result = detach_from<hstring>(this->shim().Second());
                return 0;
            }
            catch (...) { return to_hresult(); }
        }
    };
    template <typename D>
    struct produce<D, Component::Fast::ISlowClass2> : produce_base<D, Component::Fast::ISlowClass2>
    {
        int32_t WINRT_CALL Third(void** result) noexcept final
        {
            try
            {
                *result = nullptr;
                typename D::abi_guard guard(this->shim());
                *result = detach_from<hstring>(this->shim().Third());
                return 0;
            }
            catch (...) { return to_hresult(); }
        }
        int32_t WINRT_CALL Fourth(void** result) noexcept final
        {
            try
            {
                *result = nullptr;
                typename D::abi_guard guard(this->shim());
                *result = detach_from<hstring>(this->shim().Fourth());
                return 0;
            }
            catch (...) { return to_hresult(); }
        }
    };
    template <typename D>
    struct produce<D, Component::Fast::ISlowClassStatics> : produce_base<D, Component::Fast::ISlowClassStatics>
    {
        int32_t WINRT_CALL StaticMethod(void** result) noexcept final
        {
            try
            {
                *result = nullptr;
                typename D::abi_guard guard(this->shim());
                *result = detach_from<hstring>(this->shim().StaticMethod());
                return 0;
            }
            catch (...) { return to_hresult(); }
        }
    };
    template <typename D>
    struct produce<D, fast_interface<Component::Fast::FastClass>> : produce_base<D, fast_interface<Component::Fast::FastClass>>
    {
        int32_t WINRT_CALL First(void** result) noexcept final
        {
            try
            {
                *result = nullptr;
                typename D::abi_guard guard(this->shim());
                *result = detach_from<hstring>(this->shim().First());
                return 0;
            }
            catch (...) { return to_hresult(); }
        }
        int32_t WINRT_CALL Second(void** result) noexcept final
        {
            try
            {
                *result = nullptr;
                typename D::abi_guard guard(this->shim());
                *result = detach_from<hstring>(this->shim().Second());
                return 0;
            }
            catch (...) { return to_hresult(); }
        }
        int32_t WINRT_CALL Third(void** result) noexcept final
        {
            try
            {
                *result = nullptr;
                typename D::abi_guard guard(this->shim());
                *result = detach_from<hstring>(this->shim().Third());
                return 0;
            }
            catch (...) { return to_hresult(); }
        }
        int32_t WINRT_CALL Fourth(void** result) noexcept final
        {
            try
            {
                *result = nullptr;
                typename D::abi_guard guard(this->shim());
                *result = detach_from<hstring>(this->shim().Fourth());
                return 0;
            }
            catch (...) { return to_hresult(); }
        }
    };
}
namespace winrt::Component::Fast
{
    inline FastClass::FastClass() :
        FastClass(impl::call_factory<FastClass>([](auto&& f) { return f.template ActivateInstance<FastClass>(); }))
    {
    }
    inline hstring FastClass::StaticMethod()
    {
        return impl::call_factory<FastClass, Component::Fast::IFastClassStatics>([&](auto&& f) { return f.StaticMethod(); });
    }
    inline hstring FastClass::First() const
    {
        void* result;
        check_hresult((*(impl::abi_t<fast_interface<FastClass>>**)this)->First(&result));
        return { take_ownership_from_abi, result };
    }
    inline hstring FastClass::Second() const
    {
        void* result;
        check_hresult((*(impl::abi_t<fast_interface<FastClass>>**)this)->Second(&result));
        return { take_ownership_from_abi, result };
    }
    inline hstring FastClass::Third() const
    {
        void* result;
        check_hresult((*(impl::abi_t<fast_interface<FastClass>>**)this)->Third(&result));
        return { take_ownership_from_abi, result };
    }
    inline hstring FastClass::Fourth() const
    {
        void* result;
        check_hresult((*(impl::abi_t<fast_interface<FastClass>>**)this)->Fourth(&result));
        return { take_ownership_from_abi, result };
    }
    inline hstring SlowClass::StaticMethod()
    {
        return impl::call_factory<SlowClass, Component::Fast::ISlowClassStatics>([&](auto&& f) { return f.StaticMethod(); });
    }
    inline SlowClass::SlowClass() :
        SlowClass(impl::call_factory<SlowClass>([](auto&& f) { return f.template ActivateInstance<SlowClass>(); }))
    {
    }
}
namespace std
{
    template<> struct hash<winrt::Component::Fast::IFastClass> : winrt::impl::hash_base<winrt::Component::Fast::IFastClass> {};
    template<> struct hash<winrt::Component::Fast::IFastClass2> : winrt::impl::hash_base<winrt::Component::Fast::IFastClass2> {};
    template<> struct hash<winrt::Component::Fast::IFastClassStatics> : winrt::impl::hash_base<winrt::Component::Fast::IFastClassStatics> {};
    template<> struct hash<winrt::Component::Fast::ISlowClass> : winrt::impl::hash_base<winrt::Component::Fast::ISlowClass> {};
    template<> struct hash<winrt::Component::Fast::ISlowClass2> : winrt::impl::hash_base<winrt::Component::Fast::ISlowClass2> {};
    template<> struct hash<winrt::Component::Fast::ISlowClassStatics> : winrt::impl::hash_base<winrt::Component::Fast::ISlowClassStatics> {};
    template<> struct hash<winrt::Component::Fast::FastClass> : winrt::impl::hash_base<winrt::Component::Fast::FastClass> {};
    template<> struct hash<winrt::Component::Fast::SlowClass> : winrt::impl::hash_base<winrt::Component::Fast::SlowClass> {};
}
