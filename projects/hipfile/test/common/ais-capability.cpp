/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "ais-capability.h"

#include "hip.h"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace hipFile::test {

namespace {

    // BIT6 of the KFD topology node "capability" field signals that amdgpu has
    // initialized AIS on that node, implying the kernel supports P2PDMA.
    constexpr uint64_t KFD_AIS_CAPABILITY_BIT = 0x40;

}

void
AisCapability::detectKernelAis()
{
    const std::string topology_nodes = "/sys/class/kfd/kfd/topology/nodes";

    bool any_gpu = false;

    for (int id = 0;; ++id) {
        const std::string props_path = topology_nodes + "/" + std::to_string(id) + "/properties";
        std::ifstream     in{props_path};
        if (!in.is_open()) {
            break;
        }

        uint64_t    capability = 0;
        uint32_t    simd_count = 0;
        std::string key;
        uint64_t    value;
        while (in >> key >> value) {
            if (key == "capability") {
                capability = value;
            }
            else if (key == "simd_count") {
                simd_count = static_cast<uint32_t>(value);
            }
        }

        if (simd_count == 0) {
            continue; // Not a GPU, disregard
        }
        any_gpu = true;
        if ((capability & KFD_AIS_CAPABILITY_BIT) == 0) {
            kernel_ais = false;
            return;
        }
    }

    kernel_ais = any_gpu;
}

void
AisCapability::detectHipRuntime()
{
    hip_runtime = hipFile::getHipAmdFileReadPtr() != nullptr && hipFile::getHipAmdFileWritePtr() != nullptr;
}

void
AisCapability::detectAmdgpu()
{
    std::ifstream kallsyms{"/proc/kallsyms"};
    if (!kallsyms.is_open()) {
        std::cerr << "Unable to open /proc/kallsyms\n";
        amdgpu = false;
        return;
    }

    std::string line;
    while (std::getline(kallsyms, line)) {
        if (line.find("kfd_ais_rw_file") != std::string::npos) {
            amdgpu = true;
            return;
        }
    }
    amdgpu = false;
}

AisCapability::GateDecision
AisCapability::populate()
{
    detectKernelAis();
    detectHipRuntime();
    detectAmdgpu();

    std::cerr << "AIS kernel AIS-init support: " << (kernel_ais ? "yes" : "no") << "\n";
    std::cerr << "AIS HIP runtime support:     " << (hip_runtime ? "yes" : "no") << "\n";
    std::cerr << "AIS amdgpu support:          " << (amdgpu ? "yes" : "no") << "\n";

    if (fastpathAvailable()) {
        return GateDecision::Run;
    }
    return allow_skip ? GateDecision::Skip : GateDecision::Fail;
}

std::string
AisCapability::report() const
{
    auto pass_fail = [](bool ok) { return ok ? "Pass" : "Fail"; };

    std::ostringstream os;
    os << "Fastpath Validation:\n";
    os << "  HIP Runtime Check:  " << pass_fail(hip_runtime) << "\n";
    os << "  amdgpu Check:       " << pass_fail(amdgpu) << "\n";
    os << "  AIS init check:     " << pass_fail(kernel_ais);

    return os.str();
}

std::string
AisCapability::skipHint() const
{
    return "To skip these tests instead of failing, configure the build with "
           "-DHIPFILE_ALLOW_SKIP_FASTPATH_TESTS=ON "
           "(passes --allow-skip-fastpath to the system test binary).";
}

}
