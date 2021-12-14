#pragma once

#ifndef _D3DCLASS_H_

#define _D3DCLASS_H_

/**
头文件中的第一件事是指定使用此对象模块时要链接的库。
d3d12.lib 第一个库包含在DirectX 12中设置和绘制3D图形的所有Direct3D功能。
dxgi.lib 第二个库包含与计算机上的硬件进行接口的工具，以获取有关显示器刷新速率、所使用的视频卡等等的信息。
d3dcompiler.lib 第三个库包含编译着色器的功能。
**/

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

#include <d3d12.h>
#include <dxgi1_4.h>

class D3DClass 
{
	public:
		D3DClass();
		D3DClass(const D3DClass&);
		~D3DClass();

		bool Initialize(int, int, HWND, bool, bool);
		void Shutdown();
		bool Render();
	private:
		bool m_vsync_enabled;
		ID3D12Device* m_device;
		ID3D12CommandQueue* m_commandQueue;
		char m_videoCardDescription[128];
		IDXGISwapChain3* m_swapChain;
		ID3D12DescriptorHeap* m_renderTargetViewHeap;
		ID3D12Resource* m_backBufferRenderTarget[2];
		unsigned int m_bufferIndex;
		ID3D12CommandAllocator* m_commandAllocator;
		ID3D12GraphicsCommandList* m_commandList;
		ID3D12PipelineState* m_pipelineState;
		ID3D12Fence* m_fence;
		HANDLE m_fenceEvent;
		unsigned long long m_fenceValue;
};
#endif // !_D3DCLASS_H_
