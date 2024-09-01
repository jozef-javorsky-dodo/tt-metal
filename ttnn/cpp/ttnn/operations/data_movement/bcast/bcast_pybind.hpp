// SPDX-FileCopyrightText: © 2024 Tenstorrent Inc.
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "pybind11/pybind_fwd.hpp"

namespace ttnn::operations::data_movement::detail {
namespace py = pybind11;
void py_bind_bcast(py::module& module);

}  // namespace ttnn::operations::data_movement::detail