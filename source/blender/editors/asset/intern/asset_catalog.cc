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

/** \file
 * \ingroup edasset
 */

#include "BKE_asset_catalog.hh"
#include "BKE_asset_library.hh"

#include "BLI_string_utils.h"

#include "ED_asset_catalog.hh"

using namespace blender;
using namespace blender::bke;

struct CatalogUniqueNameFnData {
  const AssetCatalogService &catalog_service;
  StringRef parent_path;
};

static std::string to_full_path(StringRef parent_path, StringRef name)
{
  return parent_path.is_empty() ?
             std::string(name) :
             std::string(parent_path) + AssetCatalogService::PATH_SEPARATOR + name;
}

static bool catalog_name_is_not_unique_fn(void *arg, const char *name)
{
  CatalogUniqueNameFnData &fn_data = *static_cast<CatalogUniqueNameFnData *>(arg);
  std::string fullpath = to_full_path(fn_data.parent_path, name);
  if (fn_data.catalog_service.find_catalog_from_path(fullpath)) {
    return true;
  }
  return false;
}

static std::string catalog_name_ensure_unique(AssetCatalogService &catalog_service,
                                              StringRefNull name,
                                              StringRef parent_path)
{
  CatalogUniqueNameFnData fn_data = {catalog_service, parent_path};

  char unique_name[NAME_MAX] = "";
  BLI_uniquename_cb(catalog_name_is_not_unique_fn,
                    &fn_data,
                    name.c_str(),
                    '.',
                    unique_name,
                    sizeof(unique_name));

  return unique_name;
}

AssetCatalog *ED_asset_catalog_add(blender::bke::AssetLibrary *library,
                                   StringRefNull name,
                                   StringRef parent_path)
{
  if (!library || !library->catalog_service) {
    return nullptr;
  }

  std::string unique_name = catalog_name_ensure_unique(
      *library->catalog_service, name, parent_path);
  std::string fullpath = to_full_path(parent_path, unique_name);

  return library->catalog_service->create_catalog(fullpath);
}

void ED_asset_catalog_remove(blender::bke::AssetLibrary *library, const CatalogID &catalog_id)
{
  if (!library || !library->catalog_service) {
    return;
  }

  library->catalog_service->delete_catalog(catalog_id);
}
