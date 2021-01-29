//
// resource.hpp
// 
// Two phase construction patter implementation (with lazy eval resource class).
//
// (c) 2020 Tomaz Stih
// This code is licensed under MIT license (see LICENSE.txt for details).
// 
// 16.01.2020   tstih
// 
#ifndef _RESOURCE_HPP
#define _RESOURCE_HPP

namespace nice {

//{{BEGIN.DEC}}
    template<typename T, T N = nullptr>
    class resource {
    public:
        // Create and destroy pattern.
        virtual T create() = 0;
        virtual void destroy() noexcept = 0;

        // Id setter.
        virtual void instance(T instance) const { instance_ = instance; }

        // Id getter with lazy eval.
        virtual T instance() const {
            // Lazy evaluate by callign create.
            if (instance_ == N)
                instance_ = const_cast<resource<T, N>*>(this)->create();
            // Return.
            return instance_;
        }

        bool initialized() {
            return !(instance_ == N);
        }

    private:
        // Store resource value here.
        mutable T instance_{ N };
    };
//{{END.DEC}}

}

#endif // _RESOURCE_HPP