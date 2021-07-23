/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "BKE_mesh_remesh_voxel.h"

#include "GEO_weld.h"

#include "UI_interface.h"
#include "UI_resources.h"

#include "node_geometry_util.hh"

static bNodeSocketTemplate geo_node_merge_by_distance_in[] = {
    {SOCK_GEOMETRY, N_("Geometry")},
    {SOCK_FLOAT, N_("Distance"), 0.0f, 0, 0, 0, 0, 10000.0f, PROP_DISTANCE},
    {SOCK_STRING, N_("Selection")},
    {-1, ""},
};

static bNodeSocketTemplate geo_node_merge_by_distance_out[] = {
    {SOCK_GEOMETRY, N_("Geometry")},
    {-1, ""},
};

static void geo_node_merge_by_distance_layout(uiLayout *layout,
                                              bContext *UNUSED(C),
                                              PointerRNA *ptr)
{
  uiItemR(layout, ptr, "merge_mode", 0, "", ICON_NONE);
}

static void geo_merge_by_distance_init(bNodeTree *UNUSED(ntree), bNode *node)
{
  node->custom1 = WELD_MODE_ALL;
}

namespace blender::nodes {
static void geo_node_merge_by_distance_exec(GeoNodeExecParams params)
{
  GeometrySet geometry_set = params.extract_input<GeometrySet>("Geometry");
  geometry_set = geometry_set_realize_instances(geometry_set);

  if (geometry_set.has_mesh()) {
    MeshComponent &mesh_component = geometry_set.get_component_for_write<MeshComponent>();
    const Mesh *input_mesh = mesh_component.get_for_read();

    const bool default_selection = true;
    GVArray_Typed<bool> selection_attribute = params.get_input_attribute<bool>(
        "Selection", mesh_component, ATTR_DOMAIN_POINT, default_selection);
    VArray_Span<bool> selection{selection_attribute};

    const float distance = params.extract_input<float>("Distance");

    Mesh *result = GEO_weld(input_mesh, selection.data(), distance, 0);
    if (result != input_mesh) {
      geometry_set.replace_mesh(result);
    }
  }

  params.set_output("Geometry", std::move(geometry_set));
}
}  // namespace blender::nodes

void register_node_type_geo_merge_by_distance()
{
  static bNodeType ntype;
  geo_node_type_base(
      &ntype, GEO_NODE_MERGE_BY_DISTANCE, "Merge By Distance", NODE_CLASS_GEOMETRY, 0);
  node_type_socket_templates(
      &ntype, geo_node_merge_by_distance_in, geo_node_merge_by_distance_out);
  node_type_init(&ntype, geo_merge_by_distance_init);
  ntype.geometry_node_execute = blender::nodes::geo_node_merge_by_distance_exec;
  ntype.draw_buttons = geo_node_merge_by_distance_layout;
  nodeRegisterType(&ntype);
}