// SPDX-FileCopyrightText: © 2023 Tenstorrent Inc.
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <map>
#include <memory>
#include <optional>
#include <vector>

#include "tt_metal/distributed/mesh_device_view.hpp"
#include "tt_metal/impl/device/device.hpp"
#include "tt_metal/impl/sub_device/sub_device_types.hpp"
#include "tt_metal/tt_stl/span.hpp"

namespace tt::tt_metal::distributed {

using DeviceIds = std::vector<int>;
using MeshDeviceID = size_t;
using MeshOffset = std::pair<size_t, size_t>;
class MeshDeviceView;

struct MeshSubDeviceManagerId;

struct MeshDeviceConfig {
    MeshShape mesh_shape;
    MeshOffset offset;
    std::vector<chip_id_t> physical_device_ids;
    MeshType mesh_type;

    MeshDeviceConfig(const MeshShape& mesh_shape, MeshType mesh_type) :
        mesh_shape(mesh_shape),
        offset(MeshOffset{0, 0}),
        physical_device_ids(std::vector<chip_id_t>()),
        mesh_type(mesh_type) {}

    MeshDeviceConfig(
        const MeshShape& mesh_shape,
        const MeshOffset& offset = MeshOffset{0, 0},
        const std::vector<chip_id_t>& physical_device_ids = {},
        MeshType mesh_type = MeshType::RowMajor) :
        mesh_shape(mesh_shape), offset(offset), physical_device_ids(physical_device_ids), mesh_type(mesh_type) {}
};

// SystemMesh creates a virtualization over the physical devices in the system.
// It creates a logical 2D-mesh of devices and manages the mapping between logical and physical device coordinates.
class SystemMesh {
   private:
    friend class MeshDevice;
    class Impl;  // Forward declaration only
    std::unique_ptr<Impl> pimpl_;
    SystemMesh();
    ~SystemMesh();

    std::vector<chip_id_t> request_available_devices(const MeshDeviceConfig& config) const;
    void register_mesh_device(const std::shared_ptr<MeshDevice>& mesh_device, const std::vector<Device*>& devices);

public:
    static SystemMesh& instance();
    SystemMesh(const SystemMesh &) = delete;
    SystemMesh &operator=(const SystemMesh &) = delete;
    SystemMesh(SystemMesh &&) = delete;
    SystemMesh &operator=(SystemMesh &&) = delete;

    // Get the shape of the logical mesh
    const MeshShape& get_shape() const;
    size_t get_num_devices() const;

    // Get the physical device IDs mapped to a MeshDevice
    std::vector<chip_id_t> get_mapped_physical_device_ids(const MeshDeviceConfig &config) const;
};

class MeshDevice : public std::enable_shared_from_this<MeshDevice> {
private:
    MeshDeviceID mesh_id;
    MeshShape mesh_device_shape;
    MeshType type;
    std::shared_ptr<MeshDeviceView> primary_view;
    std::map<chip_id_t, Device*> opened_devices;
    std::vector<Device*> devices;
    std::vector<std::shared_ptr<MeshDevice>> submeshes;  // Parent owns submeshes and responsible fortheir destruction
    std::weak_ptr<MeshDevice> parent_mesh;               // Submesh created with reference to parent mesh

    void initialize(
        size_t l1_small_size,
        size_t trace_region_size,
        size_t num_command_queues,
        const DispatchCoreConfig& dispatch_core_config,
        const MeshDeviceConfig& config);

public:
    MeshDevice(const MeshShape& mesh_device_shape, MeshType type, std::weak_ptr<MeshDevice> parent_mesh = {});
    ~MeshDevice();

    MeshDevice(const MeshDevice&) = delete;
    MeshDevice& operator=(const MeshDevice&) = delete;

    MeshDevice(MeshDevice&&) = delete;
    MeshDevice& operator=(MeshDevice&&) = delete;

    std::vector<Device*> get_devices() const;
    Device* get_device_index(size_t logical_device_id) const;
    Device* get_device(chip_id_t physical_device_id) const;
    Device* get_device(size_t row_idx, size_t col_idx) const;

    const DeviceIds get_device_ids() const;

    size_t num_devices() const;
    size_t num_rows() const;
    size_t num_cols() const;
    MeshShape shape() const;

    CoreCoord compute_with_storage_grid_size() const;

    CoreCoord dram_grid_size() const;

    tt::ARCH arch() const;
    void enable_async(bool enable);
    void enable_program_cache();
    void disable_and_clear_program_cache();

    void close_devices();
    std::shared_ptr<const MeshDeviceView> get_view() const;
    std::shared_ptr<MeshDeviceView> get_view();

    std::string to_string() const;
    MeshDeviceID get_mesh_id() const;
    bool is_parent_mesh() const;

    std::vector<std::shared_ptr<MeshDevice>> get_submeshes() const;

    std::shared_ptr<MeshDevice> create_submesh(
        const MeshShape& submesh_shape,
        const MeshOffset& offset = MeshOffset{0, 0},
        MeshType type = MeshType::RowMajor);

    std::vector<std::shared_ptr<MeshDevice>> create_submeshes(
        const MeshShape& submesh_shape, MeshType type = MeshType::RowMajor);

    size_t num_program_cache_entries() const;

    MeshSubDeviceManagerId create_sub_device_manager(
        tt::stl::Span<const SubDevice> sub_devices, DeviceAddr local_l1_size);
    void load_sub_device_manager(MeshSubDeviceManagerId mesh_sub_device_manager_id);
    void clear_loaded_sub_device_manager();
    void remove_sub_device_manager(MeshSubDeviceManagerId mesh_sub_device_manager_id);

    static std::shared_ptr<MeshDevice> create(
        const MeshDeviceConfig& config,
        size_t l1_small_size = DEFAULT_L1_SMALL_SIZE,
        size_t trace_region_size = DEFAULT_TRACE_REGION_SIZE,
        size_t num_command_queues = 1,
        const DispatchCoreConfig& dispatch_core_config = DispatchCoreConfig{});
};

std::ostream& operator<<(std::ostream& os, const MeshDevice& mesh_device);

// TODO: This will be removed once we have DistributedDevice
// Currently required since each device manages its own sub-device manager ids
struct MeshSubDeviceManagerId {
    MeshSubDeviceManagerId(const MeshDevice& mesh_device);

    std::vector<SubDeviceManagerId> sub_device_manager_ids;
};

}  // namespace tt::tt_metal::distributed
