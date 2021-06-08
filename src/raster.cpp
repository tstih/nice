//
// raster.cpp
// 
// Raster image (32bpp raw data). This is used for all 
// images in nice.
//
// (c) 2021 Tomaz Stih
// This code is licensed under MIT license (see LICENSE.txt for details).
// 
// 02.06.2021   tstih
// 
#include "raster.hpp"

namespace nice {
//{{BEGIN.DEF}}
    
    raster::raster(int width, int height) : 
        width_(width), 
        height_(height) {
        
        // Calculate raster length.
        len_ = width * height * 3;
        // Allocate memory.
        raw_=std::make_unique<uint8_t[]>(len_+1);
    }

    raster::raster(int width, int height, const uint8_t* bgrarr) :
        raster(width,height) {
        // Copy BGR array.
        std::copy(bgrarr, bgrarr + len_, raw_.get());
    }

    raster::~raster() {}

    int raster::width() const {
        return width_;
    }

    int raster::height() const {
        return height_;
    }

    uint8_t* raster::raw() const {
        return raw_.get();
    }
//{{END.DEF}}
}