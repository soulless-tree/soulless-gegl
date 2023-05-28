/* This file is an image processing operation for GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, see <https://www.gnu.org/licenses/>.
 *
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 *           2023 Soulless Tree <soullesstree@proton.me>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

/*UI: Metallic effect*/
property_double (metallic_effect, _("Metallic Effect"), 1.5)
  description (_("Strength of the metallic effect"))
  value_range (0, 5)
  ui_steps    (0.1, 1.0)

property_boolean (invert, _("Invert Metallic"), FALSE)

/*UI: Alien Map Channels */
property_double (red_frequency, _("Red Frequency"), 5.0)
  description (_("Red frequency"))
  value_range (0, 20)
  ui_steps    (0.1, 1.0)
  ui_digits   (1) 

property_double (green_frequency, _("Green Frequency"), 3.0)
  description (_("Green frequency"))
  value_range (0, 20)
  ui_steps    (0.1, 1.0)

property_double (blue_frequency, _("Blue Frequency"), 0.3)
  description (_("Blue frequency"))
  value_range (0, 20)
  ui_steps    (0.1, 1.0)

/*UI: Shadow Controls*/
property_double (holo_shadow_threshold, _("Shadow Colors Threshold"), 0.55)
  description (_("Presence of color in the shadow areas"))
  value_range (0, 1)
  ui_steps    (0.05, 0.1)

property_double (holo_shadow_opacity, _("Opacity"), 1)
  value_range (0, 1)
  ui_steps    (0.05, 0.1)

/*UI: Light Controls*/
property_double (holo_light_threshold, _("Light Colors Threshold"), 0.4)
  description (_("Presence of color in the light areas"))
  value_range (0, 1)
  ui_steps    (0.05, 0.1)

/*UI: Opacity Shadow */
property_double (holo_light_opacity, _("Opacity"), 0.6)
  value_range (0, 1)
  ui_steps    (0.05, 0.1)

#else

#include <gegl-plugin.h>

#define GEGL_OP_META
#define GEGL_OP_NAME     holographic_effect 
#define GEGL_OP_C_SOURCE holographic-effect.c

#include "gegl-op.h"

typedef struct
{
  GeglNode *input; 
  GeglNode *output; 
  GeglNode *src_holo_1; 
  GeglNode *src_holo_2; 
  GeglNode *src_metal; 
  GeglNode *src_metal_n_shadows; 
  GeglNode *invert; 
  GeglNode *saturation; 
  GeglNode *darken; 
  GeglNode *lighten; 
  GeglNode *metallic_1; 
  GeglNode *metallic_2; 
  GeglNode *holo; 
  GeglNode *holo_shadow; 
  GeglNode *holo_shadow_threshold; 
  GeglNode *holo_shadow_opacity; 
  GeglNode *holo_light; 
  GeglNode *holo_light_threshold;
  GeglNode *holo_light_opacity;
} State;

static void update_graph(GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  State *state = o->user_data;
  if (!state) return;

  if (o->invert)
  {
    gegl_node_link_many (state->input, state->metallic_1, state->metallic_2, state->invert, state->src_holo_1, state->darken, state->holo_shadow_opacity, state->src_metal, state->holo_shadow, state->src_holo_2, state->lighten, state->holo_light_opacity, state->src_metal_n_shadows, state->holo_light, state->output, NULL);

    gegl_node_connect_from(state->src_metal, "aux", state->invert, "output");
    gegl_node_connect_from(state->holo_shadow, "aux", state->holo_shadow_opacity, "output");
  }
  else
  {
    gegl_node_link_many (state->input, state->metallic_1, state->metallic_2, state->src_holo_1, state->darken, state->holo_shadow_opacity, state->src_metal, state->holo_shadow, state->src_holo_2, state->lighten, state->holo_light_opacity, state->src_metal_n_shadows, state->holo_light, state->output, NULL);

    gegl_node_connect_from(state->src_metal, "aux", state->metallic_2, "output");
    gegl_node_connect_from(state->holo_shadow, "aux", state->holo_shadow_opacity, "output");
  }
}

