//
// Game.cpp
//

#include "pch.h"
#include "Game.h"


//toreorganise
#include <fstream>

extern void ExitGame();

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

Game::Game() noexcept(false)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>();
    m_deviceResources->RegisterDeviceNotify(this);
    
}

Game::~Game()
{
#ifdef DXTK_AUDIO
    if (m_audEngine)
    {
        m_audEngine->Suspend();
    }
#endif
}

// Initialize the Direct3D resources required to run.
void Game::Initialize(HWND window, int width, int height)
{

	m_input.Initialise(window);

    m_deviceResources->SetWindow(window, width, height);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();


	//setup light
	m_Light.setAmbientColour(0.3f, 0.3f, 0.3f, 1.0f);
	m_Light.setDiffuseColour(1.0f, 0.0f, 0.0f, 1.0f);
	m_Light.setPosition(1.0f, 1.0f, 1.0f);
    m_Light.setDirection(-1.0f, -1.0f, 0.0f);

	//setup camera
	m_Camera01.setPosition(Vector3(0.0f, 02.0f, 8.0f));
	m_Camera01.setRotation(Vector3(-90.0f, -180.0f, 0.0f));	//orientation is -90 becuase zero will be looking up at the sky straight up. 

	
#ifdef DXTK_AUDIO
    // Create DirectXTK for Audio objects
    AUDIO_ENGINE_FLAGS eflags = AudioEngine_Default;
#ifdef _DEBUG
    eflags = eflags | AudioEngine_Debug;
#endif

    m_audEngine = std::make_unique<AudioEngine>(eflags);

    m_audioEvent = 0;
    m_audioTimerAcc = 10.f;
    m_retryDefault = false;

    m_waveBank = std::make_unique<WaveBank>(m_audEngine.get(), L"adpcmdroid.xwb");

    m_soundEffect = std::make_unique<SoundEffect>(m_audEngine.get(), L"musicmono_adpcm.wav");
    //m_soundEffect = std::make_unique<SoundEffect>(m_audEngine.get(), L"cha-la-head-cha-la.wav");
    m_effect1 = m_soundEffect->CreateInstance();
    m_effect2 = m_waveBank->CreateInstance(10);
#endif
}

#pragma region Frame Update
// Executes the basic game loop.
void Game::Tick()
{
	//take in input
	m_input.Update();								//update the hardware
	m_gameInputCommands = m_input.getGameInput();	//retrieve the input for our game
	
	//Update all game objects
    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

	//Render all game content. 
    Render();

#ifdef DXTK_AUDIO
    // Only update audio engine once per frame
    if (!m_audEngine->IsCriticalError() && m_audEngine->Update())
    {
        // Setup a retry in 1 second
        m_audioTimerAcc = 1.f;
        m_retryDefault = true;
    }
#endif

	
}

// Updates the world.
void Game::Update(DX::StepTimer const& timer)
{
	//note that currently.  Delta-time is not considered in the game object movement. 
	if (m_gameInputCommands.left)
	{
       
		Vector3 rotation = m_Camera01.getRotation();
		rotation.y += m_Camera01.getRotationSpeed() * m_timer.GetFramesPerSecond();
		m_Camera01.setRotation(rotation);
	}
	if (m_gameInputCommands.right)
	{
		Vector3 rotation = m_Camera01.getRotation();
		rotation.y  -= m_Camera01.getRotationSpeed() * m_timer.GetFramesPerSecond();
		m_Camera01.setRotation(rotation);
	}
	if (m_gameInputCommands.forward)
	{
		Vector3 position = m_Camera01.getPosition(); //get the position
		position += m_Camera01.getForward() * m_Camera01.getMoveSpeed() * m_timer.GetFramesPerSecond(); //add the forward vector
		m_Camera01.setPosition(position);
	}
	if (m_gameInputCommands.back)
	{
		Vector3 position = m_Camera01.getPosition(); //get the position
		position -= m_Camera01.getForward() * m_Camera01.getMoveSpeed() * m_timer.GetFramesPerSecond(); //add the forward vector
		m_Camera01.setPosition(position);
	}
    if (m_gameInputCommands.up)
    {
        Vector3 position = m_Camera01.getPosition();
        position -= m_Camera01.getUpwards() * m_Camera01.getMoveSpeed() * m_timer.GetFramesPerSecond(); 
        m_Camera01.setPosition(position);
    }
    if (m_gameInputCommands.down) 
    {
        Vector3 position = m_Camera01.getPosition();
        position += m_Camera01.getUpwards() * m_Camera01.getMoveSpeed() * m_timer.GetFramesPerSecond();
        m_Camera01.setPosition(position);
    }

	m_Camera01.Update();	//camera update.



	m_view = m_Camera01.getCameraMatrix();
	m_world = Matrix::Identity;

#ifdef DXTK_AUDIO
    m_audioTimerAcc -= (float)timer.GetElapsedSeconds();
    if (m_audioTimerAcc < 0)
    {
        if (m_retryDefault)
        {
            m_retryDefault = false;
            if (m_audEngine->Reset())
            {
                // Restart looping audio
                m_effect1->Play(true);
            }
        }
        else
        {
            m_audioTimerAcc = 4.f;

            m_waveBank->Play(m_audioEvent++);

            if (m_audioEvent >= 11)
                m_audioEvent = 0;
        }
    }
#endif

  
	if (m_input.Quit())
	{
		ExitGame();
	}
}
#pragma endregion

