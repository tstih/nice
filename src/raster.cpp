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
        height_(height),
        stride_(width%sizeof(uint8_t)) {
        
        // Calculate raster length.
        len_=stride_ * height;
        // Allocate memory.
        data_=std::make_unique<uint8_t>(len_);
    }

    raster::~raster() {}

//{{END.DEF}}
}