# EmexFoundation

## Introduction
EmexFoundation is CoreFoundation's FOSS cousin. It brings everything CoreFoundation gave to Linux, FreeBSD and macOS licensed under AGPLv3.

## Runtime
It's runtime called `EFRuntime` is what all EmexFoundation object's use to manage their life cycle. Every object has a object header which is the raw `EFObject` type. The runtime differentiates between objects and allocators using their first field `_rt` which is of type `EFRootType`, each type is differently treated by the runtime. For example it is used so `EFCopyDescription` works correctly for all root types. `EFAllocatorRef` is a referemce to an raw `EFAllocator` which is how objects manage memory, using three methods `EFAllocatorAllocate`, `EFAllocatorReallocate` and `EFAllocatorDeallocate`, but a object creation symbol doesn"t use those manually to create a object of it's type in question it uses another symbol named `EFObjectCreate` which takes in a `EFAllocatorRef`. Each object has as stated before a `EFRootType` at the very start of it's structure, but we shall talk about identification, object's are identified by their `typeID` field which is of type `EFTypeID`, that is because object's can't just exist, they need some root handling by the class. A class has handlers for initialization, deinitilization, equalization, hashing and copying it's description. The class also carries a name for `EFCopyDescription` to fallback to. Those handlers exist, because each object is reference counted, which allows for clean ownership semantics and why? Well a object can hold other object's for example, so on deinitilization it has to release each object it holds.

### ARC-like features in pure C
It's kind of a trick but we extended and improved the usual known CoreFoundation and it's nerve racking memory management with modern C tricks that didn't exist in the past, so good for us :3.

#### EFAUTOREL
It's a macro which uses a clanup handler which uses the safe `EFReleaseTry`(more on that later) so even if the value is NULL it is safe, tho you need to be aware of it's semantics, you can't just set it to NULL, it won't release the object. So code as follows won't work:
```c
void foo(void)
{
    EFAUTOREL EFStringRef someString = EFStringCreateWithCString(kEFAllocatorDefault, "meow", kEFStringEncodingUTF8);
    EFLog(EFSTR("%@\n"), someString);
    someString = NULL; /* lost in-case the creation succeeded */
}
```
to solve this you either don't do anything and let the scope end or if you really have to reuse it you shall do as follows:
```c
void foo(void)
{
    EFAUTOREL EFStringRef someString = EFStringCreateWithCString(kEFAllocatorDefault, "meow", kEFStringEncodingUTF8);
    EFLog(EFSTR("%@\n"), someString);
    EFReleasetry(someString); /* performs a null check before EFRelease() */
    someString = EFStringCreateWithCString(kEFAllocatorDefault, "nya", kEFStringEncodingUTF8);
    EFLog(EFSTR("%@\n"), someString);
    /* auto-release will still work */
}
```
later we will create a `EFAUTOSWAP` don't worry.

#### EFAUTOTRANSFER
since you can't just return the pointer in the scope because it would then drop to reference zero we made another macro which zeros and returns the value before return and returns the actual value. this is so this works:
```c
EFStringRef foo(void)
{
    EFAUTOREL EFStringRef someString = EFStringCreateWithCString(kEFAllocatorDefault, "meow", kEFStringEncodingUTF8);
    /* do something with it */
    return EFAUTOTRANSFER(someString); /* won't release value */
}
```

