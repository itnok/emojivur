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

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include <harfbuzz/hb.h>
#include <harfbuzz/hb-ot.h>

#include <cairo/cairo.h>
#include <cairo/cairo-ft.h>
#include <cairo/cairo-pdf.h>

#include "config.h"
#include "cli_options.h"
#include "emojivur.h"

#define UNUSED(x) ((void)(x))
#define MIN(a, b)               \
    ({                          \
        __typeof__(a) _a = (a); \
        __typeof__(b) _b = (b); \
        _a < _b ? _a : _b;      \
    })
#define MAX(a, b)               \
    ({                          \
        __typeof__(a) _a = (a); \
        __typeof__(b) _b = (b); \
        _a > _b ? _a : _b;      \
    })

/*!
 * \brief Set PDF document metatags
 *
 * \param cairo_pfd_surface Cairo PDF Surface associated with the PDF document to tag
 *
 */
void emojivur_set_pdf_metadata(cairo_surface_t *cairo_pdf_surface)
{
    char pdf_creator[128];
    sprintf(pdf_creator, "%s v%s", APP_NAME, APP_VERSION);
    cairo_pdf_surface_set_metadata(cairo_pdf_surface, CAIRO_PDF_METADATA_CREATOR, pdf_creator);
}

/*!
 * \brief Create a single page PDF document containing all emojis provided on one line
 *
 * \param emoji             Configuration for the Cairo surface to create and render
 * \param pdf_filename      File name for the PDF to create
 *
 */
void emojivur_pdf_output(emoji_to_render_t emoji, char *pdf_filename)
{
    // Creating a cairo PDF Surface (using SDL2 window width & height to size it)
    cairo_surface_t *cairo_pdf_surface = cairo_pdf_surface_create(
        pdf_filename,
        emoji.viewport.w,
        emoji.viewport.h);

    // Creating a Cairo context
    cairo_t *cairo_pdf_context = cairo_create(cairo_pdf_surface);

    emojivur_set_pdf_metadata(cairo_pdf_surface);

    cairo_set_source_rgba(cairo_pdf_context, 0, 0, 0, 1.0);
    cairo_set_font_face(cairo_pdf_context, emoji.font_face);
    cairo_set_font_size(cairo_pdf_context, emoji.glyph_size);

    // Render glyph onto cairo context (which render onto SDL2 surface)
    cairo_show_glyphs(cairo_pdf_context, emoji.glyphs, emoji.glyph_count);

    // Flush page to render it and clear the context eventually for following pages
    cairo_show_page(cairo_pdf_context);

    // Clean up
    cairo_surface_flush(cairo_pdf_surface);
    cairo_surface_destroy(cairo_pdf_surface);
    cairo_destroy(cairo_pdf_context);
}

