/* include/graphics/SkDeque.h
**
** Copyright 2006, Google Inc.
**
** Licensed under the Apache License, Version 2.0 (the "License"); 
** you may not use this file except in compliance with the License. 
** You may obtain a copy of the License at 
**
**     http://www.apache.org/licenses/LICENSE-2.0 
**
** Unless required by applicable law or agreed to in writing, software 
** distributed under the License is distributed on an "AS IS" BASIS, 
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
** See the License for the specific language governing permissions and 
** limitations under the License.
*/

#ifndef SkTDeque_DEFINED
#define SkTDeque_DEFINED

#include "SkNoncopyable.h"

template <typename T> struct sk_trait_trivial_constructor  { enum { value = false }; };
template <typename T> struct sk_trait_trivial_destructor   { enum { value = false }; };
template <typename T> struct sk_trait_trivial_copy         { enum { value = false }; };
template <typename T> struct sk_trait_trivial_assign       { enum { value = false }; };

template <typename T> struct sk_traits {
    enum {
        has_trivial_constructor = sk_trait_trivial_constructor<T>::value,
        has_trivial_destructor  = sk_trait_trivial_destructor<T>::value,
        has_trivial_copy        = sk_trait_trivial_copy<T>::value,
        has_trivial_assign      = sk_trait_trivial_assign<T>::value
    };
};

#define SK_SET_BASIC_TRAITS(T)                                                  \
    template <> struct sk_trait_trivial_constructor<T> { enum { value = true }; };     \
    template <> struct sk_trait_trivial_destructor<T>  { enum { value = true }; };     \
    template <> struct sk_trait_trivial_copy<T>        { enum { value = true }; };     \
    template <> struct sk_trait_trivial_assign<T>      { enum { value = true }; }

#define SK_SET_TYPE_TRAITS(T, ctor, dtor, copy, asgn)                           \
    template <> struct sk_trait_trivial_constructor<T> { enum { value = ctor }; };     \
    template <> struct sk_trait_trivial_destructor<T>  { enum { value = dtor }; };     \
    template <> struct sk_trait_trivial_copy<T>        { enum { value = copy }; };     \
    template <> struct sk_trait_trivial_assign<T>      { enum { value = asgn }; }


SK_SET_BASIC_TRAITS(void);
SK_SET_BASIC_TRAITS(bool);
SK_SET_BASIC_TRAITS(char);
SK_SET_BASIC_TRAITS(unsigned char);
SK_SET_BASIC_TRAITS(short);
SK_SET_BASIC_TRAITS(unsigned short);
SK_SET_BASIC_TRAITS(int);
SK_SET_BASIC_TRAITS(unsigned int);
SK_SET_BASIC_TRAITS(long);
SK_SET_BASIC_TRAITS(unsigned long);
#ifdef SkLONGLONG
    SK_SET_BASIC_TRAITS(SkLONGLONG);
#endif
#ifdef SK_CAN_USE_FLOAT
    SK_SET_BASIC_TRAITS(float);
    SK_SET_BASIC_TRAITS(double);
#endif

///////////////////////////////////////////////////////////////////////////////////////////////

#include <new>

class SkDeque : SkNoncopyable {
public:
    SkDeque(size_t elemSize);
    SkDeque(size_t elemSize, void* storage, size_t storageSize);
    ~SkDeque();

    bool    empty() const { return fCount == 0; }
    int     count() const { return fCount; }

    const void* front() const;
    const void* back() const;

    void* front()
    {
        return (void*)((const SkDeque*)this)->front();
    }
    void* back()
    {
        return (void*)((const SkDeque*)this)->back();
    }

    void*   push_front();
    void*   push_back();
    
    void    pop_front();
    void    pop_back();
    
    SkDEBUGCODE(static void UnitTest();)

private:
    struct Head;

public:
    class Iter {
    public:
        Iter(const SkDeque& d);
        void* next();

    private:
        SkDeque::Head*  fHead;
        char*           fPos;
        size_t          fElemSize;
    };

private:
    Head*   fFront;
    Head*   fBack;
    size_t  fElemSize;
    void*   fInitialStorage;
    int     fCount;
    
    friend class Iter;
};

template <typename T> class SkTDeque : SkNoncopyable {
public:
    SkTDeque() : fD(sizeof(T)) {}
    SkTDeque(T storage[], int count) : fD(sizeof(T), storage, count * sizeof(T)) {}
    inline ~SkTDeque();

    bool empty() const { return fD.empty(); }
    int count() const { return fD.count(); }

    T*          front() { return (T*)fD.front(); }
    const T*    front() const { return (const T*)fD.front(); }
    T*          back() { return (T*)fD.back(); }
    const T*    back() const { return (const T*)fD.back(); }
    
    T* push_front()
    {
        T* front = (T*)fD.push_front();
        if (!sk_traits<T>::has_trivial_constructor) {
            new(front) T();
        }
        return front;
    }
    T* push_back()
    {
        T* back = (T*)fD.push_back();
        if (!sk_traits<T>::has_trivial_constructor) {
            new(back) T();
        }
        return back;
    }

    T* push_front(const T& value)
    {
        T* front = (T*)fD.push_front();
        if (sk_traits<T>::has_trivial_copy) {
            *front = value;
        }
        else {
            new(front) T(value);
        }
        return front;
    }
    T* push_back(const T& value)
    {
        T* back = (T*)fD.push_back();
        if (sk_traits<T>::has_trivial_copy) {
            *back = value;
        }
        else {
            new(back) T(value);
        }
        return back;
    }
    
    void pop_front()
    {
        if (!sk_traits<T>::has_trivial_destructor) {
            this->front()->~T();
        }
        fD.pop_front();
    }
    void pop_back()
    {
        if (!sk_traits<T>::has_trivial_destructor) {
            this->back()->~T();
        }
        fD.pop_back();
    }
    
    class Iter : private SkDeque::Iter {
    public:
        Iter(const SkTDeque<T>& d) : SkDeque::Iter(d.fD) {}
        T* next() { return (T*)SkDeque::Iter::next(); }
    };

private:
    SkDeque fD;
    
    friend class Iter;
};

template <size_t COUNT, typename T> class SkSTDeque : public SkTDeque<T> {
public:
    SkSTDeque() : SkTDeque<T>((T*)fStorage, COUNT) {}
    
private:
    uint32_t fStorage[SkAlign4(COUNT * sizeof(T))];
};

////////////////////////////////////////////////////////////////////////////////////

template <typename T> inline SkTDeque<T>::~SkTDeque()
{
    if (!sk_traits<T>::has_trivial_destructor)
    {
        Iter   iter(*this);
        T*     t;
        while ((t = iter.next()) != NULL) {
            t->~T();
        }
    }
}

#endif
