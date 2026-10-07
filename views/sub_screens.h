// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2023-2026 SegerEnd, see LICENSE for additional terms

#pragma once

#include <gui/view.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SelectionMinifigure,
    SelectionVehicle,
    SelectionFavorites,
    SelectionSaved,
    SelectionCount // Total number of selections, must always be last
} SelectionType;

void draw_placement_selection_screen(Canvas* canvas, SelectionType selection);

#ifdef __cplusplus
}
#endif
