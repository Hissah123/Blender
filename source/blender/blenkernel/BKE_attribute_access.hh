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

#include <mutex>

#include "FN_cpp_type.hh"
#include "FN_generic_span.hh"
#include "FN_generic_virtual_array.hh"

#include "BKE_attribute.h"

#include "BLI_color.hh"
#include "BLI_float2.hh"
#include "BLI_float3.hh"

namespace blender::bke {

using fn::CPPType;
using fn::GVArray;
using fn::GVMutableArray;

const CPPType *custom_data_type_to_cpp_type(const CustomDataType type);
CustomDataType cpp_type_to_custom_data_type(const CPPType &type);
CustomDataType attribute_data_type_highest_complexity(Span<CustomDataType> data_types);
AttributeDomain attribute_domain_highest_priority(Span<AttributeDomain> domains);

struct ReadAttributeLookup {
  std::unique_ptr<GVArray> varray;
  AttributeDomain domain;

  operator bool() const
  {
    return this->varray.get() != nullptr;
  }
};

struct WriteAttributeLookup {
  std::unique_ptr<GVMutableArray> varray;
  AttributeDomain domain;

  operator bool() const
  {
    return this->varray.get() != nullptr;
  }
};

class OutputAttribute {
 public:
  using SaveF = std::function<void(OutputAttribute &)>;

 private:
  std::unique_ptr<GVMutableArray> varray_;
  AttributeDomain domain_;
  SaveF save_;
  std::optional<fn::GVMutableArray_GSpan> optional_span_varray_;

 public:
  OutputAttribute() = default;

  OutputAttribute(std::unique_ptr<GVMutableArray> varray, AttributeDomain domain, SaveF save)
      : varray_(std::move(varray)), domain_(domain), save_(std::move(save))
  {
  }

  operator bool() const
  {
    return varray_.get() != nullptr;
  }

  GVMutableArray &operator*()
  {
    return *varray_;
  }

  GVMutableArray *operator->()
  {
    return varray_.get();
  }

  GVMutableArray &varray()
  {
    return *varray_;
  }

  AttributeDomain domain() const
  {
    return domain_;
  }

  const CPPType &cpp_type() const
  {
    return varray_->type();
  }

  CustomDataType custom_data_type() const
  {
    return cpp_type_to_custom_data_type(this->cpp_type());
  }

  fn::GMutableSpan as_span()
  {
    if (!optional_span_varray_.has_value()) {
      optional_span_varray_.emplace(*varray_);
    }
    fn::GVMutableArray_GSpan &span_varray = *optional_span_varray_;
    return span_varray;
  }

  template<typename T> MutableSpan<T> as_span()
  {
    return this->as_span().typed<T>();
  }

  void save();
};

template<typename T> class OutputAttribute_Typed {
 private:
  OutputAttribute attribute_;
  std::optional<fn::GVMutableArray_Typed<T>> optional_varray_;
  VMutableArray<T> *varray_ = nullptr;

 public:
  OutputAttribute_Typed(OutputAttribute attribute) : attribute_(std::move(attribute))
  {
    if (attribute_) {
      optional_varray_.emplace(attribute_.varray());
      varray_ = &**optional_varray_;
    }
  }

  operator bool() const
  {
    return varray_ != nullptr;
  }

  VMutableArray<T> &operator*()
  {
    return *varray_;
  }

  VMutableArray<T> *operator->()
  {
    return varray_;
  }

  VMutableArray<T> &varray()
  {
    return *varray_;
  }

  AttributeDomain domain() const
  {
    return attribute_.domain();
  }

  const CPPType &cpp_type() const
  {
    return CPPType::get<T>();
  }

  CustomDataType custom_data_type() const
  {
    return cpp_type_to_custom_data_type(this->cpp_type());
  }

  MutableSpan<T> as_span()
  {
    return attribute_.as_span<T>();
  }

  void save()
  {
    attribute_.save();
  }
};

}  // namespace blender::bke
