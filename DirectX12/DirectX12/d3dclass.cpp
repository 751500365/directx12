
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

	//�洢��ֱͬ�����á�
	m_vsync_enabled = vsync;

	//����Ҫ���ĵ�һ�����Ǵ���Direct3D�豸���������ǵ�DirectX����Ҫ�ӿڡ�
	//Ϊ�˴����豸������Ҫһ����Ϊ��������Ĳ��������Լ�����������������������ǽ�ʹ�õ�DirectX�汾��
	//����DirectX 12�������ݵĴ�����Ƶ���������Ը�����Ƶ�����������ù��ܼ���
	//���磬������Ƶ������ֻ��֧��directx10��Ӳ����������������������Ƶ��֧��directx12��
	//��ˣ����������õĹ��ܼ���Ϊ10_0��������ʹ��directx12��ȥ��������Ƶ���ϲ����õĹ��ܡ�

	//�����Լ�������ΪDirectX 12.1�����������е�DirectX 12���ԡ�
	//ע��:�������еĿ�Ƭ��֧��������directx12��������Լ��������Ҫ��һЩ��Ƭ�Ͻ��͵�12.0��
	featureLevel = D3D_FEATURE_LEVEL_12_1;

	// ����Direct3D 12�豸.
	result = D3D12CreateDevice(NULL, featureLevel, __uuidof(ID3D12Device), (void**)&m_device);
	if (FAILED(result))
	{
		MessageBox(hwnd, L"Could not create a DirectX 12.1 device.  The default video card does not support DirectX 12.1.", L"DirectX Device Failure", MB_OK);
		return false;
	}

	//��ʱ��������Կ���DirectX 12�����ݣ������豸�ĵ��ý�ʧ�ܡ�
	//��Щ����������������Ϊdirectx11��Ƶ��������Ҫ����Ϊdirectx12��Ƶ����
	//����һЩ����Կ�Ҳ�����������ģ���Ҫ�ǵ͹��ĵ�Intel�Կ�����Ҫ���Ǹ߹��ĵ�Nvidia�Կ���
	//Ҫ���������⣬����Ҫ��ʹ��Ĭ���豸������ö�ٻ����е�������Ƶ����
	//���û�ѡ��Ҫʹ����һ����Ȼ���ڴ����豸ʱָ�����ſ���
	//�������ʧ�ܣ�������ʹ����������ȡ��һ����Ƶ�����������ٴδ������豸��
	//Ҫ����Ĭ���Կ�����������Կ�������Ҫ����Ĭ���Կ�����������Ϊ��һ���������ͣ�������ʹ��NULL��

	//���豸����֮��Ȼ�󴴽�������С�
	//����ʹ��DirectX 12�е����������ִ�������б�
	//�����ϣ�ÿһ֡���Ƕ�����Ⱦ�ŵ������б��У�Ȼ������Ǵ��ݸ�������У���������GPU��ִ�С�
	//ÿ��GPUͨ����һ��������С�
	//�ڱ��̳��У����ǽ��ڵ���������Ϊ0����ָ��ֻʹ�õ���GPU��

	// ��ʼ��������е�������
	ZeroMemory(&commandQueueDesc, sizeof(commandQueueDesc));

	// ����������е�������
	commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	commandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	commandQueueDesc.NodeMask = 0;

	//����������С�
	result = m_device->CreateCommandQueue(&commandQueueDesc, __uuidof(ID3D12CommandQueue), (void**)&m_commandQueue);
	if (FAILED(result))
	{
		return false;
	}

	//�����ǳ�ʼ��������֮ǰ�����Ǳ�����Կ� / �������л�ȡˢ���ʡ�
	//ÿ̨������������в�ͬ��������ǽ���Ҫ��ѯ����Ϣ��
	//���ǲ�ѯ���Ӻͷ�ĸֵ��Ȼ���������ڼ佫���Ǵ��ݸ�DirectX�����������ʵ���ˢ���ʡ�
	//������ǲ�����������ֻ�ǽ�ˢ��������ΪĬ��ֵ(����ܲ������������м������)��
	//��ôDirectX��ͨ��ִ�л��������ƶ����ǻ�������ת����Ӧ��
	//��ή�����ܲ��ڵ�������и����Ǵ������˵Ĵ���


	// ����һ��DirectXͼ�νӿڴ���.
	result = CreateDXGIFactory1(__uuidof(IDXGIFactory4), (void**)&factory);
	if (FAILED(result))
	{
		return false;
	}

	// ʹ�ô���Ϊ��Ҫͼ�νӿ�(�Կ�)������������
	result = factory->EnumAdapters(0, &adapter);
	if (FAILED(result))
	{
		return false;
	}

	// ö�������������(������).
	result = adapter->EnumOutputs(0, &adapterOutput);
	if (FAILED(result))
	{
		return false;
	}

	//��ȡ�ʺ������������DXGI_FORMAT_R8G8B8A8_UNORM��ʾ��ʽ��ģʽ��(������)��
	// DXGI_ENUM_MODES_INTERLACED
	UINT flags = 0;
	result = adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, flags, &numModes, NULL);
	if (FAILED(result))
	{
		return false;
	}

	// ����һ���б����������������/�Կ���ϵ����п��ܵ���ʾģʽ��
	displayModeList = new DXGI_MODE_DESC[numModes];
	if (!displayModeList)
	{
		return false;
	}

	//���������ʾģʽ�б�ṹ��
	result = adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, flags, &numModes, displayModeList);
	if (FAILED(result))
	{
		return false;
	}

	// ���ڱ������е���ʾģʽ���ҵ�����Ļ�߶ȺͿ��ƥ���ģʽ��
	//�ҵ�ƥ��ʱ���洢�ü�����ˢ�����ʵķ��Ӻͷ�ĸ��
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

	// ������������ˢ���ʵķ��Ӻͷ�ĸ��
	//���ǽ�ʹ�����������������һ�������Կ������ƺ��Կ��ڴ档

	//��ȡ������(��Ƶ��)˵����
	result = adapter->GetDesc(&adapterDesc);
	if (FAILED(result))
	{
		return false;
	}


	// ���Կ�������ת��Ϊ�ַ����鲢�洢����
	error = wcstombs_s(&stringLength, m_videoCardDescription, 128, adapterDesc.Description, 128);
	if (error != 0)
	{
		return false;
	}

	//���������Ѿ��洢��ˢ���ʵķ��Ӻͷ�ĸ�Լ���Ƶ����Ϣ�����ǿ����ͷ����ڻ�ȡ����Ϣ�Ľṹ�ͽӿڡ�
	//Ȼ�������ǲ����ͷŽӿڣ���Ϊ������Ҫ����������������
	// �ͷ���ʾģʽ�б�
	delete[] displayModeList;
	displayModeList = 0;

	// �ͷ������������
	adapterOutput->Release();
	adapterOutput = 0;

	// �ͷ���������
	adapter->Release();
	adapter = 0;

	//����������������ϵͳ��ˢ�����ʣ����ǿ��Կ�ʼ�����������ˡ�
	//����Ҫ���ĵ�һ��������д����������������������ͼ�ν������Ƶ���������������
	//ͨ��ʹ��һ����̨�������������������л�ͼ��Ȼ�����л����û���Ļ��
	//��������ʾʱ���㿪ʼ������һ֡����һ���ػ��塣��ֻҪ��ÿһ֡�н�������; 
	//�����Ϊʲô������Ϊ��������
	//ע�⣬������룬��������������ϵĻ������������ڽ̳̣����ǽ����ֻʹ��˫������ϵͳ��

	// ��ʼ��������������
	ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));

	//������������Ϊʹ��˫���塣
	swapChainDesc.BufferCount = 2;

	// ���ý������еĺ�̨�������ĸ߶ȺͿ�ȡ�
	swapChainDesc.BufferDesc.Height = screenHeight;
	swapChainDesc.BufferDesc.Width = screenWidth;

	// Ϊ��̨����������һ�������32λ���档
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	//����̨���������÷�����Ϊ����Ŀ�������
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

	//���ý���Ч�����ڽ���������ǰ�Ļ��������ݡ�
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	// ������Ⱦ���ڵľ����
	swapChainDesc.OutputWindow = hwnd;

	// ����Ϊȫ���򴰿�ģʽ��
	if (fullscreen)
	{
		swapChainDesc.Windowed = false;
	}
	else
	{
		swapChainDesc.Windowed = true;
	}

	//��������������һ������ˢ���ʡ�ˢ������ָһ���ӽ����˻������ϵ���Ļ�Ĵ�����
	//�����graphicsclass.h��ͷ�н�vsync����Ϊtrue����ô�⽫����ϵͳ���õ�ˢ������(����60hz)��
	//����ζ������ÿ��ֻ������Ļ60��(���ϵͳˢ���ʴ���60�������)��
	//Ȼ����������ǽ�vsync����Ϊfalse����ô������һ�����ھ����ܶ�ػ�����Ļ��������ܻᵼ��һЩ�Ӿ�����

	// ���ñ��ݻ�������ˢ������.
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

	// �ص�multisampling.
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;

	// ��ɨ����˳�����������Ϊδָ����
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	//��Ҫ���ø߼���־��
	swapChainDesc.Flags = 0;

	//һ����д�����������������ǾͿ��Դ����������ˡ�
	///����������֮�����ǽ�itts�ӿ��������汾3�Ľ�������
	//�������ǾͿ��Է��ʰ汾1��û�еĸ��µĽ�����������
	//����ע�⣬�ڴ���������ʱ�����ǻ���������֮������������С�
	//������������ʱ�����ǽ��������ͺ�̨��������������ָ�������������Ե�GPU��
	//�����Ҫ��Ⱦ���gpu�������Ϊÿ��gpu�����������ͺ�̨�������������߼������飬���罻��֡��Ⱦ

	// /���ʹ�ý���������������������	
	result = factory->CreateSwapChain(m_commandQueue, &swapChainDesc, &swapChain);
	if (FAILED(result))
	{
		return false;
	}

	// ����������IDXGISwapChain������IDXGISwapChain3�ӿڣ�
	//������洢����Ϊm_swapChain��˽�г�Ա�����С�
	//�⽫ʹ�����ܹ�ʹ�ø��µĹ��ܣ������ȡ��ǰ�ĺ󻺳���������
	result = swapChain->QueryInterface(__uuidof(IDXGISwapChain3), (void**)&m_swapChain);
	if (FAILED(result))
	{
		return false;
	}

	// ���ָ��ԭʼ�������ӿڵ�ָ�룬��Ϊ����ʹ�õ��ǰ汾3��m_swapChain��
	swapChain = 0;

	// ����������������Ȼ���ͷŹ�����
	factory->Release();
	factory = 0;
	//��Ȼ����������ȫ���ã��������ڿ���Ϊ�����󻺳���������ȾĿ����ͼ��
	//��ȾĿ����ͼ����GPU�������󱸻�����������Ⱦ������Դ��
	//Ҫ������ͼ������������Ҫ����һ�����������Խ��ϻػ�������ͼ�������ڴ��С�
	//�����������Ѻ����ǿ��Ի�ȡ�����ڴ�λ�õľ����
	//Ȼ��ʹ��ָ����ڴ�λ�õ�ָ�봴����ͼ���⽫��DirectX 12��������Դ�󶨵�ͨ�����⡣

	// ��ʼ�������󻺳�������ȾĿ����ͼ��������
	ZeroMemory(&renderTargetViewHeapDesc, sizeof(renderTargetViewHeapDesc));

	//�������󱸻���������������������Ϊ�����������ö���������ȾĿ����ͼ��	
	renderTargetViewHeapDesc.NumDescriptors = 2;
	renderTargetViewHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	renderTargetViewHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	// Ϊ�󻺳���������ȾĿ����ͼ�ѡ�
	result = m_device->CreateDescriptorHeap(&renderTargetViewHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&m_renderTargetViewHeap);
	if (FAILED(result))
	{
		return false;
	}

	// ��ȡ��ȾĿ����ͼ������ʼ�ڴ�λ�õľ�����Ա�ʶ�����󻺳�������ȾĿ����ͼ��λ�ںδ���
	renderTargetViewHandle = m_renderTargetViewHeap->GetCPUDescriptorHandleForHeapStart();

	// ��ȡ��ȾĿ����ͼ���������ڴ�λ�ô�С
	renderTargetViewDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// �ӽ�������ȡָ���һ���󱸻�������ָ�롣
	result = m_swapChain->GetBuffer(0, __uuidof(ID3D12Resource), (void**)&m_backBufferRenderTarget[0]);
	if (FAILED(result))
	{
		return false;
	}

	// Ϊ��һ���󻺳�������һ����ȾĿ����ͼ��
	m_device->CreateRenderTargetView(m_backBufferRenderTarget[0], NULL, renderTargetViewHandle);

	// ����ͼ������ӵ���ȾĿ����ͼ���е���һ��������λ�á�
	renderTargetViewHandle.ptr += renderTargetViewDescriptorSize;

	// �ӽ�������ȡָ��ڶ����󻺳�����ָ�롣
	result = m_swapChain->GetBuffer(1, __uuidof(ID3D12Resource), (void**)&m_backBufferRenderTarget[1]);
	if (FAILED(result))
	{
		return false;
	}

	// Ϊ�ڶ����󻺳�������һ����ȾĿ����ͼ
	m_device->CreateRenderTargetView(m_backBufferRenderTarget[1], NULL, renderTargetViewHandle);

	//ͨ��Ϊ���ǵ������󻺳�������������ȾĿ����ͼ�����ǽ��ܹ�ʹ�����ǽ�����Ⱦ��
	//���ȣ�������Ҫ��ȡһ����������ǰ�����������Ƶ���������

	// /���ջ�ó�ʼ�������������ǵ�ǰ���򻺳�������ʼ������
	m_bufferIndex = m_swapChain->GetCurrentBackBufferIndex();

	//���Ǵ�������һ�����������������
	//���������������Ϊ����ÿ֡���͸�GPU����Ⱦͼ�ε������б�����ڴ�

	//����һ�������������
	result = m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), (void**)&m_commandAllocator);
	if (FAILED(result))
	{
		return false;
	}

	//����Ҫ������һ���Ǵ��������б������б���DirectX 12��Ҫ���Ĺؼ����֮һ��
	//�����ϣ�ÿ֡��������Ⱦ������䵽�����б��У�Ȼ���䷢�͵������������ִ�������б�
	//���ң�������ø��߼�ʱ����������������б�����ִ�����ǣ��������ȾЧ�ʡ�
	//���ǣ����úܼ��֣���Ϊ����Ҫ�����κζ��̳߳�����һ��������Դ��
	//��ȷ����ȫ�ش����߳�֮���ִ��˳���������ϵ�����ǳ��ڼ������
	//�ڱ��̳��У�����ʱ���ڴ˴�����һ����
	//���Ժ�Ľ̳��У�������D3DClass��ɾ������Ϊ�����������ط���

	//����һ�����������б�.
	result = m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator, NULL, __uuidof(ID3D12GraphicsCommandList), (void**)&m_commandList);
	if (FAILED(result))
	{
		return false;
	}

	//�����������Ҫ�ڳ�ʼ���ڼ�ر������б���Ϊ�����ڼ�¼״̬�´����ġ�	
	result = m_commandList->Close();
	if (FAILED(result))
	{
		return false;
	}

	//����Ҫ���������һ������դ������GPU��ȫ��Ⱦ������ͨ����������ύ�������б�ʱ��
	//���ǽ�Χ�������ź�֪ͨ������֪ͨ���ǡ�
	//GPU��CPUͬ����ȫȡ����������DirectX 12�еĴ�����������˸���դ��Ϊ�ǳ���Ҫ�Ĺ��ߡ�

	//ΪGPUͬ������Χ����
	result = m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), (void**)&m_fence);
	if (FAILED(result))
	{
		return false;
	}

	// Ϊդ������һ���¼�����
	m_fenceEvent = CreateEventEx(NULL, FALSE, FALSE, EVENT_ALL_ACCESS);
	if (m_fenceEvent == NULL)
	{
		return false;
	}

	// ��ʼ����ʼ�����ֵ��
	m_fenceValue = 1;

	return true;
}
//Shutdown�������ͷŲ����Initialize������ʹ�õ�����ָ�룬��ǳ��򵥡�
//���ǣ��ڴ�֮ǰ�����Ƚ�����һ�ε�����ǿ�ƽ��������ͷ��κ�ָ��֮ǰ�Ƚ��봰��ģʽ��
//�����������������������ȫ��ģʽ�ͷŽ�����������׳�һЩ�쳣��
//��ˣ�Ϊ���ⷢ��������������������ڹر�Direct3D֮ǰʼ��ǿ��ִ�д���ģʽ��
void D3DClass::Shutdown()
{
	int error;


	//�ڹر�����Ϊ����ģʽ֮ǰ�����ͷŽ�����֮ǰ�����������쳣	
	if (m_swapChain)
	{
		m_swapChain->SetFullscreenState(false, NULL);
	}

	// �ر�fence�¼��Ķ�������
	error = CloseHandle(m_fenceEvent);
	if (error == 0)
	{
	}

	// �ͷ�fence.
	if (m_fence)
	{
		m_fence->Release();
		m_fence = 0;
	}

	// �ͷſչܵ�״̬��
	if (m_pipelineState)
	{
		m_pipelineState->Release();
		m_pipelineState = 0;
	}

	//�ͷ������б�
	if (m_commandList)
	{
		m_commandList->Release();
		m_commandList = 0;
	}

	// �ͷ����������
	if (m_commandAllocator)
	{
		m_commandAllocator->Release();
		m_commandAllocator = 0;
	}

	// �ͷź󻺳�����ȾĿ����ͼ��.
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

	// �ͷ���ȾĿ����ͼ�ѡ�.
	if (m_renderTargetViewHeap)
	{
		m_renderTargetViewHeap->Release();
		m_renderTargetViewHeap = 0;
	}

	// �ͷŽ�������
	if (m_swapChain)
	{
		m_swapChain->Release();
		m_swapChain = 0;
	}

	// �ͷ��������
	if (m_commandQueue)
	{
		m_commandQueue->Release();
		m_commandQueue = 0;
	}

	// �ͷ��豸��
	if (m_device)
	{
		m_device->Release();
		m_device = 0;
	}

	return;
}

