#pragma once

#include <SDL3/SDL.h>

#define RESIZE_BORDER 8

inline SDL_HitTestResult HitTestCallback(SDL_Window *window, const SDL_Point *area, void *data) {
    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    
    if (area->y < RESIZE_BORDER) {
        if (area->x < RESIZE_BORDER) return SDL_HITTEST_RESIZE_TOPLEFT;
        if (area->x > w - RESIZE_BORDER) return SDL_HITTEST_RESIZE_TOPRIGHT;
        return SDL_HITTEST_RESIZE_TOP;
    }
    if (area->y > h - RESIZE_BORDER) {
        if (area->x < RESIZE_BORDER) return SDL_HITTEST_RESIZE_BOTTOMLEFT;
        if (area->x > w - RESIZE_BORDER) return SDL_HITTEST_RESIZE_BOTTOMRIGHT;
        return SDL_HITTEST_RESIZE_BOTTOM;
    }
    if (area->x < RESIZE_BORDER) return SDL_HITTEST_RESIZE_LEFT;
    if (area->x > w - RESIZE_BORDER) return SDL_HITTEST_RESIZE_RIGHT;

    if (area->y < 40) {
        return SDL_HITTEST_DRAGGABLE;
    }
    
    return SDL_HITTEST_DRAGGABLE;
}