/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <string>

namespace hipFile::test {

// Check AIS capability for tests that attempt to force fast path.
// Reimplements logic from hipfile/tools/ais-check/ais-check.
class AisCapability {
public:
    // allow_skip_on_unavailable: if true, an unavailable fastpath yields Skip
    // instead of Fail.
    explicit AisCapability(bool allow_skip_on_unavailable) : allow_skip{allow_skip_on_unavailable}
    {
    }

    // Described if fastpath tests should fail with a warning message, skip, or run.
    enum class GateDecision {
        Run,
        Skip,
        Fail,
    };

    GateDecision populate();

    std::string report() const;
    std::string skipHint() const;

private:
    void detectKernelAis();
    void detectHipRuntime();
    void detectAmdgpu();

    bool fastpathAvailable() const
    {
        return kernel_ais && hip_runtime && amdgpu;
    }

    bool allow_skip = false;

    bool kernel_ais  = false; // AIS-init bit set on all GPU nodes in KFD topology
    bool hip_runtime = false; // hipAmdFileRead + hipAmdFileWrite resolvable
    bool amdgpu      = false; // kfd_ais_rw_file present in /proc/kallsyms
};

}
