/*
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "shared/source/device_binary_format/patchtokens_decoder.h"
#include "shared/test/unit_test/device_binary_format/patchtokens_tests.h"
#include "shared/test/unit_test/helpers/debug_manager_state_restore.h"
#include "shared/test/unit_test/mocks/mock_device.h"
#include "shared/test/unit_test/mocks/mock_graphics_allocation.h"

#include "opencl/source/program/kernel_info.h"
#include "opencl/source/program/kernel_info_from_patchtokens.h"
#include "test.h"

#include "level_zero/core/source/debugger/debugger_l0.h"
#include "level_zero/core/source/image/image_format_desc_helper.h"
#include "level_zero/core/source/image/image_hw.h"
#include "level_zero/core/source/kernel/kernel_hw.h"
#include "level_zero/core/source/module/module_imp.h"
#include "level_zero/core/source/sampler/sampler_hw.h"
#include "level_zero/core/test/unit_tests/fixtures/device_fixture.h"
#include "level_zero/core/test/unit_tests/fixtures/module_fixture.h"
#include "level_zero/core/test/unit_tests/mocks/mock_device.h"
#include "level_zero/core/test/unit_tests/mocks/mock_kernel.h"
#include "level_zero/core/test/unit_tests/mocks/mock_module.h"

void NEO::populateKernelDescriptor(KernelDescriptor &dst, const PatchTokenBinary::KernelFromPatchtokens &src, uint32_t gpuPointerSizeInBytes);

namespace L0 {
namespace ult {

using KernelImpSetGroupSizeTest = Test<DeviceFixture>;

HWTEST_F(KernelImpSetGroupSizeTest, WhenCalculatingLocalIdsThenGrfSizeIsTakenFromCapabilityTable) {
    Mock<Kernel> mockKernel;
    Mock<Module> mockModule(this->device, nullptr);
    mockKernel.descriptor.kernelAttributes.simdSize = 1;
    mockKernel.module = &mockModule;
    auto grfSize = mockModule.getDevice()->getHwInfo().capabilityTable.grfSize;
    uint32_t groupSize[3] = {2, 3, 5};
    auto ret = mockKernel.setGroupSize(groupSize[0], groupSize[1], groupSize[2]);
    EXPECT_EQ(ZE_RESULT_SUCCESS, ret);
    EXPECT_EQ(groupSize[0] * groupSize[1] * groupSize[2], mockKernel.numThreadsPerThreadGroup);
    EXPECT_EQ(grfSize * groupSize[0] * groupSize[1] * groupSize[2], mockKernel.perThreadDataSizeForWholeThreadGroup);
    ASSERT_LE(grfSize * groupSize[0] * groupSize[1] * groupSize[2], mockKernel.perThreadDataSizeForWholeThreadGroup);
    using LocalIdT = unsigned short;
    auto threadOffsetInLocalIds = grfSize / sizeof(LocalIdT);
    auto generatedLocalIds = reinterpret_cast<LocalIdT *>(mockKernel.perThreadDataForWholeThreadGroup);

    uint32_t threadId = 0;
    for (uint32_t z = 0; z < groupSize[2]; ++z) {
        for (uint32_t y = 0; y < groupSize[1]; ++y) {
            for (uint32_t x = 0; x < groupSize[0]; ++x) {
                EXPECT_EQ(x, generatedLocalIds[0 + threadId * threadOffsetInLocalIds]) << " thread : " << threadId;
                EXPECT_EQ(y, generatedLocalIds[1 + threadId * threadOffsetInLocalIds]) << " thread : " << threadId;
                EXPECT_EQ(z, generatedLocalIds[2 + threadId * threadOffsetInLocalIds]) << " thread : " << threadId;
                ++threadId;
            }
        }
    }
}

HWTEST_F(KernelImpSetGroupSizeTest, givenLocalIdGenerationByRuntimeDisabledWhenSettingGroupSizeThenLocalIdsAreNotGenerated) {
    Mock<Kernel> mockKernel;
    Mock<Module> mockModule(this->device, nullptr);
    mockKernel.descriptor.kernelAttributes.simdSize = 1;
    mockKernel.module = &mockModule;
    mockKernel.kernelRequiresGenerationOfLocalIdsByRuntime = false;

    uint32_t groupSize[3] = {2, 3, 5};
    auto ret = mockKernel.setGroupSize(groupSize[0], groupSize[1], groupSize[2]);
    EXPECT_EQ(ZE_RESULT_SUCCESS, ret);
    EXPECT_EQ(groupSize[0] * groupSize[1] * groupSize[2], mockKernel.numThreadsPerThreadGroup);
    EXPECT_EQ(0u, mockKernel.perThreadDataSizeForWholeThreadGroup);
    EXPECT_EQ(0u, mockKernel.perThreadDataSize);
    EXPECT_EQ(nullptr, mockKernel.perThreadDataForWholeThreadGroup);
}

using SetKernelArg = Test<ModuleFixture>;
using ImageSupport = IsWithinProducts<IGFX_SKYLAKE, IGFX_TIGERLAKE_LP>;

HWTEST2_F(SetKernelArg, givenImageAndKernelWhenSetArgImageThenCrossThreadDataIsSet, ImageSupport) {
    createKernel();

    auto &imageArg = const_cast<NEO::ArgDescImage &>(kernel->kernelImmData->getDescriptor().payloadMappings.explicitArgs[3].as<NEO::ArgDescImage>());
    imageArg.metadataPayload.imgWidth = 0x0;
    imageArg.metadataPayload.imgHeight = 0x8;
    imageArg.metadataPayload.imgDepth = 0x10;

    imageArg.metadataPayload.arraySize = 0x18;
    imageArg.metadataPayload.numSamples = 0x1c;
    imageArg.metadataPayload.channelDataType = 0x20;
    imageArg.metadataPayload.channelOrder = 0x24;
    imageArg.metadataPayload.numMipLevels = 0x28;

    imageArg.metadataPayload.flatWidth = 0x30;
    imageArg.metadataPayload.flatHeight = 0x38;
    imageArg.metadataPayload.flatPitch = 0x40;
    imageArg.metadataPayload.flatBaseOffset = 0x48;

    ze_image_desc_t desc = {};

    desc.type = ZE_IMAGE_TYPE_3D;
    desc.format.layout = ZE_IMAGE_FORMAT_LAYOUT_8_8_8_8;
    desc.format.type = ZE_IMAGE_FORMAT_TYPE_UINT;
    desc.width = 11;
    desc.height = 13;
    desc.depth = 17;

    desc.format.x = ZE_IMAGE_FORMAT_SWIZZLE_A;
    desc.format.y = ZE_IMAGE_FORMAT_SWIZZLE_0;
    desc.format.z = ZE_IMAGE_FORMAT_SWIZZLE_1;
    desc.format.w = ZE_IMAGE_FORMAT_SWIZZLE_X;

    auto imageHW = std::make_unique<WhiteBox<::L0::ImageCoreFamily<gfxCoreFamily>>>();
    auto ret = imageHW->initialize(device, &desc);
    ASSERT_EQ(ZE_RESULT_SUCCESS, ret);

    auto handle = imageHW->toHandle();
    auto imgInfo = imageHW->getImageInfo();
    auto pixelSize = imgInfo.surfaceFormat->ImageElementSizeInBytes;

    kernel->setArgImage(3, sizeof(imageHW.get()), &handle);

    auto crossThreadData = kernel->getCrossThreadData();

    auto pImgWidth = ptrOffset(crossThreadData, imageArg.metadataPayload.imgWidth);
    EXPECT_EQ(imgInfo.imgDesc.imageWidth, *pImgWidth);

    auto pImgHeight = ptrOffset(crossThreadData, imageArg.metadataPayload.imgHeight);
    EXPECT_EQ(imgInfo.imgDesc.imageHeight, *pImgHeight);

    auto pImgDepth = ptrOffset(crossThreadData, imageArg.metadataPayload.imgDepth);
    EXPECT_EQ(imgInfo.imgDesc.imageDepth, *pImgDepth);

    auto pArraySize = ptrOffset(crossThreadData, imageArg.metadataPayload.arraySize);
    EXPECT_EQ(imgInfo.imgDesc.imageArraySize, *pArraySize);

    auto pNumSamples = ptrOffset(crossThreadData, imageArg.metadataPayload.numSamples);
    EXPECT_EQ(imgInfo.imgDesc.numSamples, *pNumSamples);

    auto pNumMipLevels = ptrOffset(crossThreadData, imageArg.metadataPayload.numMipLevels);
    EXPECT_EQ(imgInfo.imgDesc.numMipLevels, *pNumMipLevels);

    auto pFlatBaseOffset = ptrOffset(crossThreadData, imageArg.metadataPayload.flatBaseOffset);
    EXPECT_EQ(imageHW->getAllocation()->getGpuAddress(), *reinterpret_cast<const uint64_t *>(pFlatBaseOffset));

    auto pFlatWidth = ptrOffset(crossThreadData, imageArg.metadataPayload.flatWidth);
    EXPECT_EQ((imgInfo.imgDesc.imageWidth * pixelSize) - 1u, *pFlatWidth);

    auto pFlatHeight = ptrOffset(crossThreadData, imageArg.metadataPayload.flatHeight);
    EXPECT_EQ((imgInfo.imgDesc.imageHeight * pixelSize) - 1u, *pFlatHeight);

    auto pFlatPitch = ptrOffset(crossThreadData, imageArg.metadataPayload.flatPitch);
    EXPECT_EQ(imgInfo.imgDesc.imageRowPitch - 1u, *pFlatPitch);

    auto pChannelDataType = ptrOffset(crossThreadData, imageArg.metadataPayload.channelDataType);
    EXPECT_EQ(getClChannelDataType(desc.format), *reinterpret_cast<const cl_channel_type *>(pChannelDataType));

    auto pChannelOrder = ptrOffset(crossThreadData, imageArg.metadataPayload.channelOrder);
    EXPECT_EQ(getClChannelOrder(desc.format), *reinterpret_cast<const cl_channel_order *>(pChannelOrder));
}

HWTEST2_F(SetKernelArg, givenSamplerAndKernelWhenSetArgSamplerThenCrossThreadDataIsSet, ImageSupport) {
    createKernel();

    auto &samplerArg = const_cast<NEO::ArgDescSampler &>(kernel->kernelImmData->getDescriptor().payloadMappings.explicitArgs[5].as<NEO::ArgDescSampler>());
    samplerArg.metadataPayload.samplerAddressingMode = 0x0;
    samplerArg.metadataPayload.samplerNormalizedCoords = 0x4;
    samplerArg.metadataPayload.samplerSnapWa = 0x8;

    ze_sampler_desc_t desc = {};

    desc.addressMode = ZE_SAMPLER_ADDRESS_MODE_CLAMP;
    desc.filterMode = ZE_SAMPLER_FILTER_MODE_NEAREST;
    desc.isNormalized = true;

    auto sampler = std::make_unique<WhiteBox<::L0::SamplerCoreFamily<gfxCoreFamily>>>();

    auto ret = sampler->initialize(device, &desc);
    ASSERT_EQ(ZE_RESULT_SUCCESS, ret);

    auto handle = sampler->toHandle();

    kernel->setArgSampler(5, sizeof(sampler.get()), &handle);

    auto crossThreadData = kernel->getCrossThreadData();

    auto pSamplerSnapWa = ptrOffset(crossThreadData, samplerArg.metadataPayload.samplerSnapWa);
    EXPECT_EQ(std::numeric_limits<uint32_t>::max(), *reinterpret_cast<const uint32_t *>(pSamplerSnapWa));

    auto pSamplerAddressingMode = ptrOffset(crossThreadData, samplerArg.metadataPayload.samplerAddressingMode);
    EXPECT_EQ(0x01, *pSamplerAddressingMode);

    auto pSamplerNormalizedCoords = ptrOffset(crossThreadData, samplerArg.metadataPayload.samplerNormalizedCoords);
    EXPECT_EQ(0x08, *pSamplerNormalizedCoords);
}

using ArgSupport = IsWithinProducts<IGFX_SKYLAKE, IGFX_TIGERLAKE_LP>;

HWTEST2_F(SetKernelArg, givenBufferArgumentWhichHasNotBeenAllocatedByRuntimeThenInvalidArgumentIsReturned, ArgSupport) {
    createKernel();

    uint64_t hostAddress = 0x1234;

    ze_result_t res = kernel->setArgBuffer(0, sizeof(hostAddress), &hostAddress);

    EXPECT_EQ(ZE_RESULT_ERROR_INVALID_ARGUMENT, res);
}

class KernelPropertiesTests : public ModuleFixture, public ::testing::Test {
  public:
    void SetUp() override {
        ModuleFixture::SetUp();

        ze_kernel_desc_t kernelDesc = {};
        kernelDesc.pKernelName = kernelName.c_str();

        ze_result_t res = module->createKernel(&kernelDesc, &kernelHandle);
        EXPECT_EQ(ZE_RESULT_SUCCESS, res);

        kernel = L0::Kernel::fromHandle(kernelHandle);
    }

    void TearDown() override {
        Kernel::fromHandle(kernelHandle)->destroy();
        ModuleFixture::TearDown();
    }

    ze_kernel_handle_t kernelHandle;
    L0::Kernel *kernel = nullptr;
};

HWTEST_F(KernelPropertiesTests, givenKernelThenCorrectNameIsRetrieved) {
    size_t kernelSize = 0;
    ze_result_t res = kernel->getKernelName(&kernelSize, nullptr);
    EXPECT_EQ(ZE_RESULT_SUCCESS, res);
    EXPECT_EQ(kernelSize, kernelName.length() + 1);

    size_t alteredKernelSize = kernelSize * 2;
    res = kernel->getKernelName(&alteredKernelSize, nullptr);
    EXPECT_EQ(ZE_RESULT_SUCCESS, res);
    EXPECT_EQ(alteredKernelSize, kernelSize);

    char *kernelNameRetrieved = new char[kernelSize];
    res = kernel->getKernelName(&kernelSize, kernelNameRetrieved);
    EXPECT_EQ(ZE_RESULT_SUCCESS, res);

    EXPECT_EQ(0, strncmp(kernelName.c_str(), kernelNameRetrieved, kernelSize));

    delete[] kernelNameRetrieved;
}

HWTEST_F(KernelPropertiesTests, whenInitializingThenCalculatesProperPrivateSurfaceSize) {
    uint32_t computeUnitsUsedForSratch = 0x300;

    KernelInfo kernelInfo;
    auto &kernelAttributes = kernelInfo.kernelDescriptor.kernelAttributes;
    kernelAttributes.perHwThreadPrivateMemorySize = 0x100;
    kernelAttributes.simdSize = 8;

    KernelImmutableData kernelImmutableData(device);
    kernelImmutableData.initialize(&kernelInfo, device, computeUnitsUsedForSratch, nullptr, nullptr, false);

    size_t expectedSize = static_cast<size_t>(kernelAttributes.perHwThreadPrivateMemorySize) * computeUnitsUsedForSratch;
    EXPECT_GE(expectedSize, kernelImmutableData.getPrivateMemoryGraphicsAllocation()->getUnderlyingBufferSize());
}

HWTEST_F(KernelPropertiesTests, givenValidKernelThenPropertiesAreRetrieved) {
    ze_kernel_properties_t kernelProperties = {};

    kernelProperties.requiredNumSubGroups = std::numeric_limits<uint32_t>::max();
    kernelProperties.requiredSubgroupSize = std::numeric_limits<uint32_t>::max();
    kernelProperties.maxSubgroupSize = std::numeric_limits<uint32_t>::max();
    kernelProperties.maxNumSubgroups = std::numeric_limits<uint32_t>::max();
    kernelProperties.localMemSize = std::numeric_limits<uint32_t>::max();
    kernelProperties.privateMemSize = std::numeric_limits<uint32_t>::max();
    kernelProperties.spillMemSize = std::numeric_limits<uint32_t>::max();
    kernelProperties.numKernelArgs = std::numeric_limits<uint32_t>::max();
    memset(&kernelProperties.uuid.kid, std::numeric_limits<int>::max(),
           sizeof(kernelProperties.uuid.kid));
    memset(&kernelProperties.uuid.mid, std::numeric_limits<int>::max(),
           sizeof(kernelProperties.uuid.mid));

    ze_kernel_properties_t kernelPropertiesBefore = {};
    kernelPropertiesBefore = kernelProperties;

    ze_result_t res = kernel->getProperties(&kernelProperties);
    EXPECT_EQ(ZE_RESULT_SUCCESS, res);

    EXPECT_EQ(6U, kernelProperties.numKernelArgs);

    EXPECT_EQ(0U, kernelProperties.requiredNumSubGroups);
    EXPECT_EQ(0U, kernelProperties.requiredSubgroupSize);

    uint32_t maxSubgroupSize = this->kernel->getKernelDescriptor().kernelAttributes.simdSize;
    ASSERT_NE(0U, maxSubgroupSize);
    EXPECT_EQ(maxSubgroupSize, kernelProperties.maxSubgroupSize);

    uint32_t maxKernelWorkGroupSize = static_cast<uint32_t>(this->module->getDevice()->getNEODevice()->getDeviceInfo().maxWorkGroupSize);
    uint32_t maxNumSubgroups = maxKernelWorkGroupSize / maxSubgroupSize;
    EXPECT_EQ(maxNumSubgroups, kernelProperties.maxNumSubgroups);

    EXPECT_EQ(sizeof(float) * 16U, kernelProperties.localMemSize);
    EXPECT_EQ(0U, kernelProperties.privateMemSize);
    EXPECT_EQ(0U, kernelProperties.spillMemSize);

    uint8_t zeroKid[ZE_MAX_KERNEL_UUID_SIZE];
    uint8_t zeroMid[ZE_MAX_MODULE_UUID_SIZE];
    memset(&zeroKid, 0, ZE_MAX_KERNEL_UUID_SIZE);
    memset(&zeroMid, 0, ZE_MAX_MODULE_UUID_SIZE);
    EXPECT_EQ(0, memcmp(&kernelProperties.uuid.kid, &zeroKid,
                        sizeof(kernelProperties.uuid.kid)));
    EXPECT_EQ(0, memcmp(&kernelProperties.uuid.mid, &zeroMid,
                        sizeof(kernelProperties.uuid.mid)));
}

HWTEST_F(KernelPropertiesTests, givenValidKernelIndirectAccessFlagsThenFlagsSetCorrectly) {
    UnifiedMemoryControls unifiedMemoryControls = kernel->getUnifiedMemoryControls();
    EXPECT_EQ(false, unifiedMemoryControls.indirectDeviceAllocationsAllowed);
    EXPECT_EQ(false, unifiedMemoryControls.indirectHostAllocationsAllowed);
    EXPECT_EQ(false, unifiedMemoryControls.indirectSharedAllocationsAllowed);

    ze_kernel_indirect_access_flags_t flags = ZE_KERNEL_INDIRECT_ACCESS_FLAG_DEVICE |
                                              ZE_KERNEL_INDIRECT_ACCESS_FLAG_HOST |
                                              ZE_KERNEL_INDIRECT_ACCESS_FLAG_SHARED;
    auto res = kernel->setIndirectAccess(flags);
    EXPECT_EQ(ZE_RESULT_SUCCESS, res);

    unifiedMemoryControls = kernel->getUnifiedMemoryControls();
    EXPECT_EQ(true, unifiedMemoryControls.indirectDeviceAllocationsAllowed);
    EXPECT_EQ(true, unifiedMemoryControls.indirectHostAllocationsAllowed);
    EXPECT_EQ(true, unifiedMemoryControls.indirectSharedAllocationsAllowed);
}

HWTEST_F(KernelPropertiesTests, givenValidKernelWithIndirectAccessFlagsAndDisableIndirectAccessSetToZeroThenFlagsAreSet) {
    DebugManagerStateRestore restorer;
    NEO::DebugManager.flags.DisableIndirectAccess.set(0);

    UnifiedMemoryControls unifiedMemoryControls = kernel->getUnifiedMemoryControls();
    EXPECT_EQ(false, unifiedMemoryControls.indirectDeviceAllocationsAllowed);
    EXPECT_EQ(false, unifiedMemoryControls.indirectHostAllocationsAllowed);
    EXPECT_EQ(false, unifiedMemoryControls.indirectSharedAllocationsAllowed);

    ze_kernel_indirect_access_flags_t flags = ZE_KERNEL_INDIRECT_ACCESS_FLAG_DEVICE |
                                              ZE_KERNEL_INDIRECT_ACCESS_FLAG_HOST |
                                              ZE_KERNEL_INDIRECT_ACCESS_FLAG_SHARED;
    auto res = kernel->setIndirectAccess(flags);
    EXPECT_EQ(ZE_RESULT_SUCCESS, res);

    unifiedMemoryControls = kernel->getUnifiedMemoryControls();
    EXPECT_TRUE(unifiedMemoryControls.indirectDeviceAllocationsAllowed);
    EXPECT_TRUE(unifiedMemoryControls.indirectHostAllocationsAllowed);
    EXPECT_TRUE(unifiedMemoryControls.indirectSharedAllocationsAllowed);
}

HWTEST_F(KernelPropertiesTests, givenValidKernelWithIndirectAccessFlagsAndDisableIndirectAccessSetToOneThenFlagsAreNotSet) {
    DebugManagerStateRestore restorer;
    NEO::DebugManager.flags.DisableIndirectAccess.set(1);

    UnifiedMemoryControls unifiedMemoryControls = kernel->getUnifiedMemoryControls();
    EXPECT_EQ(false, unifiedMemoryControls.indirectDeviceAllocationsAllowed);
    EXPECT_EQ(false, unifiedMemoryControls.indirectHostAllocationsAllowed);
    EXPECT_EQ(false, unifiedMemoryControls.indirectSharedAllocationsAllowed);

    ze_kernel_indirect_access_flags_t flags = ZE_KERNEL_INDIRECT_ACCESS_FLAG_DEVICE |
                                              ZE_KERNEL_INDIRECT_ACCESS_FLAG_HOST |
                                              ZE_KERNEL_INDIRECT_ACCESS_FLAG_SHARED;
    auto res = kernel->setIndirectAccess(flags);
    EXPECT_EQ(ZE_RESULT_SUCCESS, res);

    unifiedMemoryControls = kernel->getUnifiedMemoryControls();
    EXPECT_FALSE(unifiedMemoryControls.indirectDeviceAllocationsAllowed);
    EXPECT_FALSE(unifiedMemoryControls.indirectHostAllocationsAllowed);
    EXPECT_FALSE(unifiedMemoryControls.indirectSharedAllocationsAllowed);
}

HWTEST_F(KernelPropertiesTests, givenValidKernelIndirectAccessFlagsSetThenExpectKernelIndirectAllocationsAllowedTrue) {
    EXPECT_EQ(false, kernel->hasIndirectAllocationsAllowed());

    ze_kernel_indirect_access_flags_t flags = ZE_KERNEL_INDIRECT_ACCESS_FLAG_DEVICE;
    auto res = kernel->setIndirectAccess(flags);

    EXPECT_EQ(ZE_RESULT_SUCCESS, res);
    EXPECT_EQ(true, kernel->hasIndirectAllocationsAllowed());
}

HWTEST_F(KernelPropertiesTests, givenValidKernelAndNoMediavfestateThenSpillMemSizeIsZero) {
    ze_kernel_properties_t kernelProperties = {};

    kernelProperties.spillMemSize = std::numeric_limits<uint32_t>::max();

    ze_kernel_properties_t kernelPropertiesBefore = {};
    kernelPropertiesBefore = kernelProperties;

    ze_result_t res = kernel->getProperties(&kernelProperties);
    EXPECT_EQ(ZE_RESULT_SUCCESS, res);

    L0::ModuleImp *moduleImp = reinterpret_cast<L0::ModuleImp *>(module.get());
    NEO::KernelInfo *ki = nullptr;
    for (uint32_t i = 0; i < moduleImp->getTranslationUnit()->programInfo.kernelInfos.size(); i++) {
        ki = moduleImp->getTranslationUnit()->programInfo.kernelInfos[i];
        if (ki->kernelDescriptor.kernelMetadata.kernelName.compare(0, ki->kernelDescriptor.kernelMetadata.kernelName.size(), kernel->getImmutableData()->getDescriptor().kernelMetadata.kernelName) == 0) {
            break;
        }
    }

    ki->patchInfo.mediavfestate = nullptr;
    EXPECT_EQ(0u, kernelProperties.spillMemSize);
}

HWTEST_F(KernelPropertiesTests, givenValidKernelAndNollocateStatelessPrivateSurfaceThenPrivateMemSizeIsZero) {
    ze_kernel_properties_t kernelProperties = {};

    kernelProperties.spillMemSize = std::numeric_limits<uint32_t>::max();

    ze_kernel_properties_t kernelPropertiesBefore = {};
    kernelPropertiesBefore = kernelProperties;

    ze_result_t res = kernel->getProperties(&kernelProperties);
    EXPECT_EQ(ZE_RESULT_SUCCESS, res);

    L0::ModuleImp *moduleImp = reinterpret_cast<L0::ModuleImp *>(module.get());
    NEO::KernelInfo *ki = nullptr;
    for (uint32_t i = 0; i < moduleImp->getTranslationUnit()->programInfo.kernelInfos.size(); i++) {
        ki = moduleImp->getTranslationUnit()->programInfo.kernelInfos[i];
        if (ki->kernelDescriptor.kernelMetadata.kernelName.compare(0, ki->kernelDescriptor.kernelMetadata.kernelName.size(), kernel->getImmutableData()->getDescriptor().kernelMetadata.kernelName) == 0) {
            break;
        }
    }

    ki->patchInfo.pAllocateStatelessPrivateSurface = nullptr;
    EXPECT_EQ(0u, kernelProperties.privateMemSize);
}

using KernelLocalIdsTest = Test<ModuleFixture>;

HWTEST_F(KernelLocalIdsTest, WhenKernelIsCreatedThenDefaultLocalIdGenerationbyRuntimeIsTrue) {
    createKernel();

    EXPECT_TRUE(kernel->requiresGenerationOfLocalIdsByRuntime());
}

struct KernelIsaTests : Test<ModuleFixture> {
    void SetUp() override {
        Test<ModuleFixture>::SetUp();

        auto &capabilityTable = device->getNEODevice()->getRootDeviceEnvironment().getMutableHardwareInfo()->capabilityTable;
        bool createBcsEngine = !capabilityTable.blitterOperationsSupported;
        capabilityTable.blitterOperationsSupported = true;

        if (createBcsEngine) {
            auto &engine = device->getNEODevice()->getEngine(0);
            bcsOsContext.reset(OsContext::create(nullptr, 0, device->getNEODevice()->getDeviceBitfield(), aub_stream::ENGINE_BCS, PreemptionMode::Disabled,
                                                 false, false, false));
            engine.osContext = bcsOsContext.get();
            engine.commandStreamReceiver->setupContext(*bcsOsContext);
        }
    }

    std::unique_ptr<OsContext> bcsOsContext;
};

TEST_F(KernelIsaTests, givenKernelAllocationInLocalMemoryWhenCreatingWithoutAllowedCpuAccessThenUseBcsForTransfer) {
    DebugManagerStateRestore restore;
    DebugManager.flags.ForceLocalMemoryAccessMode.set(static_cast<int32_t>(LocalMemoryAccessMode::CpuAccessDisallowed));
    DebugManager.flags.ForceNonSystemMemoryPlacement.set(1 << (static_cast<int64_t>(NEO::GraphicsAllocation::AllocationType::KERNEL_ISA) - 1));

    uint32_t kernelHeap = 0;
    KernelInfo kernelInfo;
    kernelInfo.heapInfo.KernelHeapSize = 1;
    kernelInfo.heapInfo.pKernelHeap = &kernelHeap;

    KernelImmutableData kernelImmutableData(device);

    auto bcsCsr = device->getNEODevice()->getEngine(aub_stream::EngineType::ENGINE_BCS, false, false).commandStreamReceiver;
    auto initialTaskCount = bcsCsr->peekTaskCount();

    kernelImmutableData.initialize(&kernelInfo, device, 0, nullptr, nullptr, false);

    if (kernelImmutableData.getIsaGraphicsAllocation()->isAllocatedInLocalMemoryPool()) {
        EXPECT_EQ(initialTaskCount + 1, bcsCsr->peekTaskCount());
    } else {
        EXPECT_EQ(initialTaskCount, bcsCsr->peekTaskCount());
    }

    device->getNEODevice()->getMemoryManager()->freeGraphicsMemory(kernelInfo.kernelAllocation);
}

TEST_F(KernelIsaTests, givenKernelAllocationInLocalMemoryWhenCreatingWithAllowedCpuAccessThenDontUseBcsForTransfer) {
    DebugManagerStateRestore restore;
    DebugManager.flags.ForceLocalMemoryAccessMode.set(static_cast<int32_t>(LocalMemoryAccessMode::CpuAccessAllowed));
    DebugManager.flags.ForceNonSystemMemoryPlacement.set(1 << (static_cast<int64_t>(NEO::GraphicsAllocation::AllocationType::KERNEL_ISA) - 1));

    uint32_t kernelHeap = 0;
    KernelInfo kernelInfo;
    kernelInfo.heapInfo.KernelHeapSize = 1;
    kernelInfo.heapInfo.pKernelHeap = &kernelHeap;

    KernelImmutableData kernelImmutableData(device);

    auto bcsCsr = device->getNEODevice()->getEngine(aub_stream::EngineType::ENGINE_BCS, false, false).commandStreamReceiver;
    auto initialTaskCount = bcsCsr->peekTaskCount();

    kernelImmutableData.initialize(&kernelInfo, device, 0, nullptr, nullptr, false);

    EXPECT_EQ(initialTaskCount, bcsCsr->peekTaskCount());

    device->getNEODevice()->getMemoryManager()->freeGraphicsMemory(kernelInfo.kernelAllocation);
}

TEST_F(KernelIsaTests, givenKernelAllocationInLocalMemoryWhenCreatingWithDisallowedCpuAccessAndDisabledBlitterThenFallbackToCpuCopy) {
    DebugManagerStateRestore restore;
    DebugManager.flags.ForceLocalMemoryAccessMode.set(static_cast<int32_t>(LocalMemoryAccessMode::CpuAccessDisallowed));
    DebugManager.flags.ForceNonSystemMemoryPlacement.set(1 << (static_cast<int64_t>(NEO::GraphicsAllocation::AllocationType::KERNEL_ISA) - 1));

    device->getNEODevice()->getExecutionEnvironment()->rootDeviceEnvironments[0]->getMutableHardwareInfo()->capabilityTable.blitterOperationsSupported = false;

    uint32_t kernelHeap = 0;
    KernelInfo kernelInfo;
    kernelInfo.heapInfo.KernelHeapSize = 1;
    kernelInfo.heapInfo.pKernelHeap = &kernelHeap;

    KernelImmutableData kernelImmutableData(device);

    auto bcsCsr = device->getNEODevice()->getEngine(aub_stream::EngineType::ENGINE_BCS, false, false).commandStreamReceiver;
    auto initialTaskCount = bcsCsr->peekTaskCount();

    kernelImmutableData.initialize(&kernelInfo, device, 0, nullptr, nullptr, false);

    EXPECT_EQ(initialTaskCount, bcsCsr->peekTaskCount());

    device->getNEODevice()->getMemoryManager()->freeGraphicsMemory(kernelInfo.kernelAllocation);
}

TEST_F(KernelIsaTests, givenKernelInfoWhenInitializingImmutableDataWithInternalIsaThenCorrectAllocationTypeIsUsed) {
    uint32_t kernelHeap = 0;
    KernelInfo kernelInfo;
    kernelInfo.heapInfo.KernelHeapSize = 1;
    kernelInfo.heapInfo.pKernelHeap = &kernelHeap;

    KernelImmutableData kernelImmutableData(device);

    kernelImmutableData.initialize(&kernelInfo, device, 0, nullptr, nullptr, true);
    EXPECT_EQ(NEO::GraphicsAllocation::AllocationType::KERNEL_ISA_INTERNAL, kernelImmutableData.getIsaGraphicsAllocation()->getAllocationType());
}

TEST_F(KernelIsaTests, givenKernelInfoWhenInitializingImmutableDataWithNonInternalIsaThenCorrectAllocationTypeIsUsed) {
    uint32_t kernelHeap = 0;
    KernelInfo kernelInfo;
    kernelInfo.heapInfo.KernelHeapSize = 1;
    kernelInfo.heapInfo.pKernelHeap = &kernelHeap;

    KernelImmutableData kernelImmutableData(device);

    kernelImmutableData.initialize(&kernelInfo, device, 0, nullptr, nullptr, false);
    EXPECT_EQ(NEO::GraphicsAllocation::AllocationType::KERNEL_ISA, kernelImmutableData.getIsaGraphicsAllocation()->getAllocationType());
}

TEST_F(KernelIsaTests, givenGlobalBuffersWhenCreatingKernelImmutableDataThenBuffersAreAddedToResidencyContainer) {
    uint32_t kernelHeap = 0;
    KernelInfo kernelInfo;
    kernelInfo.heapInfo.KernelHeapSize = 1;
    kernelInfo.heapInfo.pKernelHeap = &kernelHeap;

    KernelImmutableData kernelImmutableData(device);

    uint64_t gpuAddress = 0x1200;
    void *buffer = reinterpret_cast<void *>(gpuAddress);
    size_t size = 0x1100;
    NEO::MockGraphicsAllocation globalVarBuffer(buffer, gpuAddress, size);
    NEO::MockGraphicsAllocation globalConstBuffer(buffer, gpuAddress, size);

    kernelImmutableData.initialize(&kernelInfo, device, 0,
                                   &globalConstBuffer, &globalVarBuffer, false);
    auto &resCont = kernelImmutableData.getResidencyContainer();
    EXPECT_EQ(1, std::count(resCont.begin(), resCont.end(), &globalVarBuffer));
    EXPECT_EQ(1, std::count(resCont.begin(), resCont.end(), &globalConstBuffer));
}

TEST_F(KernelIsaTests, givenDebugONAndKernelDegugInfoWhenInitializingImmutableDataThenRegisterElf) {
    uint32_t kernelHeap = 0;
    KernelInfo kernelInfo;
    kernelInfo.heapInfo.KernelHeapSize = 1;
    kernelInfo.heapInfo.pKernelHeap = &kernelHeap;
    auto debugData = new DebugData;
    kernelInfo.kernelDescriptor.external.debugData.reset(debugData);
    class MockDebugger : public DebuggerL0 {
      public:
        MockDebugger(NEO::Device *neodev) : DebuggerL0(neodev) {
        }
        void registerElf(NEO::DebugData *debugData, NEO::GraphicsAllocation *isaAllocation) override {
            debugData->vIsaSize = 123;
        };
        size_t getSbaTrackingCommandsSize(size_t trackedAddressCount) override { return static_cast<size_t>(0); };
        void programSbaTrackingCommands(NEO::LinearStream &cmdStream, const SbaAddresses &sba) override{};
    };
    MockDebugger *debugger = new MockDebugger(neoDevice);

    neoDevice->getExecutionEnvironment()->rootDeviceEnvironments[0]->debugger.reset(static_cast<NEO::Debugger *>(debugger));
    KernelImmutableData kernelImmutableData(device);

    kernelImmutableData.initialize(&kernelInfo, device, 0, nullptr, nullptr, false);
    EXPECT_EQ(kernelInfo.kernelDescriptor.external.debugData->vIsaSize, static_cast<uint32_t>(123));
}

using KernelImpPatchBindlessTest = Test<ModuleFixture>;

TEST_F(KernelImpPatchBindlessTest, GivenKernelImpWhenPatchBindlessOffsetCalledThenOffsetPatchedCorrectly) {
    Mock<Kernel> kernel;
    WhiteBox<::L0::DeviceImp> mockDevice;
    mockDevice.neoDevice = neoDevice;
    neoDevice->incRefInternal();
    neoDevice->bindlessHeapHelper.reset(new NEO::BindlessHeapsHelper(neoDevice->getMemoryManager(), neoDevice->getNumAvailableDevices() > 1, neoDevice->getRootDeviceIndex()));
    Mock<Module> mockModule(&mockDevice, nullptr);
    kernel.module = &mockModule;
    NEO::MockGraphicsAllocation alloc;
    uint32_t bindless = 0x40;
    auto &hwHelper = NEO::HwHelper::get(device->getHwInfo().platform.eRenderCoreFamily);
    size_t size = hwHelper.getRenderSurfaceStateSize();
    auto expectedSsInHeap = device->getNEODevice()->getBindlessHeapsHelper()->allocateSSInHeap(size, &alloc, NEO::BindlessHeapsHelper::GLOBAL_SSH);
    auto patchLocation = ptrOffset(kernel.getCrossThreadData(), bindless);
    auto patchValue = hwHelper.getBindlessSurfaceExtendedMessageDescriptorValue(static_cast<uint32_t>(expectedSsInHeap.surfaceStateOffset));

    auto ssPtr = kernel.patchBindlessSurfaceState(&alloc, bindless);

    EXPECT_EQ(ssPtr, expectedSsInHeap.ssPtr);
    EXPECT_TRUE(memcmp(const_cast<uint8_t *>(patchLocation), &patchValue, sizeof(patchValue)) == 0);
    EXPECT_TRUE(std::find(kernel.getResidencyContainer().begin(), kernel.getResidencyContainer().end(), expectedSsInHeap.heapAllocation) != kernel.getResidencyContainer().end());
}

HWTEST2_F(KernelImpPatchBindlessTest, GivenKernelImpWhenSetSurfaceStateBindlessThenSurfaceStateUpdated, MatchAny) {
    using RENDER_SURFACE_STATE = typename FamilyType::RENDER_SURFACE_STATE;

    ze_kernel_desc_t desc = {};
    desc.pKernelName = kernelName.c_str();

    WhiteBoxKernelHw<gfxCoreFamily> mockKernel;
    mockKernel.module = module.get();
    mockKernel.initialize(&desc);
    auto &arg = const_cast<NEO::ArgDescPointer &>(mockKernel.kernelImmData->getDescriptor().payloadMappings.explicitArgs[0].template as<NEO::ArgDescPointer>());
    arg.bindless = 0x40;
    arg.bindful = undefined<SurfaceStateHeapOffset>;

    neoDevice->bindlessHeapHelper.reset(new NEO::BindlessHeapsHelper(neoDevice->getMemoryManager(), neoDevice->getNumAvailableDevices() > 1, neoDevice->getRootDeviceIndex()));

    auto &hwHelper = NEO::HwHelper::get(device->getHwInfo().platform.eRenderCoreFamily);
    size_t size = hwHelper.getRenderSurfaceStateSize();
    uint64_t gpuAddress = 0x2000;
    void *buffer = reinterpret_cast<void *>(gpuAddress);

    NEO::MockGraphicsAllocation mockAllocation(buffer, gpuAddress, size);
    auto expectedSsInHeap = device->getNEODevice()->getBindlessHeapsHelper()->allocateSSInHeap(size, &mockAllocation, NEO::BindlessHeapsHelper::GLOBAL_SSH);

    memset(expectedSsInHeap.ssPtr, 0, size);
    auto surfaceStateBefore = *reinterpret_cast<RENDER_SURFACE_STATE *>(expectedSsInHeap.ssPtr);
    mockKernel.setBufferSurfaceState(0, buffer, &mockAllocation);

    auto surfaceStateAfter = *reinterpret_cast<RENDER_SURFACE_STATE *>(expectedSsInHeap.ssPtr);

    EXPECT_FALSE(memcmp(&surfaceStateAfter, &surfaceStateBefore, size) == 0);
}

HWTEST2_F(KernelImpPatchBindlessTest, GivenKernelImpWhenSetSurfaceStateBindfulThenSurfaceStateNotUpdated, MatchAny) {
    using RENDER_SURFACE_STATE = typename FamilyType::RENDER_SURFACE_STATE;
    ze_kernel_desc_t desc = {};
    desc.pKernelName = kernelName.c_str();

    WhiteBoxKernelHw<gfxCoreFamily> mockKernel;
    mockKernel.module = module.get();
    mockKernel.initialize(&desc);

    auto &arg = const_cast<NEO::ArgDescPointer &>(mockKernel.kernelImmData->getDescriptor().payloadMappings.explicitArgs[0].template as<NEO::ArgDescPointer>());
    arg.bindless = undefined<CrossThreadDataOffset>;
    arg.bindful = 0x40;

    neoDevice->bindlessHeapHelper.reset(new NEO::BindlessHeapsHelper(neoDevice->getMemoryManager(), neoDevice->getNumAvailableDevices() > 1, neoDevice->getRootDeviceIndex()));

    auto &hwHelper = NEO::HwHelper::get(device->getHwInfo().platform.eRenderCoreFamily);
    size_t size = hwHelper.getRenderSurfaceStateSize();
    uint64_t gpuAddress = 0x2000;
    void *buffer = reinterpret_cast<void *>(gpuAddress);

    NEO::MockGraphicsAllocation mockAllocation(buffer, gpuAddress, size);
    auto expectedSsInHeap = device->getNEODevice()->getBindlessHeapsHelper()->allocateSSInHeap(size, &mockAllocation, NEO::BindlessHeapsHelper::GLOBAL_SSH);

    memset(expectedSsInHeap.ssPtr, 0, size);
    auto surfaceStateBefore = *reinterpret_cast<RENDER_SURFACE_STATE *>(expectedSsInHeap.ssPtr);
    mockKernel.setBufferSurfaceState(0, buffer, &mockAllocation);

    auto surfaceStateAfter = *reinterpret_cast<RENDER_SURFACE_STATE *>(expectedSsInHeap.ssPtr);

    EXPECT_TRUE(memcmp(&surfaceStateAfter, &surfaceStateBefore, size) == 0);
}

struct MyMockKernel : public Mock<Kernel> {
    void setBufferSurfaceState(uint32_t argIndex, void *address, NEO::GraphicsAllocation *alloc) override {
        setSurfaceStateCalled = true;
    }
    bool setSurfaceStateCalled = false;
};

TEST_F(KernelImpPatchBindlessTest, GivenValidBindlessOffsetWhenSetArgBufferWithAllocThensetBufferSurfaceStateCalled) {
    ze_kernel_desc_t desc = {};
    desc.pKernelName = kernelName.c_str();
    MyMockKernel mockKernel;

    mockKernel.module = module.get();
    mockKernel.initialize(&desc);

    auto &arg = const_cast<NEO::ArgDescPointer &>(mockKernel.kernelImmData->getDescriptor().payloadMappings.explicitArgs[0].as<NEO::ArgDescPointer>());
    arg.bindless = 0x40;
    arg.bindful = undefined<SurfaceStateHeapOffset>;

    NEO::MockGraphicsAllocation alloc;

    mockKernel.setArgBufferWithAlloc(0, 0x1234, &alloc);

    EXPECT_TRUE(mockKernel.setSurfaceStateCalled);
}

TEST_F(KernelImpPatchBindlessTest, GivenValidBindfulOffsetWhenSetArgBufferWithAllocThensetBufferSurfaceStateCalled) {
    ze_kernel_desc_t desc = {};
    desc.pKernelName = kernelName.c_str();
    MyMockKernel mockKernel;

    mockKernel.module = module.get();
    mockKernel.initialize(&desc);

    auto &arg = const_cast<NEO::ArgDescPointer &>(mockKernel.kernelImmData->getDescriptor().payloadMappings.explicitArgs[0].as<NEO::ArgDescPointer>());
    arg.bindless = undefined<CrossThreadDataOffset>;
    arg.bindful = 0x40;

    NEO::MockGraphicsAllocation alloc;

    mockKernel.setArgBufferWithAlloc(0, 0x1234, &alloc);

    EXPECT_TRUE(mockKernel.setSurfaceStateCalled);
}

TEST_F(KernelImpPatchBindlessTest, GivenUndefiedBidfulAndBindlesstOffsetWhenSetArgBufferWithAllocThenSetBufferSurfaceStateIsNotCalled) {
    ze_kernel_desc_t desc = {};
    desc.pKernelName = kernelName.c_str();
    MyMockKernel mockKernel;

    mockKernel.module = module.get();
    mockKernel.initialize(&desc);

    auto &arg = const_cast<NEO::ArgDescPointer &>(mockKernel.kernelImmData->getDescriptor().payloadMappings.explicitArgs[0].as<NEO::ArgDescPointer>());
    arg.bindless = undefined<CrossThreadDataOffset>;
    arg.bindful = undefined<SurfaceStateHeapOffset>;

    NEO::MockGraphicsAllocation alloc;

    mockKernel.setArgBufferWithAlloc(0, 0x1234, &alloc);

    EXPECT_FALSE(mockKernel.setSurfaceStateCalled);
}
template <GFXCORE_FAMILY gfxCoreFamily>
struct MyMockImage : public WhiteBox<::L0::ImageCoreFamily<gfxCoreFamily>> {
    //MyMockImage() : WhiteBox<::L0::ImageCoreFamily<gfxCoreFamily>>();
    void copySurfaceStateToSSH(void *surfaceStateHeap, const uint32_t surfaceStateOffset) override {
        passedSurfaceStateHeap = surfaceStateHeap;
        passedSurfaceStateOffset = surfaceStateOffset;
    }
    void *passedSurfaceStateHeap = nullptr;
    uint32_t passedSurfaceStateOffset = 0;
};

HWTEST2_F(SetKernelArg, givenImageAndBindlessKernelWhenSetArgImageThenCopySurfaceStateToSSHCalledWithCorrectArgs, ImageSupport) {
    createKernel();

    neoDevice->bindlessHeapHelper.reset(new NEO::BindlessHeapsHelper(neoDevice->getMemoryManager(), neoDevice->getNumAvailableDevices() > 1, neoDevice->getRootDeviceIndex()));
    auto &imageArg = const_cast<NEO::ArgDescImage &>(kernel->kernelImmData->getDescriptor().payloadMappings.explicitArgs[3].template as<NEO::ArgDescImage>());
    auto &addressingMode = kernel->kernelImmData->getDescriptor().kernelAttributes.imageAddressingMode;
    const_cast<NEO::KernelDescriptor::AddressingMode &>(addressingMode) = NEO::KernelDescriptor::Bindless;
    imageArg.bindless = 0x0;
    imageArg.bindful = undefined<SurfaceStateHeapOffset>;
    ze_image_desc_t desc = {};
    auto &hwHelper = NEO::HwHelper::get(neoDevice->getHardwareInfo().platform.eRenderCoreFamily);
    auto surfaceStateSize = hwHelper.getRenderSurfaceStateSize();

    auto imageHW = std::make_unique<MyMockImage<gfxCoreFamily>>();
    auto ret = imageHW->initialize(device, &desc);
    auto handle = imageHW->toHandle();
    ASSERT_EQ(ZE_RESULT_SUCCESS, ret);

    auto expectedSsInHeap = neoDevice->bindlessHeapHelper->allocateSSInHeap(surfaceStateSize, imageHW->getAllocation(), BindlessHeapsHelper::BindlesHeapType::GLOBAL_SSH);

    kernel->setArgImage(3, sizeof(imageHW.get()), &handle);

    EXPECT_EQ(imageHW->passedSurfaceStateHeap, expectedSsInHeap.ssPtr);
    EXPECT_EQ(imageHW->passedSurfaceStateOffset, 0u);
}

HWTEST2_F(SetKernelArg, givenImageAndBindfulKernelWhenSetArgImageThenCopySurfaceStateToSSHCalledWithCorrectArgs, ImageSupport) {
    createKernel();

    auto &imageArg = const_cast<NEO::ArgDescImage &>(kernel->kernelImmData->getDescriptor().payloadMappings.explicitArgs[3].template as<NEO::ArgDescImage>());
    auto addressingMode = const_cast<NEO::KernelDescriptor::AddressingMode &>(kernel->kernelImmData->getDescriptor().kernelAttributes.imageAddressingMode);
    addressingMode = NEO::KernelDescriptor::Bindful;
    imageArg.bindless = undefined<CrossThreadDataOffset>;
    imageArg.bindful = 0x40;
    ze_image_desc_t desc = {};

    auto imageHW = std::make_unique<MyMockImage<gfxCoreFamily>>();
    auto ret = imageHW->initialize(device, &desc);
    auto handle = imageHW->toHandle();
    ASSERT_EQ(ZE_RESULT_SUCCESS, ret);

    kernel->setArgImage(3, sizeof(imageHW.get()), &handle);

    EXPECT_EQ(imageHW->passedSurfaceStateHeap, kernel->getSurfaceStateHeapData());
    EXPECT_EQ(imageHW->passedSurfaceStateOffset, imageArg.bindful);
}

} // namespace ult
} // namespace L0
