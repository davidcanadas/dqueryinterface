# dqueryinterface
The fast-path to get the query-interface pattern into your C++ program.

## Features

* One-header, works out-of-the-box.
* Multi-thread-ready.
* No operating system headers needed.
* Includes extras to operate your interfaces.

## Using **dqueryinterface** in your programs

```c++
#include "dqueryinterface.h"
#include <cstdlib>
#include <memory>

// Declare your interfaces.
struct DFooInterface { virtual auto Foo() noexcept -> void = 0; };
struct DBarInterface { virtual auto Bar() noexcept -> void = 0; };
struct DBazInterface { virtual auto Bar() noexcept -> void = 0; };

// Declare objects deriving from DQueryInterface and implementing your interfaces of choice.
struct DExampleClass
    : DQueryInterface
    , DFooInterface, DBarInterface

{
    auto Foo() noexcept -> void override { printf("Run Foo().\n"); }
    auto Bar() noexcept -> void override { printf("Run Bar().\n"); }

private:
    // Implement the QueryInterfaceByTypeId() method required by DQueryInterface.
    auto QueryInterfaceByTypeId(const type_info& in_typeId) const noexcept -> const void* override
    {
        if (typeid(DFooInterface) == in_typeId) return static_cast<const DFooInterface*>(this);
        if (typeid(DBarInterface) == in_typeId) return static_cast<const DBarInterface*>(this);
        return nullptr;
    }
};

int main(int, char**)
{
    std::shared_ptr<DQueryInterface> obj1 = std::make_shared<DExampleClass>();

    // Check for specific implementations.
    assert( obj1->HasInterface<DFooInterface>());
    assert( obj1->HasInterface<DBarInterface>());
    assert(!obj1->HasInterface<DBazInterface>());

    // Call any implemented interface.
    obj1->QueryInterface<DFooInterface>()->Foo();
}
```

## Advanced usage

### Iterating objects implementing specific interfaces

```c++
#include "dqueryinterface.h"
#include <cstdlib>
#include <memory>

// Declare your interfaces.
struct DFooInterface { virtual auto Foo() noexcept -> void = 0; };
struct DBarInterface { virtual auto Bar() noexcept -> void = 0; };

// Declare objects deriving from DQueryInterface and implementing your interfaces of choice.
struct DExampleClass
    : DQueryInterface
    , DFooInterface, DBarInterface

{
    auto Foo() noexcept -> void override { printf("Run Foo() on 0x%p.\n", this); }
    auto Bar() noexcept -> void override { printf("Run Bar() on 0x%p.\n", this); }

private:
    // Implement the QueryInterfaceByTypeId() method required by DQueryInterface.
    auto QueryInterfaceByTypeId(const type_info& in_typeId) const noexcept -> const void* override
    {
        if (typeid(DFooInterface) == in_typeId) return static_cast<const DFooInterface*>(this);
        if (typeid(DBarInterface) == in_typeId) return static_cast<const DBarInterface*>(this);
        return nullptr;
    }
};

int main(int, char**)
{
    // Create your objects as shared pointers.
    std::shared_ptr<DQueryInterface> obj1 = std::make_shared<DExampleClass>();
    std::shared_ptr<DQueryInterface> obj2 = std::make_shared<DExampleClass>();

    // Create the DObjectRegistry instance and registers your objects onto it.
    auto objectRegistry = DObjectRegistry();
    objectRegistry.RequestAddObject(obj1);
    objectRegistry.RequestAddObject(obj2);

    // Create as many interface collections as needed. They can be created on the
    // fly if deemed necessary, however they perform better when reused.
    auto fooInstances = objectRegistry.CreateInterfaceCollection<DFooInterface>();
    auto barInstances = objectRegistry.CreateInterfaceCollection<DBarInterface>();
    
    // Iterate your collections accessing the interface.
    fooInstances.ForEach([](DFooInterface& in_interface) 
    { 
        in_interface.Foo(); 
        return DQueryInterface::EPredicateResult::Ok; // Return "Ok" to continue iterating.
    });
    // Or, alternatively, iterate your collections retrieving the original objects instead.
    barInstances.ForEach([](const std::shared_ptr<DQueryInterface>& in_object) 
    { 
        in_object->QueryInterface<DBarInterface>()->Bar(); 
        return DQueryInterface::EPredicateResult::CancellationRequested; // Return "CancellationRequest" to stop iterating.
    });
}
```

### Mutexes

The `DObjectRegistry` class is actually a template accepting one type argument determining the type of mutex to use when accesing the registry and collections from multi-threaded environments. The default implementation uses `std::mutex`, however it is recommended to switch to a more lightweight exclusion system, like `SRWLock` in Windows. In any case, the type you provide should have an interface compatible with `std::scoped_lock`.

### Single-threaded model

Just provide a class implementing no mutex at all as type argument to `DObjectRegistry`.

### COM-like QueryInterface pattern

The example objects provided within this `README.md` file are directly deriving from the implemented interfaces. However, this makes this QueryInterface implementation incompatible with the `IUnknown::QueryInterface` implementation used by Microsoft COM objects.

In particular, objects implementing the Microsoft COM approach should be re-interfaceable to any other implemented interface in the original object. Thus, this code should be valid:

```c++
assert
(
    std::make_shared<DExampleClass>()
        ->QueryInterface<DFooInterface>()
            ->QueryInterface<DBarInterface>()
);
```

To accomplish this requirement, it is suggested to use composition/aggregation instead of inheritance. Using composition/aggregation, any provided interface can be, at the same time, implementing `DQueryInterface`. Holding a `std::shared_ptr<DQueryInterface>` member to the root object in the composition/aggregation hierarchy would suffice to make it work.

# Examples

An example solution for Visual Studio 2022 is provided under the folder `vs2022`.

---

*Licensed under the MIT license.*
*This library is under active development and can contain bugs and/or non-portable code. Tested on Microsoft Windows/Visual Studio 2022 (VC++ & Clang) only!*
