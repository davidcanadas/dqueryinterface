/*
 * MIT License
 * 
 * Copyright (c) 2023 David Cañadas Mazo.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN 
 * THE SOFTWARE.
 */

#pragma once

#include <cassert>
#include <functional>
#include <memory>
#include <mutex>
#include <tuple>
#include <typeinfo>
#include <vector>

struct DQueryInterface
{
    enum class EPredicateResult : uint8_t { Ok = 0, CancellationRequested };
    virtual auto QueryInterfaceByTypeId(const std::type_info& in_typeId)  const noexcept -> const void* = 0;

    // Template access.
    template<typename T> auto QueryInterface() const noexcept -> const T* { return reinterpret_cast<const T*>(QueryInterfaceByTypeId(typeid(T))); }
    template<typename T> auto QueryInterface() noexcept       -> T*       { return const_cast<T*>(static_cast<const DQueryInterface&>(*this).QueryInterface<T>()); }
    template<typename T> auto HasInterface  () const noexcept -> bool     { return QueryInterface<T>() != nullptr; }

    // COM-like access.
    auto QueryInterface(const std::type_info& in_typeId, const void** out_interface) const noexcept -> bool { return (*out_interface = QueryInterfaceByTypeId(in_typeId)); }
    auto QueryInterface(const std::type_info& in_typeId,       void** out_interface)       noexcept -> bool { return (*out_interface = const_cast<void*>(QueryInterfaceByTypeId(in_typeId))); }

    // Runtime  access.
    auto QueryInterface(const std::type_info& in_typeId) const noexcept -> const void* { return QueryInterfaceByTypeId(in_typeId); }
    auto QueryInterface(const std::type_info& in_typeId)       noexcept -> void*       { return const_cast<void*>(QueryInterfaceByTypeId(in_typeId)); }
    auto HasInterface  (const std::type_info& in_typeId) const noexcept -> bool        { return QueryInterfaceByTypeId(in_typeId) != nullptr; }
};

template<typename TMUTEXTYPE = std::mutex>
struct DObjectRegistry final
{
    DObjectRegistry() = default;
   ~DObjectRegistry() = default;

    template<typename TINTERFACE> struct DInterfaceCollection;
    template<typename TINTERFACE> auto CreateInterfaceCollection()    noexcept -> DInterfaceCollection<TINTERFACE> { return DInterfaceCollection<TINTERFACE>(*this); }
    auto RequestAddObject(std::shared_ptr<DQueryInterface> in_object) noexcept -> void
    {
        assert(in_object); 
        auto&& _ = std::scoped_lock(m_objectsToAddLock); 
        m_objectsToAdd.push_back(in_object); 
    }

    auto RequestRemoveObject(std::shared_ptr<DQueryInterface> in_object, std::function<auto (std::shared_ptr<DQueryInterface>&) -> DQueryInterface::EPredicateResult> in_optProcessRemovalPredicateFn) noexcept -> void
    {
        assert(in_object);
        if (in_optProcessRemovalPredicateFn && (in_optProcessRemovalPredicateFn(in_object) == DQueryInterface::EPredicateResult::CancellationRequested))
            return;
        auto&& _ = std::scoped_lock(m_objectsToRemoveLock);
        m_objectsToRemove.push_back(in_object);
    }

    auto ForEach(std::function<auto (const std::shared_ptr<DQueryInterface>&) -> DQueryInterface::EPredicateResult> in_predicateFn) noexcept -> void
    {
        assert(in_predicateFn);
        auto&& _ = std::scoped_lock(m_objectsLock);
        if (m_objectsToAdd.size() || m_objectsToRemove.size())
        {
            bool changed = false;
            {// Process pending additions.
                auto&& __ = std::scoped_lock(m_objectsToAddLock);
                for (auto& it : m_objectsToAdd)
                    if (std::find(m_objects.begin(), m_objects.end(), it) == m_objects.end())
                    {
                        m_objects.push_back(std::move(it));
                        changed = true;
                    }
                m_objectsToAdd.clear();
            }
            {// Process pending removals.
                auto&& __ = std::scoped_lock(m_objectsToRemoveLock);
                for (auto& it : m_objectsToRemove)
                {
                    auto foundObject  = std::find(m_objects.begin(), m_objects.end(), it);
                    if ( foundObject != m_objects.end() )
                    {// Remove (order is not kept).
                        *foundObject = m_objects.back();
                        m_objects.pop_back();
                        changed = true;
                    }
                }
                m_objectsToRemove.clear();
            }
            if (changed)
                ++m_generationId;
        }
        for (auto& it : m_objects)
            if (in_predicateFn(it) == DQueryInterface::EPredicateResult::CancellationRequested)
                break;
    }

private:
    template<typename TINTERFACE>
    struct DInterfaceCollection final
    {
        DInterfaceCollection() = delete;
       ~DInterfaceCollection() = default;

        auto ForEach(std::function<auto (const std::shared_ptr<DQueryInterface>&) -> DQueryInterface::EPredicateResult> in_predicateFn) noexcept -> void
        {
            assert(in_predicateFn);
            auto&& _ = std::scoped_lock(m_objectsLock);
            if (m_generationId != m_registry.GetGenerationId())
            {
                m_objects .clear();
                m_registry.ForEach([this](const std::shared_ptr<DQueryInterface>& in_object) -> DQueryInterface::EPredicateResult
                {
                    if (in_object->HasInterface<TINTERFACE>())
                        m_objects.push_back(in_object);
                    return DQueryInterface::EPredicateResult::Ok;
                });
                m_generationId  = m_registry.GetGenerationId();
            }
            for (auto& it : m_objects)
                if (in_predicateFn(it) == DQueryInterface::EPredicateResult::CancellationRequested)
                    break;
        }

        auto ForEach(std::function<auto (TINTERFACE&) -> DQueryInterface::EPredicateResult> in_predicateFn) noexcept -> void
        {
            assert(in_predicateFn);
            ForEach([in_predicateFn](const std::shared_ptr<DQueryInterface>& in_object) -> DQueryInterface::EPredicateResult 
            { 
                return in_predicateFn(*in_object->QueryInterface<TINTERFACE>()); 
            });
        }

    private:
        friend struct DObjectRegistry;

        std::vector<std::shared_ptr<DQueryInterface>> m_objects;
        TMUTEXTYPE      m_objectsLock;
        unsigned int    m_generationId = UINT_MAX;
        struct DObjectRegistry<TMUTEXTYPE>& m_registry;
        DInterfaceCollection    (struct DObjectRegistry<TMUTEXTYPE>& in_registry) : m_registry(in_registry) { ; }
        DInterfaceCollection    (const DInterfaceCollection&)           = delete;
        DInterfaceCollection    (DInterfaceCollection&&)                = delete;
        DInterfaceCollection&   operator=(const DInterfaceCollection&)  = delete;
        auto GetGenerationId()  const noexcept -> unsigned int { return m_generationId; }
    };

    std::vector<std::shared_ptr<DQueryInterface>> m_objects, m_objectsToAdd, m_objectsToRemove;
    TMUTEXTYPE      m_objectsLock , m_objectsToAddLock, m_objectsToRemoveLock;
    unsigned int    m_generationId = 0;
    DObjectRegistry (const DObjectRegistry&)          = delete;
    DObjectRegistry (DObjectRegistry&&)               = delete;
    DObjectRegistry&operator=(const DObjectRegistry&) = delete;
    auto GetGenerationId() const noexcept -> unsigned int { return m_generationId; }
};