int main(int argc, char *argv[])
{
    struct gengetopt_args_info cli_args_info;
    if (cmdline_parser(argc, argv, &cli_args_info) != 0)
    {
        return 1;
    }

    // Load fonts
    cairo_font_face_t *cairo_ft_face;
    hb_font_t *hb_ft_font;

    // Load font using FreeType for Cairo
    FT_Library ft_library;
    assert(FT_Init_FreeType(&ft_library) == 0);
    UNUSED(ft_library);
    FT_Face ft_face = NULL;
    assert(FT_New_Face(ft_library, cli_args_info.font_arg, 0, &ft_face) == 0);
    cairo_ft_face = cairo_ft_font_face_create_for_ft_face(ft_face, 0);

    // For Harfbuzz, load using OpenType (HarfBuzz FT does not support bitmap font)
    hb_blob_t *blob = hb_blob_create_from_file(cli_args_info.font_arg);
    hb_face_t *face = hb_face_create(blob, 0);
    hb_ft_font = hb_font_create(face);
    hb_ot_font_set_funcs(hb_ft_font);
    hb_font_set_scale(hb_ft_font, cli_args_info.pxsize_arg * 64, cli_args_info.pxsize_arg * 64);

    // Create  HarfBuzz buffer
    hb_buffer_t *buf = hb_buffer_create();

    // Set buffer to LTR direction, common script and default language
    hb_buffer_set_direction(buf, HB_DIRECTION_LTR);
    hb_buffer_set_script(buf, HB_SCRIPT_COMMON);
    hb_buffer_set_language(buf, hb_language_get_default());

    // Add text and layout it
    hb_buffer_add_utf8(buf, cli_args_info.text_arg, -1, 0, -1);
    hb_shape(hb_ft_font, buf, NULL, 0);

    // Get buffer data
    unsigned int glyph_count = hb_buffer_get_length(buf);
    hb_glyph_info_t *glyph_info = hb_buffer_get_glyph_infos(buf, NULL);
    hb_glyph_position_t *glyph_pos = hb_buffer_get_glyph_positions(buf, NULL);

    unsigned int text_width_in_pixels = 0;
    unsigned int text_height_in_pixels = cli_args_info.pxsize_arg;
    for (int i = 0; i < glyph_count; ++i)
    {
        text_width_in_pixels += glyph_pos[i].x_advance / (64.0);
        text_height_in_pixels = MAX(text_height_in_pixels, glyph_pos[i].y_advance / (64.0));
    }

    printf("glyph count=%d\n", glyph_count);
    printf("text width=%d pixels\n", text_width_in_pixels);
    printf("text height=%d pixels\n", text_height_in_pixels);

    // Shape glyph for Cairo
    cairo_glyph_t *cairo_glyphs = cairo_glyph_allocate(glyph_count);
    int x = 0;
    int y = 0;
    for (int i = 0; i < glyph_count; ++i)
    {
        cairo_glyphs[i].index = glyph_info[i].codepoint;
        cairo_glyphs[i].x = x + (glyph_pos[i].x_offset / (64.0));
        cairo_glyphs[i].y = -(y + glyph_pos[i].y_offset / (64.0));
        x += glyph_pos[i].x_advance / (64.0);
        y += glyph_pos[i].y_advance / (64.0);

        printf("glyph codepoint=%lu size=(%g, %g) advance=(%g, %g)\n",
               cairo_glyphs[i].index,
               glyph_pos[i].x_advance / (64.0),
               glyph_pos[i].y_advance / (64.0),
               glyph_pos[i].x_advance / (64.0),
               glyph_pos[i].y_advance / (64.0));
    }

    SDL_DisplayMode dm;
    if (!cli_args_info.output_given)
    {
        // Initializing SDL2 makes sense only if not saving output to PDF
        assert(SDL_Init(SDL_INIT_VIDEO) == 0);

        // Get info about the screen size
        // TODO: What if there are more screens? Here checking only screen 0
        if (SDL_GetDesktopDisplayMode(0, &dm) != 0)
        {
            if (cairo_glyphs)
            {
                free(cairo_glyphs);
            }
            SDL_Log("SDL_GetDesktopDisplayMode failed: %s", SDL_GetError());
            return 1;
        }
    }

    // Decide what the viewport size is going to be like
    int margin_x = cli_args_info.pxsize_arg;
    int margin_y = cli_args_info.pxsize_arg;
    int max_width = MIN(text_width_in_pixels + margin_x, dm.w);
    int width = MAX(MIN_WINDOW_WIDTH, max_width);
    int max_height = MIN(text_height_in_pixels + margin_y, dm.h);
    int height = MAX(MIN_WINDOW_HEIGHT, max_height);

    // For PDF files reduce the margins not caring of SDL2 window size
    if (cli_args_info.output_given)
    {
        margin_x = round(cli_args_info.pxsize_arg / (64.0));
        margin_y = round(cli_args_info.pxsize_arg / (64.0));
        width = text_width_in_pixels + margin_x;
        height = text_height_in_pixels + margin_y;
    }

    // Move glyph to be at the center of the viewport
    for (int i = 0; i < glyph_count; ++i)
    {
        // PDF & Screen have coordianates origin in different places...
        if (cli_args_info.output_given)
        {
            cairo_glyphs[i].x -= margin_x;
            cairo_glyphs[i].y += height - (margin_y / 2);
        }
        else
        {
            cairo_glyphs[i].x += (width / 2) - (text_width_in_pixels / 2);
            cairo_glyphs[i].y += (height / 2) + (margin_y / 2);
        }
    }

    if (cli_args_info.output_given)
    {
        emoji_to_render_t text_to_render =
            {
                .viewport = {width, height},
                .font_face = cairo_ft_face,
                .glyphs = cairo_glyphs,
                .glyph_count = glyph_count,
                .glyph_size = cli_args_info.pxsize_arg,
            };
        emojivur_pdf_output(text_to_render, cli_args_info.output_arg);

        // Clean up destroying Cairo & HarfBuzz resources
        if (cairo_glyphs)
        {
            free(cairo_glyphs);
        }
        cairo_font_face_destroy(cairo_ft_face);
        hb_font_destroy(hb_ft_font);

        // When generating a PDF no UI is going to be provided
        return 0;
    }

    // Draw text in SDL2 with Cairo
    SDL_WindowFlags videoFlags = SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI;

    SDL_Window *window = NULL;

    window = SDL_CreateWindow(APP_NAME, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, videoFlags);
    if (window == NULL)
    {
        printf("[ERROR] Window could not be created! SDL2: %s\n", SDL_GetError());

        // Clean up destroying Cairo & HarfBuzz resources
        if (cairo_glyphs)
        {
            free(cairo_glyphs);
        }
        cairo_font_face_destroy(cairo_ft_face);
        hb_font_destroy(hb_ft_font);

        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    // Compute screen resolution
    // On a HiDPI screen like Apple Retina Displays, renderer size is twice as window size
    int window_width;
    int window_height;
    SDL_GetWindowSize(window, &window_width, &window_height);

    int renderer_width;
    int renderer_height;
    SDL_GetRendererOutputSize(renderer, &renderer_width, &renderer_height);

    int cairo_x_multiplier = renderer_width / window_width;
    int cairo_y_multiplier = renderer_height / window_height;

    // Create a SDL2 surface for Cairo to render onto
    SDL_Surface *sdl_surface = SDL_CreateRGBSurface(
        0,
        renderer_width,
        renderer_height,
        32,
        0x00ff0000,
        0x0000ff00,
        0x000000ff,
        0);

    // Get Cairo surface from a SDL2 surface
    cairo_surface_t *cairo_surface = cairo_image_surface_create_for_data(
        (unsigned char *)sdl_surface->pixels,
        CAIRO_FORMAT_RGB24,
        sdl_surface->w,
        sdl_surface->h,
        sdl_surface->pitch);

    // Scale cairo to use screen resolution
    cairo_surface_set_device_scale(cairo_surface, cairo_x_multiplier, cairo_y_multiplier);

    // Get Cairo context from Cairo surface
    cairo_t *cairo_context = cairo_create(cairo_surface);
    cairo_set_source_rgba(cairo_context, 0, 0, 0, 1.0);
    cairo_set_font_face(cairo_context, cairo_ft_face);
    cairo_set_font_size(cairo_context, cli_args_info.pxsize_arg);

    bool done = false;
    while (!done)
    {
        // Fill background in white
        SDL_FillRect(sdl_surface, NULL, SDL_MapRGB(sdl_surface->format, 255, 255, 255));

        // Render glyph onto cairo context (which render onto SDL2 surface)
        cairo_show_glyphs(cairo_context, cairo_glyphs, glyph_count);

        // Render SDL2 surface onto SDL2 renderer
        SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, sdl_surface);
        SDL_RenderCopy(renderer, texture, 0, 0);
        SDL_RenderPresent(renderer);

        // Quit app on close event
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_QUIT:
                done = true;
                break;

            default:
                break;
            }
        }
    }

    // Free memory
    if (cairo_glyphs)
    {
        free(cairo_glyphs);
    }
    cairo_surface_flush(cairo_surface);
    cairo_surface_destroy(cairo_surface);
    cairo_destroy(cairo_context);
    cairo_font_face_destroy(cairo_ft_face);
    hb_font_destroy(hb_ft_font);
    SDL_FreeSurface(sdl_surface);

    SDL_Quit();
    return 0;
}
