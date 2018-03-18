#include "Common/Utility.h"
#include "FrameResource.h"

FrameResource::FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount)
{
	ASSERT_HR(device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(CmdListAlloc.GetAddressOf())));

	PassCB		= std::make_unique<UploadBuffer<PassConstants>>(device, passCount, true);
	MaterialCB	= std::make_unique<UploadBuffer<MaterialConstants>>(device, objectCount, true);
	ObjectCB	= std::make_unique<UploadBuffer<ObjectConstants>>(device, objectCount, true);
}

FrameResource::~FrameResource()
{

}