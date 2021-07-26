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

#pragma once

/** \file
 * \ingroup bli
 *
 * msgpack implementation for `blender::Vector<T>`
 */

#include "BLI_vector.hh"

#include "msgpack.hpp"

namespace msgpack {
MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS)
{
  namespace adaptor {

  template<typename T> struct pack<blender::Vector<T>> {
    template<typename Stream>
    msgpack::packer<Stream> &operator()(msgpack::packer<Stream> &o,
                                        const blender::Vector<T> &v) const
    {
      uint32_t size = checked_get_container_size(v.size());
      o.pack_array(size);
      for (const auto &val : v) {
        o.pack(val);
      }
      return o;
    }
  };

  }  // namespace adaptor
}  // MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS)
}  // namespace msgpack
