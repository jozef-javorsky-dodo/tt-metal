// SPDX-FileCopyrightText: © 2024 Tenstorrent Inc.
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <memory>
#include "tt_metal/host_api.hpp"
#include "ttnn/types.hpp"

namespace ttnn::global_semaphore {

struct MultiDeviceGlobalSemaphore {
    MultiDeviceGlobalSemaphore(MeshDevice* mesh_device);
    std::vector<std::shared_ptr<GlobalSemaphore>> global_semaphores;
};

// Single Device APIs
std::shared_ptr<GlobalSemaphore> create_global_semaphore(
    Device* device, const CoreRangeSet& cores, uint32_t initial_value, BufferType buffer_type = BufferType::L1);
DeviceAddr get_global_semaphore_address(const std::shared_ptr<GlobalSemaphore>& global_semaphore);
void reset_global_semaphore_value(const std::shared_ptr<GlobalSemaphore>& global_semaphore);

// Multi Device APIs
MultiDeviceGlobalSemaphore create_global_semaphore(
    MeshDevice* mesh_device,
    const CoreRangeSet& cores,
    uint32_t initial_value,
    BufferType buffer_type = BufferType::L1);
std::vector<DeviceAddr> get_global_semaphore_address(const MultiDeviceGlobalSemaphore& global_semaphore);
void reset_global_semaphore_value(const MultiDeviceGlobalSemaphore& global_semaphore);

}  // namespace ttnn::global_semaphore
