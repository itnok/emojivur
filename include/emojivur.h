//  ------------------------------------------------------------------------  //
//                        _ _                                                 //
//    ___ _ __ ___   ___ (_|_)_   ___   _ _ __                                //
//   / _ \ '_ ` _ \ / _ \| | \ \ / / | | | '__|                               //
//  |  __/ | | | | | (_) | | |\ V /| |_| | |                                  //
//   \___|_| |_| |_|\___// |_| \_/  \__,_|_|                                  //
//                     |__/                                                   //
//                                                                            //
//  ------------------------------------------------------------------------  //
//  emojivur                                                                  //
//  Lightweight emoji viewer and PDF conversion utility                       //
//  ------------------------------------------------------------------------  //
//  Copyright (c) 2020 Simone Conti, @itnok <s.conti@itnok.com>               //
//  All Rights Reserved.                                                      //
//                                                                            //
//  Distributed under MIT license.                                            //
//  See file LICENSE for detail                                               //
//  or copy at https://opensource.org/licenses/MIT                            //
//  ------------------------------------------------------------------------  //
//  \file       emojivur.c
//  \author     Simone Conti (itnok)
//  \date       2020/05/17
//
//  \brief
//
#ifndef EMOJIVUR_H
#define EMOJIVUR_H

#include <harfbuzz/hb.h>
#include <harfbuzz/hb-ot.h>

#include <cairo/cairo.h>
#include <cairo/cairo-ft.h>

/*!
 * \brief A simple pair of width & height to define any viewport
 *
 */
typedef struct
{
    unsigned int w; /**< Width of the viewport */
    unsigned int h; /**< Height of the viewport */
} emoji_viewport_t;

/*!
 * \brief Cairo surface configuration
 *
 * Provides all the information to create a Cairo surface and to render it presenting the previously created glyphs.
 *
 */
typedef struct
{
    emoji_viewport_t viewport;    /**< Size of the viewport covered by the Cairo surface */
    cairo_font_face_t *font_face; /**< Cairo font face to use */
    cairo_glyph_t *glyphs;        /**< Vector of Cairo glyphs to render */
    unsigned int glyph_count;     /**< Number of Cairo glyphs to render */
    unsigned int glyph_size;      /**< Size in pixels for the glyphs to render */
} emoji_to_render_t;

struct emojivur_shared_ptrs_temp
{
    // Cairo
    cairo_t *cairo_context;
    cairo_surface_t *cairo_surface;
    cairo_font_face_t *cairo_font_face;
    cairo_glyph_t *cairo_glyphs;

    // HarfBuzz
    hb_font_t *harfbuzz_font;
    hb_buffer_t *tmp_buffer;

    // SDL2
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Surface *sdl_surface;
    SDL_Texture *sdl_texture;
} emojivur_shared_ptrs_default = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
typedef struct emojivur_shared_ptrs_temp emojivur_shared_ptrs_t;

#endif // EMOJIVUR_H
