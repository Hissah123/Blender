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
 *
 * The Original Code is Copyright (C) 2020 Blender Foundation
 * All rights reserved.
 */

#include "BKE_appdir.h"
#include "BKE_asset_catalog.hh"

#include "BLI_fileops.h"

#include "testing/testing.h"

#include <filesystem>

namespace fs = std::filesystem;

namespace blender::bke::tests {

class AssetCatalogTest : public testing::Test {
 protected:
  CatalogFilePath asset_library_root_;

  void SetUp() override
  {
    const fs::path test_files_dir = blender::tests::flags_test_asset_dir();
    if (test_files_dir.empty()) {
      FAIL();
    }

    asset_library_root_ = test_files_dir / "asset_library";
  }
};

TEST_F(AssetCatalogTest, load_single_file)
{
  AssetCatalogService service(asset_library_root_);
  service.load_from_disk(asset_library_root_ / "single_catalog_definition_file.cats.txt");

  // Test getting a non-existant catalog ID.
  EXPECT_EQ(nullptr, service.find_catalog("NONEXISTANT"));

  // Test getting an invalid catalog (without path definition).
  EXPECT_EQ(nullptr, service.find_catalog("ID_WITHOUT_PATH"));

  // Test getting a 7-bit ASCII catalog ID.
  AssetCatalog *poses_elly = service.find_catalog("POSES_ELLY");
  ASSERT_NE(nullptr, poses_elly);
  EXPECT_EQ("POSES_ELLY", poses_elly->catalog_id);
  EXPECT_EQ("character/Elly/poselib", poses_elly->path);

  // Test whitespace stripping and support in the path.
  AssetCatalog *poses_whitespace = service.find_catalog("POSES_ELLY_WHITESPACE");
  ASSERT_NE(nullptr, poses_whitespace);
  EXPECT_EQ("POSES_ELLY_WHITESPACE", poses_whitespace->catalog_id);
  EXPECT_EQ("character/Elly/poselib/white space", poses_whitespace->path);

  // Test getting a UTF-8 catalog ID.
  AssetCatalog *poses_ruzena = service.find_catalog("POSES_RUŽENA");
  ASSERT_NE(nullptr, poses_ruzena);
  EXPECT_EQ("POSES_RUŽENA", poses_ruzena->catalog_id);
  EXPECT_EQ("character/Ružena/poselib", poses_ruzena->path);
}

TEST_F(AssetCatalogTest, write_single_file)
{
  AssetCatalogService service(asset_library_root_);
  service.load_from_disk(asset_library_root_ / "single_catalog_definition_file.cats.txt");

  const CatalogFilePath tempdir = BKE_tempdir_session();
  const CatalogFilePath save_to_path = tempdir / "_asset_catalog_test.cats.txt";
  AssetCatalogDefinitionFile *cdf = service.get_catalog_definition_file();
  cdf->write_to_disk(save_to_path);

  AssetCatalogService loaded_service(save_to_path);
  loaded_service.load_from_disk();

  unlink(save_to_path.c_str());

  // Test that the expected catalogs are there.
  EXPECT_NE(nullptr, loaded_service.find_catalog("POSES_ELLY"));
  EXPECT_NE(nullptr, loaded_service.find_catalog("POSES_ELLY_WHITESPACE"));
  EXPECT_NE(nullptr, loaded_service.find_catalog("POSES_ELLY_TRAILING_SLASH"));
  EXPECT_NE(nullptr, loaded_service.find_catalog("POSES_RUŽENA"));
  EXPECT_NE(nullptr, loaded_service.find_catalog("POSES_RUŽENA_HAND"));
  EXPECT_NE(nullptr, loaded_service.find_catalog("POSES_RUŽENA_FACE"));

  // Test that the invalid catalog definition wasn't copied.
  EXPECT_EQ(nullptr, loaded_service.find_catalog("ID_WITHOUT_PATH"));

  // TODO(@sybren): test ordering of catalogs in the file.
}

}  // namespace blender::bke::tests