//���̵̳�D3DClass::Render����ֻ�ǽ���Ļ���Ϊ��ɫ��������Ⱦͼ�εľ�����Сֵ�ǳ���

bool D3DClass::Render()
{
	HRESULT result;
	D3D12_RESOURCE_BARRIER barrier;
	D3D12_CPU_DESCRIPTOR_HANDLE renderTargetViewHandle;
	unsigned int renderTargetViewDescriptorSize;
	float color[4];
	ID3D12CommandList* ppCommandLists[1];
	unsigned long long fenceToWaitFor;

	//��Ⱦ�ĵ�һ����������������������������б��ڴ档����������ע�⵽����ʹ�õĹܵ���ǰΪNULL��
	//������Ϊ�ܵ���Ҫ��ɫ���Ͷ�������ã����ǽ�����һ���̳��н�����Щ���ݡ�

	// /���ã����ã����ڴ���ص����������
	result = m_commandAllocator->Reset();
	if (FAILED(result))
	{
		return false;
	}

	//���������б�����û����ɫ���������������������Ļ��������ʱʹ�ÿչܵ�״̬��	
	result = m_commandList->Reset(m_commandAllocator, m_pipelineState);
	if (FAILED(result))
	{
		return false;
	}
	//�ڶ�����ʹ����Դ������ͬ�� / ת����һ�����ػ������Խ�����Ⱦ��Ȼ�󣬽�������Ϊ�����б��еĲ��衣

	// �����������б��м�¼����.
	// ����������Դ�ϰ���
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = m_backBufferRenderTarget[m_bufferIndex];
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	m_commandList->ResourceBarrier(1, &barrier);

	//�������ǻ�ȡ��̨��������ͼ�����Ȼ�󽫺�̨����������Ϊ��ȾĿ��
	// ��ȡ��ǰ��̨����������ȾĿ����ͼ�����

	renderTargetViewHandle = m_renderTargetViewHeap->GetCPUDescriptorHandleForHeapStart();
	renderTargetViewDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	if (m_bufferIndex == 1)
	{
		renderTargetViewHandle.ptr += renderTargetViewDescriptorSize;
	}

	// /����̨����������Ϊ��ȾĿ��.
	m_commandList->OMSetRenderTargets(1, &renderTargetViewHandle, FALSE, NULL);

	//�ڵ��Ĳ��У����ǽ������ɫ����Ϊ��ɫ����ʹ�ø���ɫ�����ȾĿ�겢�����ύ�������б�

	// /Ȼ��������ɫ���������.
	color[0] = 0.1;
	color[1] = 1.0;
	color[2] = 1.0;
	color[3] = 1.0;
	m_commandList->ClearRenderTargetView(renderTargetViewHandle, color, 0, NULL);

	//�������Ȼ�󽫺�̨��������״̬����Ϊת��Ϊ����״̬��������洢�������б��С�
	// ָʾ���ڽ�ʹ�ú󻺳���������

	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	m_commandList->ResourceBarrier(1, &barrier);

	//�����Ⱦ�б�����ǽ��ر������б�Ȼ�����ύ�����������Ϊ����ִ�и��б�
	// �ر������б�
	result = m_commandList->Close();
	if (FAILED(result))
	{
		return false;
	}

	//���������б����飨Ŀǰ��һ�������б���
	ppCommandLists[0] = m_commandList;

	//ִ�������б�
	m_commandQueue->ExecuteCommandLists(1, ppCommandLists);
	//Ȼ�����ǵ��ý������Խ���Ⱦ��֡���ָ���Ļ

	// ������Ⱦ��ɣ���󽫺󻺳�����ʾ����Ļ�ϡ�
	if (m_vsync_enabled)
	{
		// ������Ļˢ���ʡ�
		result = m_swapChain->Present(1, 0);
		if (FAILED(result))
		{
			return false;
		}
	}
	else
	{
		// �������
		result = m_swapChain->Present(0, 0);
		if (FAILED(result))
		{
			return false;
		}
	}

	//Ȼ����������Χդ�Խ���ͬ��������GPU�����Ⱦʱ֪ͨ���ǡ�
	//���ڱ��̳̣�����ֻ�����޵صȴ���ֱ������˴˵��������б�
	//���ǣ�������ͨ���ڵȴ�GPU���֮ǰ������������������Ż���

	//�����źŲ�����Χդֵ��
	fenceToWaitFor = m_fenceValue;
	result = m_commandQueue->Signal(m_fence, fenceToWaitFor);
	if (FAILED(result))
	{
		return false;
	}
	m_fenceValue++;

	// �ȴ���ֱ��GPU�����Ⱦ��
	if (m_fence->GetCompletedValue() < fenceToWaitFor)
	{
		result = m_fence->SetEventOnCompletion(fenceToWaitFor, m_fenceEvent);
		if (FAILED(result))
		{
			return false;
		}
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}
	//������һ֡��ʹ�ý���������������һ���󻺳�����

	// ÿ֡��0��1֮�������л��󻺳���������
	m_bufferIndex == 0 ? m_bufferIndex = 1 : m_bufferIndex = 0;

	return true;

}

/*��ˣ������������ڿ��Գ�ʼ���͹ر�Direct3D����������д��뽫��������һ�̳���ͬ�Ĵ��ڣ�
����Direct3D�����ѳ�ʼ�������ҽ��������Ϊ��ɫ��
��������д��뻹����ʾ�Ƿ���ȷ�����˱��������Լ��Ƿ���Դ�Windows SDK�п�����ͷ�Ϳ��ļ���
*/