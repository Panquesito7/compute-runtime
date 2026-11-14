/*
 * Copyright (C) 2020-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include <level_zero/ze_api.h>

namespace L0 {

ze_result_t ZE_APICALL zeMemAllocShared(
    ze_context_handle_t hContext,
    const ze_device_mem_alloc_desc_t *deviceDesc,
    const ze_host_mem_alloc_desc_t *hostDesc,
    size_t size,
    size_t alignment,
    ze_device_handle_t hDevice,
    void **pptr);

ze_result_t ZE_APICALL zeMemAllocDevice(
    ze_context_handle_t hContext,
    const ze_device_mem_alloc_desc_t *deviceDesc,
    size_t size,
    size_t alignment,
    ze_device_handle_t hDevice,
    void **pptr);

ze_result_t ZE_APICALL zeMemAllocHost(
    ze_context_handle_t hContext,
    const ze_host_mem_alloc_desc_t *hostDesc,
    size_t size,
    size_t alignment,
    void **pptr);

ze_result_t ZE_APICALL zeMemFree(
    ze_context_handle_t hContext,
    void *ptr);

ze_result_t ZE_APICALL zeMemFreeExt(
    ze_context_handle_t hContext,
    const ze_memory_free_ext_desc_t *pMemFreeDesc,
    void *ptr);

ze_result_t ZE_APICALL zeMemGetAllocProperties(
    ze_context_handle_t hContext,
    const void *ptr,
    ze_memory_allocation_properties_t *pMemAllocProperties,
    ze_device_handle_t *phDevice);

ze_result_t ZE_APICALL zeMemGetAddressRange(
    ze_context_handle_t hContext,
    const void *ptr,
    void **pBase,
    size_t *pSize);

ze_result_t ZE_APICALL zeMemGetIpcHandle(
    ze_context_handle_t hContext,
    const void *ptr,
    ze_ipc_mem_handle_t *pIpcHandle);

ze_result_t ZE_APICALL zeMemPutIpcHandle(
    ze_context_handle_t hContext,
    ze_ipc_mem_handle_t ipcHandle);

ze_result_t ZE_APICALL zeMemOpenIpcHandle(
    ze_context_handle_t hContext,
    ze_device_handle_t hDevice,
    ze_ipc_mem_handle_t handle,
    ze_ipc_memory_flags_t flags,
    void **pptr);

ze_result_t ZE_APICALL zeMemCloseIpcHandle(
    ze_context_handle_t hContext,
    const void *ptr);

ze_result_t ZE_APICALL zeMemGetIpcHandleFromFileDescriptorExp(ze_context_handle_t hContext, uint64_t handle, ze_ipc_mem_handle_t *pIpcHandle);

ze_result_t ZE_APICALL zeMemGetFileDescriptorFromIpcHandleExp(ze_context_handle_t hContext, ze_ipc_mem_handle_t ipcHandle, uint64_t *pHandle);

ze_result_t ZE_APICALL zeMemGetIpcHandleWithProperties(
    ze_context_handle_t hContext,
    const void *ptr,
    void *pNext,
    ze_ipc_mem_handle_t *pIpcHandle);

} // namespace L0

extern "C" {

ZE_APIEXPORT ze_result_t ZE_APICALL zeMemAllocShared(
    ze_context_handle_t hContext,
    const ze_device_mem_alloc_desc_t *deviceDesc,
    const ze_host_mem_alloc_desc_t *hostDesc,
    size_t size,
    size_t alignment,
    ze_device_handle_t hDevice,
    void **pptr);

ZE_APIEXPORT ze_result_t ZE_APICALL zeMemAllocDevice(
    ze_context_handle_t hContext,
    const ze_device_mem_alloc_desc_t *deviceDesc,
    size_t size,
    size_t alignment,
    ze_device_handle_t hDevice,
    void **pptr);

ZE_APIEXPORT ze_result_t ZE_APICALL zeMemAllocHost(
    ze_context_handle_t hContext,
    const ze_host_mem_alloc_desc_t *hostDesc,
    size_t size,
    size_t alignment,
    void **pptr);

ZE_APIEXPORT ze_result_t ZE_APICALL zeMemFree(
    ze_context_handle_t hContext,
    void *ptr);

ZE_APIEXPORT ze_result_t ZE_APICALL zeMemFreeExt(
    ze_context_handle_t hContext,
    const ze_memory_free_ext_desc_t *pMemFreeDesc,
    void *ptr);

ZE_APIEXPORT ze_result_t ZE_APICALL zeMemGetAllocProperties(
    ze_context_handle_t hContext,
    const void *ptr,
    ze_memory_allocation_properties_t *pMemAllocProperties,
    ze_device_handle_t *phDevice);

ZE_APIEXPORT ze_result_t ZE_APICALL zeMemGetAddressRange(
    ze_context_handle_t hContext,
    const void *ptr,
    void **pBase,
    size_t *pSize);

ZE_APIEXPORT ze_result_t ZE_APICALL zeMemGetIpcHandle(
    ze_context_handle_t hContext,
    const void *ptr,
    ze_ipc_mem_handle_t *pIpcHandle);

ZE_APIEXPORT ze_result_t ZE_APICALL zeMemOpenIpcHandle(
    ze_context_handle_t hContext,
    ze_device_handle_t hDevice,
    ze_ipc_mem_handle_t handle,
    ze_ipc_memory_flags_t flags,
    void **pptr);

ZE_APIEXPORT ze_result_t ZE_APICALL zeMemPutIpcHandle(
    ze_context_handle_t hContext,
    ze_ipc_mem_handle_t ipcHandle);

ZE_APIEXPORT ze_result_t ZE_APICALL zeMemCloseIpcHandle(
    ze_context_handle_t hContext,
    const void *ptr);

ZE_APIEXPORT ze_result_t ZE_APICALL zeMemGetIpcHandleFromFileDescriptorExp(
    ze_context_handle_t hContext,
    uint64_t handle,
    ze_ipc_mem_handle_t *pIpcHandle);

ZE_APIEXPORT ze_result_t ZE_APICALL zeMemGetFileDescriptorFromIpcHandleExp(
    ze_context_handle_t hContext,
    ze_ipc_mem_handle_t ipcHandle,
    uint64_t *pHandle);

ZE_APIEXPORT ze_result_t ZE_APICALL zeMemGetIpcHandleWithProperties(
    ze_context_handle_t hContext,
    const void *ptr,
    void *pNext,
    ze_ipc_mem_handle_t *pIpcHandle);

} // extern "C"
