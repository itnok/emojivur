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
 * \brief Compact way to check for validity of a pointer and exit in case it is not valid
 *
 * \param ptr       Generic pointer to check
 * \param error_msg Error message to present to the user (default: "")
 * \param exit_code Exit code to use
 *
 */
void emojivur_cleanup(emojivur_shared_ptrs_t *shared_data)
{
    if (!shared_data)
    {
        return;
    }

    if (shared_data->cairo_context)
    {
        cairo_destroy(shared_data->cairo_context);
    }

    if (shared_data->cairo_surface)
    {
        cairo_surface_flush(shared_data->cairo_surface);
        cairo_surface_destroy(shared_data->cairo_surface);
    }

    if (shared_data->cairo_font_face)
    {
        cairo_font_face_destroy(shared_data->cairo_font_face);
    }

    if (shared_data->cairo_glyphs)
    {
        cairo_glyph_free(shared_data->cairo_glyphs);
    }

    if (shared_data->harfbuzz_font)
    {
        hb_font_destroy(shared_data->harfbuzz_font);
    }

    if (shared_data->tmp_buffer)
    {
        hb_buffer_destroy(shared_data->tmp_buffer);
    }

    if (shared_data->window)
    {
        SDL_DestroyWindow(shared_data->window);
    }

    if (shared_data->renderer)
    {
        SDL_DestroyRenderer(shared_data->renderer);
    }

    if (shared_data->sdl_surface)
    {
        SDL_FreeSurface(shared_data->sdl_surface);
    }

    if (shared_data->sdl_texture)
    {
        SDL_DestroyTexture(shared_data->sdl_texture);
    }

    // SDL_Quit is safe to be called on any possibile exit condition
    // to make sure the SDL2 memory is completely and correctly released...
    SDL_Quit();
}

void emojivur_exit(emojivur_shared_ptrs_t *shared_data, char *error_msg, int exit_code)
{
    printf("[ERROR] %s (%d)", error_msg, exit_code);
    emojivur_cleanup(shared_data);
    exit(exit_code);
}

/*!
 * \brief Compact way to check for validity of a pointer and exit in case it is not valid
 *
 * \param ptr       Generic pointer to check
 * \param error_msg Error message to present to the user (default: "")
 * \param exit_code Exit code to use
 *
 */
void emojivur_ptr_valid_or_exit(emojivur_shared_ptrs_t *shared_data, void *ptr, char *error_msg, int exit_code)
{
    if (!ptr)
    {
        emojivur_exit(shared_data, error_msg, exit_code);
    }
}

/*!
 * \brief Set PDF document metatags
 *
 * \param cairo_pfd_surface Cairo PDF Surface associated with the PDF document to tag
 *
 */
