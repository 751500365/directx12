
#include "d3dclass.h"
D3DClass::D3DClass() 
{
	m_device = 0;
	m_commandQueue = 0;
	m_swapChain = 0;
	m_renderTargetViewHeap = 0;
	m_backBufferRenderTarget[0] = 0;
	m_backBufferRenderTarget[1] = 0;
	m_commandAllocator = 0;
	m_pipelineState = 0;
	m_commandList = 0;
	m_fence = 0;
	m_fenceEvent = 0;
}

D3DClass::D3DClass(const D3DClass& other)
{
}


D3DClass::~D3DClass()
{
}

bool D3DClass::Initialize(int screenHeight, int screenWidth, HWND hwnd, bool vsync, bool fullscreen)
{
	D3D_FEATURE_LEVEL featureLevel;
	HRESULT result;
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc;
	IDXGIFactory4* factory;
	IDXGIAdapter* adapter;
	IDXGIOutput* adapterOutput;
	unsigned int numModes, i, numerator, denominator, renderTargetViewDescriptorSize;
	unsigned int stringLength;
	DXGI_MODE_DESC* displayModeList;
	DXGI_ADAPTER_DESC adapterDesc;
	int error;
	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	IDXGISwapChain* swapChain;
	D3D12_DESCRIPTOR_HEAP_DESC renderTargetViewHeapDesc;
	D3D12_CPU_DESCRIPTOR_HANDLE renderTargetViewHandle;

	//存储垂直同步设置。
	m_vsync_enabled = vsync;

	//我们要做的第一件事是创建Direct3D设备。这是我们到DirectX的主要接口。
	//为了创建设备，它需要一个称为特征级别的参数。特性级别基本上允许我们设置我们将使用的DirectX版本。
	//由于DirectX 12是向后兼容的大量视频卡，您可以根据视频卡的能力设置功能级别。
	//例如，您的视频卡可能只有支持directx10的硬件，但驱动程序让您的视频卡支持directx12，
	//因此，您可以设置的功能级别为10_0，您可以使用directx12减去在您的视频卡上不可用的功能。

	//将特性级别设置为DirectX 12.1，以启用所有的DirectX 12特性。
	//注意:不是所有的卡片都支持完整的directx12，这个特性级别可能需要在一些卡片上降低到12.0。
	featureLevel = D3D_FEATURE_LEVEL_12_1;

	// 创建Direct3D 12设备.
	result = D3D12CreateDevice(NULL, featureLevel, __uuidof(ID3D12Device), (void**)&m_device);
	if (FAILED(result))
	{
		MessageBox(hwnd, L"Could not create a DirectX 12.1 device.  The default video card does not support DirectX 12.1.", L"DirectX Device Failure", MB_OK);
		return false;
	}

	//有时，如果主显卡与DirectX 12不兼容，创建设备的调用将失败。
	//有些机器可能有主卡作为directx11视频卡，而次要卡作为directx12视频卡。
	//还有一些混合显卡也是这样工作的，主要是低功耗的Intel显卡，次要的是高功耗的Nvidia显卡。
	//要解决这个问题，您需要不使用默认设备，而是枚举机器中的所有视频卡，
	//让用户选择要使用哪一个，然后在创建设备时指定那张卡。
	//或者如果失败，您可以使用适配器获取下一个视频卡，并尝试再次创建该设备。
	//要设置默认显卡以外的其他显卡，您需要将非默认显卡的适配器作为第一个参数发送，而不是使用NULL。

	//在设备创建之后，然后创建命令队列。
	//我们使用DirectX 12中的命令队列来执行命令列表。
	//基本上，每一帧我们都把渲染放到命令列表中，然后把它们传递给命令队列，让它们在GPU上执行。
	//每个GPU通常有一个命令队列。
	//在本教程中，我们将节点掩码设置为0，以指定只使用单个GPU。

	// 初始化命令队列的描述。
	ZeroMemory(&commandQueueDesc, sizeof(commandQueueDesc));

	// 设置命令队列的描述。
	commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	commandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	commandQueueDesc.NodeMask = 0;

	//创建命令队列。
	result = m_device->CreateCommandQueue(&commandQueueDesc, __uuidof(ID3D12CommandQueue), (void**)&m_commandQueue);
	if (FAILED(result))
	{
		return false;
	}

	//在我们初始化交换链之前，我们必须从显卡 / 监视器中获取刷新率。
	//每台计算机可能略有不同，因此我们将需要查询该信息。
	//我们查询分子和分母值，然后在设置期间将它们传递给DirectX，它将计算适当的刷新率。
	//如果我们不这样做，而只是将刷新率设置为默认值(这可能并不存在于所有计算机上)，
	//那么DirectX将通过执行缓冲区复制而不是缓冲区翻转来响应，
	//这会降低性能并在调试输出中给我们带来恼人的错误。


	// 创建一个DirectX图形接口代理.
	result = CreateDXGIFactory1(__uuidof(IDXGIFactory4), (void**)&factory);
	if (FAILED(result))
	{
		return false;
	}

	// 使用代理为主要图形接口(显卡)创建适配器。
	result = factory->EnumAdapters(0, &adapter);
	if (FAILED(result))
	{
		return false;
	}

	// 枚举主适配器输出(监视器).
	result = adapter->EnumOutputs(0, &adapterOutput);
	if (FAILED(result))
	{
		return false;
	}

	//获取适合适配器输出的DXGI_FORMAT_R8G8B8A8_UNORM显示格式的模式数(监视器)。
	// DXGI_ENUM_MODES_INTERLACED
	UINT flags = 0;
	result = adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, flags, &numModes, NULL);
	if (FAILED(result))
	{
		return false;
	}

	// 创建一个列表来保存这个监视器/显卡组合的所有可能的显示模式。
	displayModeList = new DXGI_MODE_DESC[numModes];
	if (!displayModeList)
	{
		return false;
	}

	//现在填充显示模式列表结构。
	result = adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, flags, &numModes, displayModeList);
	if (FAILED(result))
	{
		return false;
	}

	// 现在遍历所有的显示模式并找到与屏幕高度和宽度匹配的模式。
	//找到匹配时，存储该监视器刷新速率的分子和分母。
	for (i = 0; i < numModes; i++)
	{
		if (displayModeList[i].Height == (unsigned int)screenHeight)
		{
			if (displayModeList[i].Width == (unsigned int)screenWidth)
			{
				numerator = displayModeList[i].RefreshRate.Numerator;
				denominator = displayModeList[i].RefreshRate.Denominator;
			}
		}
	}

	// 我们现在有了刷新率的分子和分母。
	//我们将使用适配器检索的最后一件事是显卡的名称和显卡内存。

	//获取适配器(视频卡)说明。
	result = adapter->GetDesc(&adapterDesc);
	if (FAILED(result))
	{
		return false;
	}


	// 将显卡的名称转换为字符数组并存储它。
	error = wcstombs_s(&stringLength, m_videoCardDescription, 128, adapterDesc.Description, 128);
	if (error != 0)
	{
		return false;
	}

	//现在我们已经存储了刷新率的分子和分母以及视频卡信息，我们可以释放用于获取该信息的结构和接口。
	//然而，我们不会释放接口，因为我们需要它来创建交换链。
	// 释放显示模式列表。
	delete[] displayModeList;
	displayModeList = 0;

	// 释放适配器输出。
	adapterOutput->Release();
	adapterOutput = 0;

	// 释放适配器。
	adapter->Release();
	adapter = 0;

	//现在我们有了来自系统的刷新速率，我们可以开始创建交换链了。
	//我们要做的第一件事是填写交换链的描述。交换链是图形将被绘制到的两个缓冲区。
	//通常使用一个后台缓冲区，对它进行所有绘图，然后将其切换到用户屏幕。
	//当它被显示时，你开始绘制下一帧到另一个回缓冲。你只要在每一帧中交换它们; 
	//这就是为什么它被称为交换链。
	//注意，如果你想，你可以有两个以上的缓冲区，但对于教程，我们将坚持只使用双缓冲区系统。

	// 初始化交换链描述。
	ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));

	//将交换链设置为使用双缓冲。
	swapChainDesc.BufferCount = 2;

	// 设置交换链中的后台缓冲区的高度和宽度。
	swapChainDesc.BufferDesc.Height = screenHeight;
	swapChainDesc.BufferDesc.Width = screenWidth;

	// 为后台缓冲区设置一个常规的32位表面。
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	//将后台缓冲区的用法设置为呈现目标输出。
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

	//设置交换效果以在交换后丢弃先前的缓冲区内容。
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	// 设置渲染窗口的句柄。
	swapChainDesc.OutputWindow = hwnd;

	// 设置为全屏或窗口模式。
	if (fullscreen)
	{
		swapChainDesc.Windowed = false;
	}
	else
	{
		swapChainDesc.Windowed = true;
	}

	//交换链描述的下一部分是刷新率。刷新率是指一秒钟将回退缓冲区拖到屏幕的次数。
	//如果在graphicsclass.h报头中将vsync设置为true，那么这将锁定系统设置的刷新速率(例如60hz)。
	//这意味着它将每秒只绘制屏幕60次(如果系统刷新率大于60，则更高)。
	//然而，如果我们将vsync设置为false，那么它会在一秒钟内尽可能多地绘制屏幕，但这可能会导致一些视觉假象。

	// 设置备份缓冲区的刷新速率.
	if (m_vsync_enabled)
	{
		swapChainDesc.BufferDesc.RefreshRate.Numerator = numerator;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = denominator;
	}
	else
	{
		swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	}

	// 关掉multisampling.
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;

	// 将扫描行顺序和缩放设置为未指定。
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	//不要设置高级标志。
	swapChainDesc.Flags = 0;

	//一旦填写了描述，接下来我们就可以创建交换链了。
	///交换链创建之后，我们将itts接口升级到版本3的交换链，
	//这样我们就可以访问版本1中没有的更新的交换链方法。
	//还请注意，在创建交换链时，我们还将发送与之关联的命令队列。
	//当我们这样做时，我们将交换链和后台缓冲区关联到与指定的命令队列配对的GPU。
	//如果你要渲染多个gpu，你可以为每个gpu创建交换链和后台缓冲区来做更高级的事情，比如交替帧渲染

	// /最后，使用交换链描述创建交换链。	
	result = factory->CreateSwapChain(m_commandQueue, &swapChainDesc, &swapChain);
	if (FAILED(result))
	{
		return false;
	}

	// 接下来，将IDXGISwapChain升级到IDXGISwapChain3接口，
	//并将其存储在名为m_swapChain的私有成员变量中。
	//这将使我们能够使用更新的功能，例如获取当前的后缓冲区索引。
	result = swapChain->QueryInterface(__uuidof(IDXGISwapChain3), (void**)&m_swapChain);
	if (FAILED(result))
	{
		return false;
	}

	// 清除指向原始交换链接口的指针，因为我们使用的是版本3（m_swapChain）
	swapChain = 0;

	// 立即创建交换链，然后释放工厂。
	factory->Release();
	factory = 0;
	//既然交换链已完全设置，我们现在可以为两个后缓冲区设置渲染目标视图。
	//渲染目标视图允许GPU将两个后备缓冲区用作渲染到的资源。
	//要创建视图，我们首先需要创建一个描述符堆以将拖回缓冲区视图保存在内存中。
	//创建描述符堆后，我们可以获取堆中内存位置的句柄，
	//然后使用指向该内存位置的指针创建视图。这将是DirectX 12中所有资源绑定的通用主题。

	// 初始化两个后缓冲区的渲染目标视图堆描述。
	ZeroMemory(&renderTargetViewHeapDesc, sizeof(renderTargetViewHeapDesc));

	//将两个后备缓冲区的描述符数量设置为两个。还设置堆类型以渲染目标视图。	
	renderTargetViewHeapDesc.NumDescriptors = 2;
	renderTargetViewHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	renderTargetViewHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	// 为后缓冲区创建渲染目标视图堆。
	result = m_device->CreateDescriptorHeap(&renderTargetViewHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&m_renderTargetViewHeap);
	if (FAILED(result))
	{
		return false;
	}

	// 获取渲染目标视图堆中起始内存位置的句柄，以标识两个后缓冲区的渲染目标视图将位于何处。
	renderTargetViewHandle = m_renderTargetViewHeap->GetCPUDescriptorHandleForHeapStart();

	// 获取渲染目标视图描述符的内存位置大小
	renderTargetViewDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// 从交换链获取指向第一个后备缓冲区的指针。
	result = m_swapChain->GetBuffer(0, __uuidof(ID3D12Resource), (void**)&m_backBufferRenderTarget[0]);
	if (FAILED(result))
	{
		return false;
	}

	// 为第一个后缓冲区创建一个渲染目标视图。
	m_device->CreateRenderTargetView(m_backBufferRenderTarget[0], NULL, renderTargetViewHandle);

	// 将视图句柄增加到渲染目标视图堆中的下一个描述符位置。
	renderTargetViewHandle.ptr += renderTargetViewDescriptorSize;

	// 从交换链获取指向第二个后缓冲区的指针。
	result = m_swapChain->GetBuffer(1, __uuidof(ID3D12Resource), (void**)&m_backBufferRenderTarget[1]);
	if (FAILED(result))
	{
		return false;
	}

	// 为第二个后缓冲区创建一个渲染目标视图
	m_device->CreateRenderTargetView(m_backBufferRenderTarget[1], NULL, renderTargetViewHandle);

	//通过为我们的两个后缓冲区创建两个渲染目标视图，我们将能够使用它们进行渲染。
	//首先，我们需要获取一个索引，当前缓冲区将绘制到该索引。

	// /最终获得初始索引，缓冲区是当前反向缓冲区的起始索引。
	m_bufferIndex = m_swapChain->GetCurrentBackBufferIndex();

	//我们创建的下一件事是命令分配器。
	//命令分配器将用于为我们每帧发送给GPU的渲染图形的命令列表分配内存

	//创建一个命令分配器。
	result = m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), (void**)&m_commandAllocator);
	if (FAILED(result))
	{
		return false;
	}

	//我们要做的下一步是创建命令列表。命令列表是DirectX 12中要理解的关键组件之一。
	//基本上，每帧将所有渲染命令填充到命令列表中，然后将其发送到命令队列中以执行命令列表。
	//而且，当您变得更高级时，将创建多个命令列表并并行执行它们，以提高渲染效率。
	//但是，这变得很棘手，因为您需要像在任何多线程程序中一样管理资源，
	//并确保安全地处理线程之间的执行顺序和依赖关系。但是出于简单起见，
	//在本教程中，我暂时仅在此处创建一个。
	//在以后的教程中，它将从D3DClass中删除，因为它属于其他地方。

	//创建一个基本命令列表.
	result = m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator, NULL, __uuidof(ID3D12GraphicsCommandList), (void**)&m_commandList);
	if (FAILED(result))
	{
		return false;
	}

	//最初，我们需要在初始化期间关闭命令列表，因为它是在记录状态下创建的。	
	result = m_commandList->Close();
	if (FAILED(result))
	{
		return false;
	}

	//我们要创建的最后一件事是栅栏。当GPU完全渲染完我们通过命令队列提交的命令列表时，
	//我们将围栏用作信号通知机制来通知我们。
	//GPU和CPU同步完全取决于我们在DirectX 12中的处理能力，因此隔离栅成为非常必要的工具。

	//为GPU同步创建围栏。
	result = m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), (void**)&m_fence);
	if (FAILED(result))
	{
		return false;
	}

	// 为栅栏创建一个事件对象。
	m_fenceEvent = CreateEventEx(NULL, FALSE, FALSE, EVENT_ALL_ACCESS);
	if (m_fenceEvent == NULL)
	{
		return false;
	}

	// 初始化开始的篱笆值。
	m_fenceValue = 1;

	return true;
}
//Shutdown函数将释放并清除Initialize函数中使用的所有指针，这非常简单。
//但是，在此之前，我先进行了一次调用以强制交换链在释放任何指针之前先进入窗口模式。
//如果不这样做，而您尝试以全屏模式释放交换链，则会抛出一些异常。
//因此，为避免发生这种情况，我们总是在关闭Direct3D之前始终强制执行窗口模式。
void D3DClass::Shutdown()
{
	int error;


	//在关闭设置为窗口模式之前或在释放交换链之前，它将引发异常	
	if (m_swapChain)
	{
		m_swapChain->SetFullscreenState(false, NULL);
	}

	// 关闭fence事件的对象句柄。
	error = CloseHandle(m_fenceEvent);
	if (error == 0)
	{
	}

	// 释放fence.
	if (m_fence)
	{
		m_fence->Release();
		m_fence = 0;
	}

	// 释放空管道状态。
	if (m_pipelineState)
	{
		m_pipelineState->Release();
		m_pipelineState = 0;
	}

	//释放命令列表
	if (m_commandList)
	{
		m_commandList->Release();
		m_commandList = 0;
	}

	// 释放命令分配器
	if (m_commandAllocator)
	{
		m_commandAllocator->Release();
		m_commandAllocator = 0;
	}

	// 释放后缓冲区渲染目标视图。.
	if (m_backBufferRenderTarget[0])
	{
		m_backBufferRenderTarget[0]->Release();
		m_backBufferRenderTarget[0] = 0;
	}
	if (m_backBufferRenderTarget[1])
	{
		m_backBufferRenderTarget[1]->Release();
		m_backBufferRenderTarget[1] = 0;
	}

	// 释放渲染目标视图堆。.
	if (m_renderTargetViewHeap)
	{
		m_renderTargetViewHeap->Release();
		m_renderTargetViewHeap = 0;
	}

	// 释放交换链。
	if (m_swapChain)
	{
		m_swapChain->Release();
		m_swapChain = 0;
	}

	// 释放命令队列
	if (m_commandQueue)
	{
		m_commandQueue->Release();
		m_commandQueue = 0;
	}

	// 释放设备。
	if (m_device)
	{
		m_device->Release();
		m_device = 0;
	}

	return;
}

