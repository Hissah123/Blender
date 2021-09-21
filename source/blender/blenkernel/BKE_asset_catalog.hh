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

#pragma once

#ifndef __cplusplus
#  error This is a C++ header. The C interface is yet to be implemented/designed.
#endif

#include "BLI_filesystem.hh"
#include "BLI_function_ref.hh"
#include "BLI_map.hh"
#include "BLI_string_ref.hh"
#include "BLI_uuid.h"
#include "BLI_vector.hh"

#include <map>
#include <memory>
#include <string>

namespace blender::bke {

using CatalogID = UUID;
using CatalogPath = std::string;
using CatalogPathComponent = std::string;
using CatalogFilePath = filesystem::path;

class AssetCatalog;
class AssetCatalogDefinitionFile;
class AssetCatalogTree;

/* Manages the asset catalogs of a single asset library (i.e. of catalogs defined in a single
 * directory hierarchy). */
class AssetCatalogService {
 public:
  static const char PATH_SEPARATOR;
  static const CatalogFilePath DEFAULT_CATALOG_FILENAME;

 public:
  explicit AssetCatalogService(const CatalogFilePath &asset_library_root);

  /** Load asset catalog definitions from the files found in the asset library. */
  void load_from_disk();
  /** Load asset catalog definitions from the given file or directory. */
  void load_from_disk(const CatalogFilePath &file_or_directory_path);

  /** Return catalog with the given ID. Return nullptr if not found. */
  AssetCatalog *find_catalog(CatalogID catalog_id);

  /** Create a catalog with some sensible auto-generated catalog ID.
   * The catalog will be saved to the default catalog file.*/
  AssetCatalog *create_catalog(const CatalogPath &catalog_path);


  AssetCatalogTree *get_catalog_tree();

  /** Return true iff there are no catalogs known. */
  bool is_empty() const;

 protected:
  /* These pointers are owned by this AssetCatalogService. */
  Map<CatalogID, std::unique_ptr<AssetCatalog>> catalogs_;
  std::unique_ptr<AssetCatalogDefinitionFile> catalog_definition_file_;
  std::unique_ptr<AssetCatalogTree> catalog_tree_;
  CatalogFilePath asset_library_root_;

  void load_directory_recursive(const CatalogFilePath &directory_path);
  void load_single_file(const CatalogFilePath &catalog_definition_file_path);

  std::unique_ptr<AssetCatalogDefinitionFile> parse_catalog_file(
      const CatalogFilePath &catalog_definition_file_path);

  std::unique_ptr<AssetCatalog> parse_catalog_line(
      StringRef line, const AssetCatalogDefinitionFile *catalog_definition_file);

  /**
   * Ensure that an #AssetCatalogDefinitionFile exists in memory.
   * This is used when no such file has been loaded, and a new catalog is to be created. */
  void ensure_catalog_definition_file();

  /**
   * Ensure the asset library root directory exists, so that it can be written to.
   * TODO(@sybren): this might move to the #AssetLibrary class instead, and just assumed to exist
   * in this class. */
  bool ensure_asset_library_root();

  std::unique_ptr<AssetCatalogTree> read_into_tree();
};

class AssetCatalogTreeItem {
  friend class AssetCatalogService;

 public:
  using ChildMap = std::map<std::string, AssetCatalogTreeItem>;
  using ItemIterFn = FunctionRef<void(const AssetCatalogTreeItem &)>;

  AssetCatalogTreeItem(StringRef name, const AssetCatalogTreeItem *parent = nullptr);

  StringRef get_name() const;
  /** Return the full catalog path, defined as the name of this catalog prefixed by the full
   * catalog path of its parent and a separator. */
  CatalogPath catalog_path() const;
  int count_parents() const;

  static void foreach_item_recursive(const ChildMap &children_, const ItemIterFn callback);

 protected:
  /** Child tree items, ordered by their names. */
  ChildMap children_;
  /** The user visible name of this component. */
  CatalogPathComponent name_;

  /** Pointer back to the parent item. Used to reconstruct the hierarchy from an item (e.g. to
   * build a path). */
  const AssetCatalogTreeItem *parent_ = nullptr;
};

/**
 * A representation of the catalog paths as tree structure. Each component of the catalog tree is
 * represented by a #AssetCatalogTreeItem.
 * There is no single root tree element, the #AssetCatalogTree instance itself represents the root.
 */
class AssetCatalogTree {
  friend class AssetCatalogService;

 public:
  void foreach_item(const AssetCatalogTreeItem::ItemIterFn callback) const;

 protected:
  /** Child tree items, ordered by their names. */
  AssetCatalogTreeItem::ChildMap children_;
};

/** Keeps track of which catalogs are defined in a certain file on disk.
 * Only contains non-owning pointers to the #AssetCatalog instances, so ensure the lifetime of this
 * class is shorter than that of the #`AssetCatalog`s themselves. */
class AssetCatalogDefinitionFile {
 public:
  CatalogFilePath file_path;

  AssetCatalogDefinitionFile() = default;

  /** Write the catalog definitions to the same file they were read from. */
  void write_to_disk() const;
  /** Write the catalog definitions to an arbitrary file path. */
  void write_to_disk(const CatalogFilePath &) const;

  bool contains(CatalogID catalog_id) const;
  /* Add a new catalog. Undefined behaviour if a catalog with the same ID was already added. */
  void add_new(AssetCatalog *catalog);

 protected:
  /* Catalogs stored in this file. They are mapped by ID to make it possible to query whether a
   * catalog is already known, without having to find the corresponding `AssetCatalog*`. */
  Map<CatalogID, AssetCatalog *> catalogs_;
};

/** Asset Catalog definition, containing a symbolic ID and a path that points to a node in the
 * catalog hierarchy. */
class AssetCatalog {
 public:
  AssetCatalog() = default;
  AssetCatalog(CatalogID catalog_id, const CatalogPath &path, const std::string &simple_name);

  CatalogID catalog_id;
  CatalogPath path;
  /**
   * Simple, human-readable name for the asset catalog. This is stored on assets alongside the
   * catalog ID; the catalog ID is a UUID that is not human-readable, so to avoid complete dataloss
   * when the catalog definition file gets lost, we also store a human-readable simple name for the
   * catalog. */
  std::string simple_name;

  /**
   * Create a new Catalog with the given path, auto-generating a sensible catalog simplename.
   *
   * NOTE: the given path will be cleaned up (trailing spaces removed, etc.), so the returned
   * `AssetCatalog`'s path differ from the given one.
   */
  static std::unique_ptr<AssetCatalog> from_path(const CatalogPath &path);
  static CatalogPath cleanup_path(const CatalogPath &path);

 protected:
  /** Generate a sensible catalog ID for the given path. */
  static std::string sensible_simple_name_for_path(const CatalogPath &path);
};

}  // namespace blender::bke