void emojivur_set_pdf_metadata(cairo_surface_t *cairo_pdf_surface)
{
    char pdf_creator[128];
    snprintf(pdf_creator, 127, "%s v%s", APP_NAME, APP_VERSION);
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

//    __  __    _    ___ _   _
//   |  \/  |  / \  |_ _| \ | |
//   | |\/| | / _ \  | ||  \| |
//   | |  | |/ ___ \ | || |\  |
//   |_|  |_/_/   \_\___|_| \_|
//
//   #pragma MAIN

int main(int argc, char *argv[])
{
    struct gengetopt_args_info cli_args_info;
    if (cmdline_parser(argc, argv, &cli_args_info) != 0)
    {
        return 1;
    }

    // All pointers used are stored in this struct so that freeing them
    // at any point is trivial and code remains DRYer
    emojivur_shared_ptrs_t pshared = emojivur_shared_ptrs_default;

    // Load font using FreeType for Cairo
    FT_Library ft_library;
    if (FT_Init_FreeType(&ft_library) != 0)
    {
        emojivur_exit(&pshared,
                      "An error occured during the FreeType library initialization!", 1);
    }
    FT_Face ft_face = NULL;
    if (FT_New_Face(ft_library, cli_args_info.font_arg, 0, &ft_face) != 0)
    {
        emojivur_exit(&pshared,
                      "An error occured during the FreeType Font Face creation!", 1);
    }
    pshared.cairo_font_face = cairo_ft_font_face_create_for_ft_face(ft_face, 0);
    emojivur_ptr_valid_or_exit(&pshared, pshared.cairo_font_face,
                               "An error occurred during the Cairo Font Face creation!", 1);

    // For Harfbuzz, load using OpenType (HarfBuzz FT does not support bitmap font)
    hb_blob_t *blob = hb_blob_create_from_file(cli_args_info.font_arg);
    emojivur_ptr_valid_or_exit(&pshared, blob,
                               "An error occured during the HarfBuzz Blob creation!", 1);

    hb_face_t *face = hb_face_create(blob, 0);
    emojivur_ptr_valid_or_exit(&pshared, face,
                               "An error occured during the HarfBuzz Font Face creation!", 1);

    pshared.harfbuzz_font = hb_font_create(face);
    emojivur_ptr_valid_or_exit(&pshared, pshared.harfbuzz_font,
                               "An error occured during the HarfBuzz Font creation!", 1);

    hb_ot_font_set_funcs(pshared.harfbuzz_font);
    hb_font_set_scale(pshared.harfbuzz_font, cli_args_info.pxsize_arg * 64, cli_args_info.pxsize_arg * 64);

    // Create  HarfBuzz buffer
    pshared.tmp_buffer = hb_buffer_create();
    emojivur_ptr_valid_or_exit(&pshared, pshared.tmp_buffer,
                               "An error occured during the HarfBuzz work Buffer creation!", 1);

    // Set buffer to LTR direction, common script and default language
    hb_buffer_set_direction(pshared.tmp_buffer, HB_DIRECTION_LTR);
    hb_buffer_set_script(pshared.tmp_buffer, HB_SCRIPT_COMMON);
    hb_buffer_set_language(pshared.tmp_buffer, hb_language_get_default());

    // Add text and layout it
    hb_buffer_add_utf8(pshared.tmp_buffer, cli_args_info.text_arg, -1, 0, -1);
    hb_shape(pshared.harfbuzz_font, pshared.tmp_buffer, NULL, 0);

    // Get buffer data
    unsigned int glyph_count = hb_buffer_get_length(pshared.tmp_buffer);
    hb_glyph_info_t *glyph_info = hb_buffer_get_glyph_infos(pshared.tmp_buffer, NULL);
    emojivur_ptr_valid_or_exit(&pshared, glyph_info,
                               "An error occured during the HarfBuzz Glyph Information data creation!", 1);
    hb_glyph_position_t *glyph_pos = hb_buffer_get_glyph_positions(pshared.tmp_buffer, NULL);
    emojivur_ptr_valid_or_exit(&pshared, glyph_pos,
                               "An error occured during the HarfBuzz Glyph Positions vector creation!", 1);

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
    pshared.cairo_glyphs = cairo_glyph_allocate(glyph_count);

    int x = 0;
    int y = 0;
    for (int i = 0; i < glyph_count; ++i)
    {
        pshared.cairo_glyphs[i].index = glyph_info[i].codepoint;
        pshared.cairo_glyphs[i].x = x + (glyph_pos[i].x_offset / (64.0));
        pshared.cairo_glyphs[i].y = -(y + glyph_pos[i].y_offset / (64.0));
        x += glyph_pos[i].x_advance / (64.0);
        y += glyph_pos[i].y_advance / (64.0);

        printf("glyph codepoint=%lu size=(%g, %g) advance=(%g, %g)\n",
               pshared.cairo_glyphs[i].index,
               glyph_pos[i].x_advance / (64.0),
               glyph_pos[i].y_advance / (64.0),
               glyph_pos[i].x_advance / (64.0),
               glyph_pos[i].y_advance / (64.0));
    }

    SDL_DisplayMode dm;
    if (!cli_args_info.output_given)
    {
        // Initializing SDL2 makes sense only if not saving output to PDF
        if (SDL_Init(SDL_INIT_VIDEO) != 0)
        {
            char sdl_error_msg[128];

            snprintf(sdl_error_msg, 127, "SDL_Init failed: %s\n", SDL_GetError());
            emojivur_exit(&pshared, sdl_error_msg, 1);
        }

        // Get info about the screen size
        // TODO: What if there are more screens? Here checking only screen 0
        if (SDL_GetDesktopDisplayMode(0, &dm) != 0)
        {
            char sdl_error_msg[128];

            snprintf(sdl_error_msg, 127, "SDL_GetDesktopDisplayMode failed: %s\n", SDL_GetError());
            emojivur_exit(&pshared, sdl_error_msg, 1);
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
            pshared.cairo_glyphs[i].x += (margin_x / 2);
            pshared.cairo_glyphs[i].y += height - (margin_y / 2);
        }
        else
        {
            pshared.cairo_glyphs[i].x += (width / 2) - (text_width_in_pixels / 2);
            pshared.cairo_glyphs[i].y += (height / 2) + (margin_y / 2);
        }
    }

    if (cli_args_info.output_given)
    {
        emoji_to_render_t text_to_render =
            {
                .viewport = {width, height},
                .font_face = pshared.cairo_font_face,
                .glyphs = pshared.cairo_glyphs,
                .glyph_count = glyph_count,
                .glyph_size = cli_args_info.pxsize_arg,
            };
        emojivur_pdf_output(text_to_render, cli_args_info.output_arg);

        // Clean up destroying Cairo & HarfBuzz resources
        emojivur_cleanup(&pshared);

        // When generating a PDF no UI is going to be provided
        return 0;
    }

    // Draw text in SDL2 with Cairo
    SDL_WindowFlags videoFlags = SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI;

    pshared.window = SDL_CreateWindow(APP_NAME, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, videoFlags);
    if (!pshared.window)
    {
        char sdl_error_msg[128];

        snprintf(sdl_error_msg, 127, "Window could not be created! SDL2: %s\n", SDL_GetError());
        emojivur_exit(&pshared, sdl_error_msg, 1);
    }

    pshared.renderer = SDL_CreateRenderer(pshared.window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!pshared.renderer)
    {
        char sdl_error_msg[128];

        snprintf(sdl_error_msg, 127, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        emojivur_exit(&pshared, sdl_error_msg, 1);
    }

    // Compute screen resolution
    // On a HiDPI screen like Apple Retina Displays, renderer size is twice as window size
    int window_width;
    int window_height;
    SDL_GetWindowSize(pshared.window, &window_width, &window_height);

    int renderer_width;
    int renderer_height;
    SDL_GetRendererOutputSize(pshared.renderer, &renderer_width, &renderer_height);

    int cairo_x_multiplier = renderer_width / window_width;
    int cairo_y_multiplier = renderer_height / window_height;

    // Create a SDL2 surface for Cairo to render onto
    pshared.sdl_surface = SDL_CreateRGBSurface(
        0,
        renderer_width,
        renderer_height,
        32,
        0x00ff0000,
        0x0000ff00,
        0x000000ff,
        0);
    if (!pshared.sdl_surface)
    {
        char sdl_error_msg[128];

        snprintf(sdl_error_msg, 127, "SDL_CreateRGBSurface failed: %s\n", SDL_GetError());
        emojivur_exit(&pshared, sdl_error_msg, 1);
    }

    // Get Cairo surface from a SDL2 surface
    pshared.cairo_surface = cairo_image_surface_create_for_data(
        (unsigned char *)pshared.sdl_surface->pixels,
        CAIRO_FORMAT_RGB24,
        pshared.sdl_surface->w,
        pshared.sdl_surface->h,
        pshared.sdl_surface->pitch);
    emojivur_ptr_valid_or_exit(&pshared, pshared.cairo_surface,
                               "An error occured during the creation of the Cairo Surface associated with SDL2!", 1);

    // Scale cairo to use screen resolution
    cairo_surface_set_device_scale(pshared.cairo_surface, cairo_x_multiplier, cairo_y_multiplier);

    // Get Cairo context from Cairo surface
    pshared.cairo_context = cairo_create(pshared.cairo_surface);
    emojivur_ptr_valid_or_exit(&pshared, pshared.cairo_context,
                               "An error occured during main Cairo Context creation!", 1);
    cairo_set_source_rgba(pshared.cairo_context, 0, 0, 0, 1.0);
    cairo_set_font_face(pshared.cairo_context, pshared.cairo_font_face);
    cairo_set_font_size(pshared.cairo_context, cli_args_info.pxsize_arg);

    bool done = false;
    while (!done)
    {
        // Fill background in white
        SDL_FillRect(pshared.sdl_surface, NULL, SDL_MapRGB(pshared.sdl_surface->format, 255, 255, 255));

        // Render glyph onto cairo context (which render onto SDL2 surface)
        cairo_show_glyphs(pshared.cairo_context, pshared.cairo_glyphs, glyph_count);

        // Render SDL2 surface onto SDL2 renderer
        pshared.sdl_texture = SDL_CreateTextureFromSurface(pshared.renderer, pshared.sdl_surface);
        SDL_RenderCopy(pshared.renderer, pshared.sdl_texture, 0, 0);
        SDL_RenderPresent(pshared.renderer);

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

    emojivur_cleanup(&pshared);
    return 0;
}