//本教程的D3DClass::Render函数只是将屏幕清除为灰色。教您渲染图形的绝对最小值非常简单

bool D3DClass::Render()
{
	HRESULT result;
	D3D12_RESOURCE_BARRIER barrier;
	D3D12_CPU_DESCRIPTOR_HANDLE renderTargetViewHandle;
	unsigned int renderTargetViewDescriptorSize;
	float color[4];
	ID3D12CommandList* ppCommandLists[1];
	unsigned long long fenceToWaitFor;

	//渲染的第一步是我们重置命令分配器和命令列表内存。您会在这里注意到我们使用的管道当前为NULL。
	//这是因为管道需要着色器和额外的设置，我们将在下一个教程中介绍这些内容。

	// /重置（重用）与内存相关的命令分配器
	result = m_commandAllocator->Reset();
	if (FAILED(result))
	{
		return false;
	}

	//重置命令列表，由于没有着色器，我们现在正在清空屏幕，现在暂时使用空管道状态。	
	result = m_commandList->Reset(m_commandAllocator, m_pipelineState);
	if (FAILED(result))
	{
		return false;
	}
	//第二步是使用资源屏障来同步 / 转换下一个返回缓冲区以进行渲染。然后，将其设置为命令列表中的步骤。

	// 现在在命令列表中记录命令.
	// 首先设置资源障碍。
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = m_backBufferRenderTarget[m_bufferIndex];
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	m_commandList->ResourceBarrier(1, &barrier);

	//第三步是获取后台缓冲区视图句柄，然后将后台缓冲区设置为渲染目标
	// 获取当前后台缓冲区的渲染目标视图句柄。

	renderTargetViewHandle = m_renderTargetViewHeap->GetCPUDescriptorHandleForHeapStart();
	renderTargetViewDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	if (m_bufferIndex == 1)
	{
		renderTargetViewHandle.ptr += renderTargetViewDescriptorSize;
	}

	// /将后台缓冲区设置为渲染目标.
	m_commandList->OMSetRenderTargets(1, &renderTargetViewHandle, FALSE, NULL);

	//在第四步中，我们将清除颜色设置为灰色，并使用该颜色清除渲染目标并将其提交到命令列表。

	// /然后设置颜色以清除窗口.
	color[0] = 0.1;
	color[1] = 1.0;
	color[2] = 1.0;
	color[3] = 1.0;
	m_commandList->ClearRenderTargetView(renderTargetViewHandle, color, 0, NULL);

	//最后，我们然后将后台缓冲区的状态设置为转换为呈现状态，并将其存储在命令列表中。
	// 指示现在将使用后缓冲区来呈现

	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	m_commandList->ResourceBarrier(1, &barrier);

	//完成渲染列表后，我们将关闭命令列表，然后将其提交到命令队列以为我们执行该列表。
	// 关闭命令列表。
	result = m_commandList->Close();
	if (FAILED(result))
	{
		return false;
	}

	//加载命令列表数组（目前仅一个命令列表）。
	ppCommandLists[0] = m_commandList;

	//执行命令列表。
	m_commandQueue->ExecuteCommandLists(1, ppCommandLists);
	//然后，我们调用交换链以将渲染的帧呈现给屏幕

	// 由于渲染完成，最后将后缓冲区显示在屏幕上。
	if (m_vsync_enabled)
	{
		// 锁定屏幕刷新率。
		result = m_swapChain->Present(1, 0);
		if (FAILED(result))
		{
			return false;
		}
	}
	else
	{
		// 尽快呈现
		result = m_swapChain->Present(0, 0);
		if (FAILED(result))
		{
			return false;
		}
	}

	//然后，我们设置围栅以进行同步，并在GPU完成渲染时通知我们。
	//对于本教程，我们只是无限地等待，直到完成了此单个命令列表。
	//但是，您可以通过在等待GPU完成之前进行其他处理来获得优化。

	//发出信号并增加围栅值。
	fenceToWaitFor = m_fenceValue;
	result = m_commandQueue->Signal(m_fence, fenceToWaitFor);
	if (FAILED(result))
	{
		return false;
	}
	m_fenceValue++;

	// 等待，直到GPU完成渲染。
	if (m_fence->GetCompletedValue() < fenceToWaitFor)
	{
		result = m_fence->SetEventOnCompletion(fenceToWaitFor, m_fenceEvent);
		if (FAILED(result))
		{
			return false;
		}
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}
	//对于下一帧，使用交替索引交换到另一个后缓冲区。

	// 每帧在0和1之间来回切换后缓冲区索引。
	m_bufferIndex == 0 ? m_bufferIndex = 1 : m_bufferIndex = 0;

	return true;

}

/*因此，现在我们终于可以初始化和关闭Direct3D。编译和运行代码将产生与上一教程相同的窗口，
但是Direct3D现在已初始化，并且将窗口清除为灰色。
编译和运行代码还将显示是否正确设置了编译器，以及是否可以从Windows SDK中看到标头和库文件。
*/