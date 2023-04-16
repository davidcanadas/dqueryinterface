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

#include "../dqueryinterface.h"

#include <cstdlib>
#include <memory>

struct DFooInterface { virtual auto Foo() noexcept -> void = 0; };
struct DBarInterface { virtual auto Bar() noexcept -> void = 0; };
struct DBazInterface { virtual auto Baz() noexcept -> void = 0; };

struct DExampleClass
    : DQueryInterface
    , DBarInterface, DBazInterface

{
    auto Bar() noexcept -> void override { printf("Run Bar() from DExampleClass.\n"); }
    auto Baz() noexcept -> void override { printf("Run Baz() from DExampleClass.\n"); }

private:
    auto QueryInterfaceByTypeId(const type_info& in_typeId) const noexcept -> const void* override
    {
        if (typeid(DBarInterface) == in_typeId) return static_cast<const DBarInterface*>(this);
        if (typeid(DBazInterface) == in_typeId) return static_cast<const DBazInterface*>(this);
        return nullptr;
    }
};

struct DOtherExampleClass
    : DQueryInterface
    , DFooInterface, DBarInterface, DBazInterface
{
    auto Foo() noexcept -> void override { printf("Run Foo() from DOtherExampleClass.\n"); }
    auto Bar() noexcept -> void override { printf("Run Bar() from DOtherExampleClass.\n"); }
    auto Baz() noexcept -> void override { printf("Run Baz() from DOtherExampleClass.\n"); }

private:
    auto QueryInterfaceByTypeId(const type_info& in_typeId) const noexcept -> const void* override
    {
        if (typeid(DFooInterface) == in_typeId) return static_cast<const DFooInterface*>(this);
        if (typeid(DBarInterface) == in_typeId) return static_cast<const DBarInterface*>(this);
        if (typeid(DBazInterface) == in_typeId) return static_cast<const DBazInterface*>(this);
        return nullptr;
    }
};

int main(int, char**)
{
    auto objectRegistry = DObjectRegistry();
    auto objectsImplementingFoo = objectRegistry.CreateInterfaceCollection<DFooInterface>();
    auto objectsImplementingBar = objectRegistry.CreateInterfaceCollection<DBarInterface>();
    auto objectsImplementingBaz = objectRegistry.CreateInterfaceCollection<DBazInterface>();

    std::shared_ptr<DExampleClass> obj1 = std::make_shared<DExampleClass>();
    std::shared_ptr<DExampleClass> obj2 = std::make_shared<DExampleClass>();
    std::shared_ptr<DOtherExampleClass> obj3 = std::make_shared<DOtherExampleClass>();
    std::shared_ptr<DOtherExampleClass> obj4 = std::make_shared<DOtherExampleClass>();

    objectRegistry.RequestAddObject(obj1);
    objectRegistry.RequestAddObject(obj2);
    objectRegistry.RequestAddObject(obj3);
    objectRegistry.RequestAddObject(obj4);

    assert(!obj1->HasInterface<DFooInterface>());
    assert( obj1->HasInterface<DBarInterface>());
    assert( obj1->HasInterface<DBazInterface>());
    assert(!obj2->HasInterface<DFooInterface>());
    assert( obj2->HasInterface<DBarInterface>());
    assert( obj2->HasInterface<DBazInterface>());
    assert( obj3->HasInterface<DFooInterface>());
    assert( obj3->HasInterface<DBarInterface>());
    assert( obj3->HasInterface<DBazInterface>());
    assert( obj4->HasInterface<DFooInterface>());
    assert( obj4->HasInterface<DBarInterface>());
    assert( obj4->HasInterface<DBazInterface>());

    assert(!obj1->QueryInterface<DFooInterface>());
    assert( obj1->QueryInterface<DBarInterface>());
    assert( obj1->QueryInterface<DBazInterface>());
    assert(!obj2->QueryInterface<DFooInterface>());
    assert( obj2->QueryInterface<DBarInterface>());
    assert( obj2->QueryInterface<DBazInterface>());
    assert( obj3->QueryInterface<DFooInterface>());
    assert( obj3->QueryInterface<DBarInterface>());
    assert( obj3->QueryInterface<DBazInterface>());
    assert( obj4->QueryInterface<DFooInterface>());
    assert( obj4->QueryInterface<DBarInterface>());
    assert( obj4->QueryInterface<DBazInterface>());


    printf("--- TEST INTERFACE ACCESS ---\n");
    objectsImplementingFoo.ForEach([](DFooInterface& in_interface) noexcept -> DQueryInterface::EPredicateResult { in_interface.Foo(); return DQueryInterface::EPredicateResult::Ok; });
    objectsImplementingBar.ForEach([](DBarInterface& in_interface) noexcept -> DQueryInterface::EPredicateResult { in_interface.Bar(); return DQueryInterface::EPredicateResult::Ok; });
    objectsImplementingBaz.ForEach([](DBazInterface& in_interface) noexcept -> DQueryInterface::EPredicateResult { in_interface.Baz(); return DQueryInterface::EPredicateResult::Ok; });
    printf("\n");

    printf("--- TEST OBJECT ACCESS ------\n");
    objectsImplementingFoo.ForEach([](const std::shared_ptr<DQueryInterface>& in_object) noexcept -> DQueryInterface::EPredicateResult 
    { 
        in_object->QueryInterface<DFooInterface>()->Foo();
        return DQueryInterface::EPredicateResult::Ok; 
    });
    objectsImplementingBar.ForEach([](const std::shared_ptr<DQueryInterface>& in_object) noexcept -> DQueryInterface::EPredicateResult 
    { 
        in_object->QueryInterface<DBarInterface>()->Bar(); return DQueryInterface::EPredicateResult::Ok; 
    });
    objectsImplementingBaz.ForEach([](const std::shared_ptr<DQueryInterface>& in_object) noexcept -> DQueryInterface::EPredicateResult 
    { 
        in_object->QueryInterface<DBazInterface>()->Baz(); return DQueryInterface::EPredicateResult::Ok; 
    });
    printf("\n");

    return EXIT_SUCCESS;
}