#pragma region Frame Render
// Draws the scene.
void Game::Render()
{	
    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }

    Clear();

    m_deviceResources->PIXBeginEvent(L"Render");
    auto context = m_deviceResources->GetD3DDeviceContext();

    bool isSongPlaying = false;

    // Draw Text to the screen
    m_deviceResources->PIXBeginEvent(L"Draw sprite");
    m_sprites->Begin();
		m_font->DrawString(m_sprites.get(), L"DirectXTK Demo Window", XMFLOAT2(10, 10), Colors::Yellow);
    m_sprites->End();
    m_deviceResources->PIXEndEvent();
	
	//Set Rendering states. 
	context->OMSetBlendState(m_states->Opaque(), nullptr, 0xFFFFFFFF);
	context->OMSetDepthStencilState(m_states->DepthDefault(), 0);
	context->RSSetState(m_states->CullClockwise());
    //context->RSSetState(m_states->Wireframe());


	// Turn our shaders on,  set parameters
    // prepare for sphere
    SimpleMath::Matrix sphereLocation = SimpleMath::Matrix::CreateTranslation(3.5f, 0.0f, 4.0f);
    SimpleMath::Matrix sphereSize = SimpleMath::Matrix::CreateScale(0.5);
    m_world = m_world * sphereLocation * sphereSize;
	m_BasicShaderPair.EnableShader(context);
	m_BasicShaderPair.SetShaderParameters(context, &m_world, &m_view, &m_projection, &m_Light, m_textureBall.Get());
	//render our Sphere
	m_BasicModel.Render(context);
	
    if (m_timer.GetTotalSeconds() > 12) {  
        do {
            m_effect1->Play(true);
            m_effect2->Play();
        } while (isSongPlaying == true);   // wrong condition so that the song only plays once
    }

    // prepare transform for human...............................................................
     //Every object in your scene will have it's own world matrix, which means the constant buffer containg this matrix will need to be updated PER 3D model.
    m_world = SimpleMath::Matrix::Identity;
    SimpleMath::Matrix humanPosition;
    SimpleMath::Matrix humanRotate;
    float abductionSpeed = 0.0f + m_timer.GetTotalSeconds() * 0.5;
    if (m_timer.GetTotalSeconds() > 15 && m_timer.GetTotalSeconds() < 25) {
        humanRotate = SimpleMath::Matrix::CreateRotationY(m_timer.GetFrameCount() * 0.01);
        humanPosition = SimpleMath::Matrix::CreateTranslation(0.0f, 0.0f + abductionSpeed - 7.5f, 0.5f);
    }
    else if (m_timer.GetTotalSeconds() > 25 && m_timer.GetTotalSeconds() < 30) {
        humanPosition = SimpleMath::Matrix::CreateTranslation(0.0f, 5.0f, 0.5f);
    }
    else if (m_timer.GetTotalSeconds() > 30) {
        humanPosition = SimpleMath::Matrix::CreateTranslation(0.0f, abductionSpeed - 10.0f, 0.5f);
    }else
        humanPosition = SimpleMath::Matrix::CreateTranslation(0.0f, 0.0f, 0.5f);
    SimpleMath::Matrix humanSize = SimpleMath::Matrix::CreateScale(0.5f);
    m_world = m_world * humanRotate * humanPosition * humanSize;  
    // render human 
    m_BasicShaderPair.EnableShader(context);
    m_BasicShaderPair.SetShaderParameters(context, &m_world, &m_view, &m_projection, &m_Light, m_textureHumanSkin.Get());
    m_HumanModel.Render(context);


    
    //prepare transform for AlienDrone ..........................................................
    m_world = SimpleMath::Matrix::Identity;
    SimpleMath::Matrix dronePosition;
    if (m_timer.GetTotalSeconds() < 10) {
        dronePosition = SimpleMath::Matrix::CreateTranslation(-10.0f + 1 * m_timer.GetTotalSeconds(), 2.5f, 0.0f);
    }
    else if (m_timer.GetTotalSeconds() > 30) {
        dronePosition = SimpleMath::Matrix::CreateTranslation(0.0f, 2.5f + abductionSpeed/2 - 7.5f, 0.0f);
    }
    else {
        dronePosition = SimpleMath::Matrix::CreateTranslation(0.0f, 2.5f, 0.0f);
    }
    SimpleMath::Matrix droneSize = SimpleMath::Matrix::CreateScale(1.25f);
    m_world = m_world * dronePosition;
	//setup and draw AlienDrone
	m_BasicShaderPair.EnableShader(context);
	m_BasicShaderPair.SetShaderParameters(context, &m_world, &m_view, &m_projection, &m_Light, m_textureAlienDrone.Get());
	m_droneModel.Render(context);



	//prepare transform for floor object........................................................... 
	m_world = SimpleMath::Matrix::Identity; //set world back to identity
	SimpleMath::Matrix groundPosition = SimpleMath::Matrix::CreateTranslation(0.0f, -0.6f, 0.0f);
	m_world = m_world * groundPosition;
	//setup and draw cube
	m_BasicShaderPair.EnableShader(context);
	m_BasicShaderPair.SetShaderParameters(context, &m_world, &m_view, &m_projection, &m_Light, m_textureGrass.Get());
	m_BasicModel3.Render(context);



    //Prepare transform for poolside................................................................
    m_world = SimpleMath::Matrix::Identity;
    SimpleMath::Matrix poolSidePosition1 = SimpleMath::Matrix::CreateTranslation(-2.5f, -0.2f, 0.0f);
    m_world = m_world * poolSidePosition1;
    // setup and draw poolside
    m_BasicShaderPair.EnableShader(context);
    m_BasicShaderPair.SetShaderParameters(context, &m_world, &m_view, &m_projection, &m_Light, m_textureMarbel.Get());
    m_PoolSide1Model.Render(context);

    //Prepare transform for poolside
    m_world = SimpleMath::Matrix::Identity;
    SimpleMath::Matrix poolSidePosition2 = SimpleMath::Matrix::CreateTranslation(2.5f, -0.2f, 0.0f);
    m_world = m_world * poolSidePosition2;
    // setup and draw poolside
    m_BasicShaderPair.EnableShader(context);
    m_BasicShaderPair.SetShaderParameters(context, &m_world, &m_view, &m_projection, &m_Light, m_textureMarbel.Get());
    m_PoolSide2Model.Render(context);

    //Prepare transform for poolside
    m_world = SimpleMath::Matrix::Identity;
    SimpleMath::Matrix poolSidePosition3 = SimpleMath::Matrix::CreateTranslation(0.0f, -0.2f, -3.5f);
    m_world = m_world * poolSidePosition3;
    // setup and draw poolside
    m_BasicShaderPair.EnableShader(context);
    m_BasicShaderPair.SetShaderParameters(context, &m_world, &m_view, &m_projection, &m_Light, m_textureMarbel.Get());
    m_PoolSide3Model.Render(context);

    //Prepare transform for poolside
    m_world = SimpleMath::Matrix::Identity;
    SimpleMath::Matrix poolSidePosition4 = SimpleMath::Matrix::CreateTranslation(0.0f, -0.2f, 3.5f);
    m_world = m_world * poolSidePosition4;
    // setup and draw poolside
    m_BasicShaderPair.EnableShader(context);
    m_BasicShaderPair.SetShaderParameters(context, &m_world, &m_view, &m_projection, &m_Light, m_textureMarbel.Get());
    m_PoolSide4Model.Render(context);



    //Prepare transform for poolWater..................................................
    m_world = SimpleMath::Matrix::Identity;
    SimpleMath::Matrix poolWaterPosition = SimpleMath::Matrix::CreateTranslation(0.0f, -0.4f, 0.0f);
    m_world = m_world * poolWaterPosition;
    // setup and draw poolWater
    m_BasicShaderPair.EnableShader(context);
    m_BasicShaderPair.SetShaderParameters(context, &m_world, &m_view, &m_projection, &m_Light, m_textureWater.Get());
    m_WaterModel.Render(context);



    //Prepare transform for environmentSphere..................................................
    m_world = SimpleMath::Matrix::Identity;
    SimpleMath::Matrix environmentSize = SimpleMath::Matrix::CreateScale(256);
    m_world = m_world * environmentSize;


    // setup and draw environmentSphere......................................................
    context->RSSetState(m_states->CullCounterClockwise());
    m_BasicShaderPair.EnableShader(context);
    m_BasicShaderPair.SetShaderParameters(context, &m_world, &m_view, &m_projection, &m_Light, m_textureSky.Get());
   // m_EnvironmentSphere.Render(context);
    

    if (m_timer.GetTotalSeconds() >= 12 && m_timer.GetTotalSeconds() <= 24) {   // Ambientlight blink

        if ((int)m_timer.GetTotalSeconds() % 2 <= 0) {
            m_Light.setAmbientColour(1.f, 0.0f, 0.0f, 0.0f);
            m_Light.setDiffuseColour(0.7f, 0.4f, 0.2f, 3.0f);
        }
        else
            m_Light.setAmbientColour(0.3f, 0.3f, 0.3f, 1.0f);
            m_Light.setDiffuseColour(0.7f, 0.4f, 0.2f, 3.0f);
            
    }else
        m_Light.setAmbientColour(0.3f, 0.3f, 0.3f, 1.0f);
        m_Light.setDiffuseColour(0.7f, 0.4f, 0.2f, 3.0f);
    
    if (m_timer.GetTotalSeconds() <= 10) 
    {
        m_Light.setPosition(-10.0f + 1 * m_timer.GetTotalSeconds(), 0.0f, -1.0f);  // move light with drone           
    }
    

    // Show the new frame.
    m_deviceResources->Present();
}

