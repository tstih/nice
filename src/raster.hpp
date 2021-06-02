//
// raster.hpp
// 
// Raster image (32bpp raw data). This is used for all 
// images in nice.
//
// (c) 2021 Tomaz Stih
// This code is licensed under MIT license (see LICENSE.txt for details).
// 
// 02.06.2021   tstih
// 
#ifndef _RASTER_H
#define _RASTER_H

#include <cstdint>
#include <memory>

namespace nice {
//{{BEGIN.DEC}}
    class raster {
    public:
        // Constructs a new raster.        
        raster(int width, int height);
        // Destructs the raster.
        virtual ~raster();
    private:
        int width_, height_, stride_, len_;
        std::unique_ptr<uint8_t> data_;
    };
//{{END.DEC}}
} // namespace nice

#endif