static void attach(GeglOperation *operation)
{
  GeglNode *gegl = operation->node;
  GeglProperties *o = GEGL_PROPERTIES (operation);
  GeglNode *input, *output, *invert, *src_holo_1, *src_holo_2, *src_metal, *src_metal_n_shadows, *saturation, *darken, *lighten, *metallic_1, *metallic_2, *holo, *holo_shadow, *holo_shadow_threshold, *holo_shadow_opacity, *holo_light, *holo_light_threshold, *holo_light_opacity;

  /*Nodes*/
  input  = gegl_node_get_input_proxy  (gegl, "input");
  output = gegl_node_get_output_proxy (gegl, "output");

  invert 	      = gegl_node_new_child (gegl, "operation", "gegl:invert-linear", NULL);
  saturation          = gegl_node_new_child (gegl, "operation", "gegl:saturation", "scale", 0, NULL);

  src_holo_1          = gegl_node_new_child (gegl, "operation", "svg:src", NULL);
  src_metal 	      = gegl_node_new_child (gegl, "operation", "svg:src", NULL);
  src_holo_2          = gegl_node_new_child (gegl, "operation", "svg:src", NULL);
  src_metal_n_shadows = gegl_node_new_child (gegl, "operation", "svg:src", NULL);

  darken = gegl_node_new_child (gegl, "operation", "svg:darken", NULL);
  lighten = gegl_node_new_child (gegl, "operation", "svg:lighten", NULL);

  metallic_1 = gegl_node_new_child (gegl, "operation", "gegl:alien-map", NULL);
  metallic_2 = gegl_node_new_child (gegl, "operation", "gegl:alien-map",
		  "cpn-1-frequency", 1.0, "cpn-2-frequency", 1.0, "cpn-3-frequency", 1.0, NULL);

  holo = gegl_node_new_child (gegl, "operation", "gegl:alien-map", NULL);

  holo_shadow = gegl_node_new_child (gegl, "operation", "svg:lighten", NULL);
  holo_shadow_threshold = gegl_node_new_child (gegl, "operation", "gegl:threshold", NULL);
  holo_shadow_opacity = gegl_node_new_child (gegl, "operation", "gegl:opacity", NULL);

  holo_light = gegl_node_new_child (gegl, "operation", "svg:darken", NULL);
  holo_light_threshold = gegl_node_new_child (gegl, "operation", "gegl:threshold", NULL);
  holo_light_opacity = gegl_node_new_child (gegl, "operation", "gegl:opacity", NULL);

  /*Redirections*/
  gegl_operation_meta_redirect (operation, "metallic_effect", metallic_1, "cpn-1-frequency");
  gegl_operation_meta_redirect (operation, "metallic_effect", metallic_1, "cpn-2-frequency");
  gegl_operation_meta_redirect (operation, "metallic_effect", metallic_1, "cpn-3-frequency");

  gegl_operation_meta_redirect (operation, "red_frequency", holo, "cpn-1-frequency");
  gegl_operation_meta_redirect (operation, "green_frequency", holo, "cpn-2-frequency");
  gegl_operation_meta_redirect (operation, "blue_frequency", holo, "cpn-3-frequency");

  gegl_operation_meta_redirect (operation, "holo_shadow_threshold", holo_shadow_threshold, "value");
  gegl_operation_meta_redirect (operation, "holo_shadow_opacity", holo_shadow_opacity, "value");

  gegl_operation_meta_redirect (operation, "holo_light_threshold", holo_light_threshold, "value");
  gegl_operation_meta_redirect (operation, "holo_light_opacity", holo_light_opacity, "value");

  /*Node connections*/
  gegl_node_link_many (input, metallic_1, metallic_2, src_holo_1, darken, holo_shadow_opacity, src_metal, holo_shadow, src_holo_2, lighten, holo_light_opacity, src_metal_n_shadows, holo_light, output, NULL);
  
  gegl_node_connect_from(src_holo_1, "aux", holo, "output");
  gegl_node_link_many (input, holo, NULL);

  gegl_node_connect_from(darken, "aux", holo_shadow_threshold, "output");
  gegl_node_link_many (src_holo_1, holo_shadow_threshold, NULL);

  gegl_node_connect_from(src_metal, "aux", metallic_2, "output");
  gegl_node_connect_from(holo_shadow, "aux", holo_shadow_opacity, "output");

  gegl_node_connect_from(src_holo_2, "aux", holo, "output");
  gegl_node_link_many (input, holo, NULL);

  gegl_node_connect_from(lighten, "aux", holo_light_threshold, "output");
  gegl_node_link_many (src_holo_1, holo_light_threshold, NULL);

  gegl_node_connect_from(src_metal_n_shadows, "aux", holo_shadow, "output");
  gegl_node_connect_from(holo_light, "aux", holo_light_opacity, "output");

  /*References for update_graph()*/
  State *state = g_malloc0 (sizeof (State));

  state->input = input; 
  state->output = output; 
  state->src_holo_1 = src_holo_1; 
  state->src_holo_2 = src_holo_2; 
  state->src_metal = src_metal; 
  state->src_metal_n_shadows = src_metal_n_shadows; 
  state->invert = invert; 
  state->saturation = saturation; 
  state->darken = darken; 
  state->lighten = lighten; 
  state->metallic_1 = metallic_1; 
  state->metallic_2 = metallic_2; 
  state->holo = holo; 
  state->holo_shadow = holo_shadow; 
  state->holo_shadow_threshold = holo_shadow_threshold; 
  state->holo_shadow_opacity = holo_shadow_opacity; 
  state->holo_light = holo_light; 
  state->holo_light_threshold = holo_light_threshold;
  state->holo_light_opacity = holo_light_opacity;

  o->user_data     = state;
}

static void gegl_op_class_init(GeglOpClass *klass)
{
  GeglOperationMetaClass *operation_meta_class = GEGL_OPERATION_META_CLASS (klass);
  GeglOperationClass     *operation_class = GEGL_OPERATION_CLASS (klass);

  operation_class->attach = attach;
  operation_meta_class->update = update_graph;

  gegl_operation_class_set_keys (operation_class,
    "name",           "gegl:holographic-effect",
    "title",          _("Holographic Effect"),
    "description",    _("Holographic effect using alien maps."),
    "categories",     "Artistic",
    "reference-hash", "deafbededeafbededeafbededeafbede",
    NULL);
}

#endif
