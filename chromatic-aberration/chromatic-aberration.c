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
 *           2022 Liam Quin <slave@fromoldbooks.org>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

/*UI: Channel checkboxes*/
property_boolean (red_ch,    _("Red Channel"),   FALSE)
property_boolean (green_ch,  _("Green Channel"), TRUE)
property_boolean (blue_ch,   _("Blue Channel"),  TRUE)

/*UI: Lens zoom distortion slider*/
property_double (distortion_zoom, _("Distortion"), 2.0)
  description (_("Lens zoom distortion value"))
  value_range (0, 20)
  ui_steps    (1, 10)
  ui_digits   (2)

#else

#include <gegl-plugin.h>

#define GEGL_OP_META
#define GEGL_OP_NAME     chromatic_aberration
#define GEGL_OP_C_SOURCE chromatic-aberration.c

#include "gegl-op.h"

typedef struct
{
  GeglNode *input;
  GeglNode *output;
  GeglNode *nop;
  GeglNode *lens_dist;
  GeglNode *add_green;
  GeglNode *add_blue;
  GeglNode *ch_red;
  GeglNode *ch_green;
  GeglNode *ch_blue;
} State;

static void update_graph(GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  State *state = o->user_data;
  if (!state) return;

  /*Red Channel*/
  if (o->red_ch)
  {
    gegl_node_link_many (state->input, state->lens_dist, state->ch_red, state->add_green, state->add_blue, state->output, NULL);
  }
  else
  {
    gegl_node_link_many (state->input, state->nop, state->ch_red, state->add_green, state->add_blue, state->output, NULL);
  }

  /*Green Channel*/
  if (o->green_ch)
  {
    gegl_node_link_many (state->input, state->lens_dist, state->ch_green, NULL);
  }
  else
  {
    gegl_node_link_many (state->input, state->nop, state->ch_green, NULL);
  }

  /*Blue Channel*/
  if (o->blue_ch)
  {
    gegl_node_link_many (state->input, state->lens_dist, state->ch_blue, NULL);
  }
  else
  {
    gegl_node_link_many (state->input, state->nop, state->ch_blue, NULL);
  }
}

static void attach(GeglOperation *operation)
{
  GeglNode *gegl = operation->node;
  GeglProperties *o = GEGL_PROPERTIES (operation);
  GeglNode *input, *output, *nop, *lens_dist, *add_green, *add_blue, *ch_red, *ch_green, *ch_blue;

  /*Nodes*/
  input  = gegl_node_get_input_proxy  (gegl, "input");
  output = gegl_node_get_output_proxy (gegl, "output");

  nop       = gegl_node_new_child (gegl, "operation", "gegl:nop",             NULL);
  lens_dist = gegl_node_new_child (gegl, "operation", "gegl:lens-distortion", NULL);
  add_green = gegl_node_new_child (gegl, "operation", "gegl:add",             NULL);
  add_blue  = gegl_node_new_child (gegl, "operation", "gegl:add",             NULL);
  ch_red    = gegl_node_new_child (gegl, "operation", "gegl:channel-mixer",   "gg-gain", 0.0, "bb-gain", 0.0, NULL);
  ch_green  = gegl_node_new_child (gegl, "operation", "gegl:channel-mixer",   "rr-gain", 0.0, "bb-gain", 0.0, NULL);
  ch_blue   = gegl_node_new_child (gegl, "operation", "gegl:channel-mixer",   "rr-gain", 0.0, "gg-gain", 0.0, NULL);

  /*Redirections*/
  gegl_operation_meta_redirect (operation, "distortion_zoom", lens_dist, "zoom");

  /*Default settings: R[_] G[X] B[X]*/
  gegl_node_link_many (input, nop, ch_red, add_green, add_blue, output, NULL);

  gegl_node_connect_from(add_green, "aux", ch_green, "output");
  gegl_node_link_many (input, lens_dist, ch_green, NULL);

  gegl_node_connect_from(add_blue, "aux", ch_blue, "output");
  gegl_node_link_many (input, lens_dist, ch_blue, NULL);

  /*References for update_graph()*/
  State *state = g_malloc0 (sizeof (State));

  state->input     = input;
  state->output    = output;
  state->input     = input;
  state->output    = output;
  state->nop       = nop;
  state->lens_dist = lens_dist;
  state->add_green = add_green;
  state->add_blue  = add_blue;
  state->ch_red    = ch_red;
  state->ch_green  = ch_green;
  state->ch_blue   = ch_blue;

  o->user_data     = state;
}

static void gegl_op_class_init(GeglOpClass *klass)
{
  GeglOperationMetaClass *operation_meta_class = GEGL_OPERATION_META_CLASS (klass);
  GeglOperationClass     *operation_class = GEGL_OPERATION_CLASS (klass);

  operation_class->attach = attach;
  operation_meta_class->update = update_graph;

  gegl_operation_class_set_keys (operation_class,
    "name",           "gegl:chromatic-aberration",
    "title",          _("Chromatic Aberration"),
    "description",    _("Chromatic aberraiton using zoom distortion"),
    "categories",     "Artistic",
    "reference-hash", "deafbededeafbededeafbededeafbede",
    NULL);
}

#endif
