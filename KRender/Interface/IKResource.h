#pragma once

enum ResourceState
{
	// 还没加载或者加载失败
	RS_UNLOADED,
	// 内存数据加载在提交中
	RS_MEMORY_PENDING,
	// 正在加载内存数据
	RS_MEMORY_LOADING,
	// 内存数据加载完毕
	RS_MEMORY_LOADED,
	// 设备数据加载在提交中
	RS_DEVICE_PENDING,
	// 正在加载设备数据
	RS_DEVICE_LOADING,
	// 设备数据加载完毕
	RS_DEVICE_LOADED
};

struct IKResource
{
	virtual ~IKResource() {}
	virtual ResourceState GetResourceState() = 0;
	virtual void WaitForMemory() = 0;
	virtual void WaitForDevice() = 0;
};