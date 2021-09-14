/*
 * Copyright 2011-2013 Blender Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "render/buffers.h"
#include "util/util_image.h"
#include "util/util_string.h"
#include "util/util_unique_ptr.h"

CCL_NAMESPACE_BEGIN

/* --------------------------------------------------------------------
 * Tile.
 */

class Tile {
 public:
  int x = 0, y = 0;
  int width = 0, height = 0;

  Tile()
  {
  }
};

/* --------------------------------------------------------------------
 * Tile Manager.
 */

class TileManager {
 public:
  TileManager();
  ~TileManager();

  TileManager(const TileManager &other) = delete;
  TileManager(TileManager &&other) noexcept = delete;
  TileManager &operator=(const TileManager &other) = delete;
  TileManager &operator=(TileManager &&other) = delete;

  /* Reset current progress and start new rendering of the full-frame parameters in tiles of the
   * given size. */
  /* TODO(sergey): Consider using tile area instead of exact size to help dealing with extreme
   * cases of stretched renders. */
  void reset(const BufferParams &params, int2 tile_size);

  void update_passes(const BufferParams &params);

  inline int get_num_tiles() const
  {
    return tile_state_.num_tiles;
  }

  inline bool has_multiple_tiles() const
  {
    return tile_state_.num_tiles > 1;
  }

  bool next();
  bool done();

  const Tile &get_current_tile() const;

  /* Write render buffer of a tile to a file on disk.
   *
   * Opens file for write when first tile is written, and closes the file when the last tile has
   * been written.
   *
   * Returns true on success. */
  bool write_tile(const RenderBuffers &tile_buffers);

  /* Inform the tile manager that no more tiles will be written to disk.
   * The file will be considered final, all handles to it will be closed. */
  void finish_write_tiles();

  /* Check whether any tile ahs been written to disk. */
  inline bool has_written_tiles() const
  {
    return write_state_.num_tiles_written != 0;
  }

  /* Read full frame render buffer from tiles file on disk.
   *
   * The render buffer is configured according to the metadata in the file.
   *
   * Returns true on success. */
  bool read_full_buffer_from_disk(RenderBuffers *buffers);

  /* Remove file from disk which holds tiles results. */
  void remove_tile_file() const;

 protected:
  /* Get tile configuration for its index.
   * The tile index must be within [0, state_.tile_state_). */
  Tile get_tile_for_index(int index) const;

  bool open_tile_output();
  bool close_tile_output();

  /* Full file name of a file which holds tile results on disk. */
  string tile_filepath_;

  int2 tile_size_ = make_int2(0, 0);

  BufferParams buffer_params_;

  /* Tile scheduling state. */
  struct {
    int num_tiles_x = 0;
    int num_tiles_y = 0;
    int num_tiles = 0;

    int next_tile_index;

    Tile current_tile;
  } tile_state_;

  /* State of tiles writing to a file on disk. */
  struct {
    /* Specification of the tile image which corresponds to the buffer parameters.
     * Contains channels configured according to the passes configuration in the path traces.
     *
     * Output images are saved using this specification, input images are expected to have matched
     * specification. */
    ImageSpec image_spec;

    /* Output handle for the tile file.
     *
     * This file can not be closed until all tiles has been provided, so the handle is stored in
     * the state and is created whenever writing is requested. */
    unique_ptr<ImageOutput> tile_out;

    int num_tiles_written = 0;
  } write_state_;
};

CCL_NAMESPACE_END
