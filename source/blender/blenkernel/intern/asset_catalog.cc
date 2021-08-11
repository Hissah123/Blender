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
 * \ingroup bke
 */

#include "BKE_asset_catalog.hh"

#include "BLI_string_ref.hh"

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace blender::bke {

const char AssetCatalogService::PATH_SEPARATOR = '/';
const CatalogFilePath AssetCatalogService::DEFAULT_CATALOG_FILENAME =
    "single_catalog_definition_file.cats.txt";

AssetCatalogService::AssetCatalogService(const CatalogFilePath &asset_library_root)
    : asset_library_root_(asset_library_root)
{
}

bool AssetCatalogService::is_empty() const
{
  return catalogs_.is_empty();
}

AssetCatalog *AssetCatalogService::find_catalog(const CatalogID &catalog_id)
{
  std::unique_ptr<AssetCatalog> *catalog_uptr_ptr = this->catalogs_.lookup_ptr(catalog_id);
  if (catalog_uptr_ptr == nullptr) {
    return nullptr;
  }
  return catalog_uptr_ptr->get();
}

void AssetCatalogService::load_from_disk()
{
  load_from_disk(asset_library_root_);
}

void AssetCatalogService::load_from_disk(const CatalogFilePath &asset_library_root)
{
  fs::file_status status = fs::status(asset_library_root);
  switch (status.type()) {
    case fs::file_type::regular:
      load_single_file(asset_library_root);
      break;
    case fs::file_type::directory:
      load_directory_recursive(asset_library_root);
      break;
    default:
      // TODO(@sybren): throw an appropriate exception.
      return;
  }
}

void AssetCatalogService::load_directory_recursive(const CatalogFilePath &directory_path)
{
  // TODO(@sybren): implement proper multi-file support. For now, just load
  // the default file if it is there.
  CatalogFilePath file_path = directory_path / DEFAULT_CATALOG_FILENAME;
  fs::file_status fs_status = fs::status(file_path);

  if (!fs::exists(fs_status)) {
    /* No file to be loaded is perfectly fine. */
    return;
  }
  this->load_single_file(file_path);
}

void AssetCatalogService::load_single_file(const CatalogFilePath &catalog_definition_file_path)
{
  std::unique_ptr<AssetCatalogDefinitionFile> cdf = parse_catalog_file(
      catalog_definition_file_path);

  BLI_assert_msg(!this->catalog_definition_file_,
                 "Only loading of a single catalog definition file is supported.");
  this->catalog_definition_file_ = std::move(cdf);
}

std::unique_ptr<AssetCatalogDefinitionFile> AssetCatalogService::parse_catalog_file(
    const CatalogFilePath &catalog_definition_file_path)
{
  auto cdf = std::make_unique<AssetCatalogDefinitionFile>();
  cdf->file_path = catalog_definition_file_path;

  std::fstream infile(catalog_definition_file_path);
  std::string line;
  while (std::getline(infile, line)) {
    const StringRef trimmed_line = StringRef(line).trim().trim(PATH_SEPARATOR);
    if (trimmed_line.is_empty() || trimmed_line[0] == '#') {
      continue;
    }

    std::unique_ptr<AssetCatalog> catalog = this->parse_catalog_line(trimmed_line, cdf.get());
    if (!catalog) {
      continue;
    }

    if (cdf->contains(catalog->catalog_id)) {
      std::cerr << catalog_definition_file_path << ": multiple definitions of catalog "
                << catalog->catalog_id << " in the same file, using first occurrence."
                << std::endl;
      /* Don't store 'catalog'; unique_ptr will free its memory. */
      continue;
    }

    if (this->catalogs_.contains(catalog->catalog_id)) {
      // TODO(@sybren): apparently another CDF was already loaded. This is not supported yet.
      std::cerr << catalog_definition_file_path << ": multiple definitions of catalog "
                << catalog->catalog_id << " in multiple files, ignoring this one." << std::endl;
      /* Don't store 'catalog'; unique_ptr will free its memory. */
      continue;
    }

    /* The AssetDefinitionFile should include this catalog when writing it back to disk. */
    cdf->add_new(catalog.get());

    /* The AssetCatalog pointer is owned by the AssetCatalogService. */
    this->catalogs_.add_new(catalog->catalog_id, std::move(catalog));
  }

  return cdf;
}

std::unique_ptr<AssetCatalog> AssetCatalogService::parse_catalog_line(
    const StringRef line, const AssetCatalogDefinitionFile *catalog_definition_file)
{
  const int64_t first_space = line.find_first_of(' ');
  if (first_space == StringRef::not_found) {
    std::cerr << "Invalid line in " << catalog_definition_file->file_path << ": " << line
              << std::endl;
    return std::unique_ptr<AssetCatalog>(nullptr);
  }

  const StringRef catalog_id = line.substr(0, first_space);
  const StringRef catalog_path = line.substr(first_space + 1).trim().trim(PATH_SEPARATOR);

  return std::make_unique<AssetCatalog>(catalog_id, catalog_path);
}

AssetCatalogDefinitionFile *AssetCatalogService::get_catalog_definition_file()
{
  return catalog_definition_file_.get();
}

bool AssetCatalogDefinitionFile::contains(const CatalogID &catalog_id) const
{
  return catalogs_.contains(catalog_id);
}

void AssetCatalogDefinitionFile::add_new(AssetCatalog *catalog)
{
  catalogs_.add_new(catalog->catalog_id, catalog);
}

void AssetCatalogDefinitionFile::write_to_disk() const
{
  this->write_to_disk(this->file_path);
}

void AssetCatalogDefinitionFile::write_to_disk(const CatalogFilePath &file_path) const
{
  // TODO(@sybren): create a backup of the original file, if it exists.
  std::ofstream output(file_path);

  // TODO(@sybren): remember the line ending style that was originally read, then use that to write
  // the file again.

  // Write the header.
  // TODO(@sybren): move the header definition to some other place.
  output << "# This is an Asset Catalog Definition file for Blender." << std::endl;
  output << "#" << std::endl;
  output << "# Empty lines and lines starting with `#` will be ignored." << std::endl;
  output << "# Other lines are of the format \"CATALOG_ID /catalog/path/for/assets\"" << std::endl;
  output << "" << std::endl;

  // Write the catalogs.
  // TODO(@sybren): order them by Catalog ID or Catalog Path.
  for (const auto &catalog : catalogs_.values()) {
    output << catalog->catalog_id << " " << catalog->path << std::endl;
  }
}

AssetCatalog::AssetCatalog(const CatalogID &catalog_id, const CatalogPath &path)
    : catalog_id(catalog_id), path(path)
{
}

}  // namespace blender::bke