// Helper method to clear the back buffers.
void Game::Clear()
{
    m_deviceResources->PIXBeginEvent(L"Clear");

    // Clear the views.
    auto context = m_deviceResources->GetD3DDeviceContext();
    auto renderTarget = m_deviceResources->GetRenderTargetView();
    auto depthStencil = m_deviceResources->GetDepthStencilView();

    context->ClearRenderTargetView(renderTarget, Colors::CornflowerBlue);
    context->ClearDepthStencilView(depthStencil, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    context->OMSetRenderTargets(1, &renderTarget, depthStencil);

    // Set the viewport.
    auto viewport = m_deviceResources->GetScreenViewport();
    context->RSSetViewports(1, &viewport);

    m_deviceResources->PIXEndEvent();
}

#pragma endregion

#pragma region Message Handlers
// Message handlers
void Game::OnActivated()
{
}

void Game::OnDeactivated()
{
}

void Game::OnSuspending()
{
#ifdef DXTK_AUDIO
    m_audEngine->Suspend();
#endif
}

void Game::OnResuming()
{
    m_timer.ResetElapsedTime();

#ifdef DXTK_AUDIO
    m_audEngine->Resume();
#endif
}

void Game::OnWindowMoved()
{
    auto r = m_deviceResources->GetOutputSize();
    m_deviceResources->WindowSizeChanged(r.right, r.bottom);
}

void Game::OnWindowSizeChanged(int width, int height)
{
    if (!m_deviceResources->WindowSizeChanged(width, height))
        return;

    CreateWindowSizeDependentResources();
}

#ifdef DXTK_AUDIO
void Game::NewAudioDevice()
{
    if (m_audEngine && !m_audEngine->IsAudioDevicePresent())
    {
        // Setup a retry in 1 second
        m_audioTimerAcc = 1.f;
        m_retryDefault = true;
    }
}
#endif

// Properties
void Game::GetDefaultSize(int& width, int& height) const
{
    width = 800;
    height = 600;
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Game::CreateDeviceDependentResources()
{
    auto context = m_deviceResources->GetD3DDeviceContext();
    auto device = m_deviceResources->GetD3DDevice();

    m_states = std::make_unique<CommonStates>(device);
    m_fxFactory = std::make_unique<EffectFactory>(device);
    m_sprites = std::make_unique<SpriteBatch>(context);
    m_font = std::make_unique<SpriteFont>(device, L"SegoeUI_18.spritefont");
    m_batch = std::make_unique<PrimitiveBatch<VertexPositionColor>>(context);

    //setup our test model
    m_BasicModel.InitializeSphere(device);
    m_HumanModel.InitializeModel(device, "1.obj");
    m_droneModel.InitializeModel(device, "drone.obj");
    m_BasicModel3.InitializeBox(device, 40.0f, 0.5f, 40.0f);	//box includes dimensions
    m_PoolSide1Model.InitializeBox(device, 1.0f, 0.2f, 6.0f);
    m_PoolSide2Model.InitializeBox(device, 1.0f, 0.2f, 6.0f);
    m_PoolSide3Model.InitializeBox(device, 6.0f, 0.2f, 1.0f);
    m_PoolSide4Model.InitializeBox(device, 6.0f, 0.2f, 1.0f);
    m_WaterModel.InitializeBox(device, 4.0, 0.21f, 6.0f);
    m_EnvironmentSphere.InitializeSphere(device);

    //load and set up our Vertex and Pixel Shaders
    m_BasicShaderPair.InitStandard(device, L"light_vs.cso", L"light_ps.cso");

    //load Textures
    CreateDDSTextureFromFile(device, L"planet3.dds", nullptr, m_textureBall.ReleaseAndGetAddressOf());
    CreateDDSTextureFromFile(device, L"TextureGrass3.dds", nullptr, m_textureGrass.ReleaseAndGetAddressOf());
    CreateDDSTextureFromFile(device, L"EvilDrone_Diff.dds", nullptr, m_textureAlienDrone.ReleaseAndGetAddressOf());
    CreateDDSTextureFromFile(device, L"TextureSkinColour.dds", nullptr, m_textureHumanSkin.ReleaseAndGetAddressOf());
    CreateDDSTextureFromFile(device, L"cobble.dds", nullptr, m_textureMarbel.ReleaseAndGetAddressOf());
    CreateDDSTextureFromFile(device, L"water2.dds", nullptr, m_textureWater.ReleaseAndGetAddressOf());
    CreateDDSTextureFromFile(device, L"nightsky4.dds", nullptr, m_textureSky.ReleaseAndGetAddressOf());
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    auto size = m_deviceResources->GetOutputSize();
    float aspectRatio = float(size.right) / float(size.bottom);
    float fovAngleY = 70.0f * XM_PI / 180.0f;

    // This is a simple example of change that can be made when the app is in
    // portrait or snapped view.
    if (aspectRatio < 1.0f)
    {
        fovAngleY *= 2.0f;
    }

    // This sample makes use of a right-handed coordinate system using row-major matrices.
    m_projection = Matrix::CreatePerspectiveFieldOfView(
        fovAngleY,
        aspectRatio,
        0.01f,
        100.0f
    );
}


void Game::OnDeviceLost()
{
    m_states.reset();
    m_fxFactory.reset();
    m_sprites.reset();
    m_font.reset();
	m_batch.reset();
	m_testmodel.reset();
    m_batchInputLayout.Reset();
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion
