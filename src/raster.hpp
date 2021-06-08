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
        // Construct a raster from resource.
        raster(int width, int height, const uint8_t * bgrarr);
        // Destructs the raster.
        virtual ~raster();
        // Width.
        int width() const;
        // Height.
        int height() const;
        // Pointer to raw data.
        uint8_t* raw() const;
    private:
        int width_, height_, len_;
        std::unique_ptr<uint8_t[]> raw_; // We own this!
    };
//{{END.DEC}}
} // namespace nice

#endif