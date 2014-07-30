/* Konami video helper functions */

#pragma once
#ifndef __KONAMI_HELPER_H__
#define __KONAMI_HELPER_H__

void konami_decode_gfx(running_machine &machine, gfxdecode_device * gfxdecode, palette_device &palette, int gfx_index, UINT8 *data, UINT32 total, const gfx_layout *layout, int bpp);

/* helper function to sort three tile layers by priority order */
void konami_sortlayers3(int *layer, int *pri);
/* helper function to sort four tile layers by priority order */
void konami_sortlayers4(int *layer, int *pri);
/* helper function to sort five tile layers by priority order */
void konami_sortlayers5(int *layer, int *pri);

#endif
