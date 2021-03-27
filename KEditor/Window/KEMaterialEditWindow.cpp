#include "KEMaterialEditWindow.h"
#include "KBase/Interface/Entity/IKEntityManager.h"
#include "KBase/Interface/Component/IKRenderComponent.h"
#include "KBase/Interface/Component/IKTransformComponent.h"
#include <QToolBar>
#include <QComboBox>

KEMaterialEditWindow::KEMaterialEditWindow(QWidget *parent)
	: QMainWindow(parent),
	m_MainWindow(parent),
	m_RenderWidget(nullptr),
	m_PropertyWidget(nullptr),
	m_PropertyDockWidget(nullptr),
	m_FileSys(nullptr),
	m_EntityIdx(0)
{
	Init();
	setAttribute(Qt::WA_DeleteOnClose, true);
}

KEMaterialEditWindow::~KEMaterialEditWindow()
{
	UnInit();
}

QSize KEMaterialEditWindow::sizeHint() const
{
	assert(m_MainWindow);
	int width = m_MainWindow->width() * 2 / 3;
	int height = m_MainWindow->height() * 2 / 3;
	return QSize(width, height);
}

void KEMaterialEditWindow::resizeEvent(QResizeEvent* event)
{
}

struct ComboPreviewInfo
{
	const char* item;
	const char* path;
	bool asset;
};

const ComboPreviewInfo PREVIEW_INFO[] =
{
	{ "Sphere", "Models/sphere.obj", true},
	{ "Plane", "Models/plane.obj", true},
	{ "Torusknot", "Models/torusknot.obj", true},
	{ "Venus", "Models/venus.fbx", true,}
};

bool KEMaterialEditWindow::InitToolBar()
{
	QToolBar* toolBar = KNEW QToolBar("MaterialToolBar", this);
	addToolBar(Qt::TopToolBarArea, toolBar);

	QComboBox* previewCombo = KNEW QComboBox(toolBar);
	previewCombo->setMinimumWidth(70);

	toolBar->addWidget(previewCombo);

	QAction* saveAction = KNEW QAction(QIcon(":/images/save.png"), "Save", toolBar);
	toolBar->addAction(saveAction);

	QAction* reloadAction = KNEW QAction(QIcon(":/images/reload.png"), "Reload", toolBar);
	toolBar->addAction(reloadAction);

	QObject::connect(saveAction, &QAction::triggered, [this](bool checked)
	{
		OnSave();
	});

	QObject::connect(reloadAction, &QAction::triggered, [this](bool checked)
	{
		OnReload();
	});

	for (const ComboPreviewInfo& info : PREVIEW_INFO)
	{
		previewCombo->addItem(info.item);
	}

	QObject::connect(previewCombo, &QComboBox::currentTextChanged, [&, this](const QString& text)
	{
		m_EntityIdx = 0;
		std::string stdString = text.toStdString();
		for (int32_t i = 0; i < ARRAY_SIZE(PREVIEW_INFO); ++i)
		{
			if (strcmp(PREVIEW_INFO[i].item, stdString.c_str()) == 0)
			{
				m_EntityIdx = i;
				break;
			}
		}
		RefreshPreview();
	});

	return true;
}

bool KEMaterialEditWindow::Init()
{
	UnInit();

	InitToolBar();

	m_PropertyDockWidget = KNEW QDockWidget();
	addDockWidget(Qt::RightDockWidgetArea, m_PropertyDockWidget);

	m_RenderWidget = KNEW KEMaterialRenderWidget();
	m_RenderWidget->Init(KEngineGlobal::Engine);
	setCentralWidget(m_RenderWidget);

	m_PropertyWidget = KNEW KEMaterialPropertyWidget(this);
	m_PropertyDockWidget->setWidget(m_PropertyWidget);

	IKRenderCore* renderCore = KEngineGlobal::Engine->GetRenderCore();
	IKRenderDispatcher* renderer = renderCore->GetRenderDispatcher();
	renderer->SetCallback(m_RenderWidget->GetRenderWindow(), &m_OnRenderCallBack);

	m_MiniScene = CreateRenderScene();
	m_MiniScene->Init(SCENE_MANGER_TYPE_OCTREE, 100000.0f, glm::vec3(0.0f));

	m_OnRenderCallBack = [this](IKRenderDispatcher* dispatcher, uint32_t chainImageIndex, uint32_t frameIndex)
	{
		m_CameraController->Update();
		dispatcher->SetSceneCamera(m_MiniScene.get(), &m_MiniCamera);
		dispatcher->SetCameraCubeDisplay(false);
	};

	m_MiniCamera.SetNear(1.0f);
	m_MiniCamera.SetFar(2500.0f);
	m_MiniCamera.SetCustomLockYAxis(glm::vec3(0.0f, 1.0f, 0.0f));
	m_MiniCamera.SetLockYEnable(true);

	m_CameraController = CreateCameraPreviewController();
	m_CameraController->Init(&m_MiniCamera, m_RenderWidget->GetRenderWindow());

	return true;
}

bool KEMaterialEditWindow::UnInit()
{
	SAFE_UNINIT(m_CameraController);

	SAFE_UNINIT(m_PropertyWidget);
	SAFE_DELETE(m_PropertyDockWidget);

	setCentralWidget(nullptr);
	if (m_RenderWidget)
	{
		IKRenderCore* renderCore = KEngineGlobal::Engine->GetRenderCore();
		IKRenderDispatcher* renderer = renderCore->GetRenderDispatcher();
		renderer->RemoveCallback(m_RenderWidget->GetRenderWindow());
		m_RenderWidget->UnInit();
	}

	SAFE_UNINIT(m_MiniScene);
	SAFE_UNINIT(m_RenderWidget);

	if (m_PreviewEntity)
	{
		m_PreviewEntity->UnRegisterAllComponent();
		KECS::EntityManager->ReleaseEntity(m_PreviewEntity);
		m_PreviewEntity = nullptr;
	}

	return true;
}

bool KEMaterialEditWindow::RefreshPreview()
{
	if (m_EntityIdx >= 0)
	{
		const ComboPreviewInfo& previewItem = PREVIEW_INFO[m_EntityIdx];

		if (!m_PreviewEntity)
		{
			m_PreviewEntity = KECS::EntityManager->CreateEntity();
			m_PreviewEntity->RegisterComponent(CT_RENDER);
			m_PreviewEntity->RegisterComponent(CT_TRANSFORM);
		}

		m_MiniScene->Remove(m_PreviewEntity);

		IKRenderComponent* renderComponent = nullptr;
		if (m_PreviewEntity->GetComponent(CT_RENDER, &renderComponent))
		{
			IKMaterialParameterPtr vsPreParameter = nullptr;
			IKMaterialParameterPtr fsPreParameter = nullptr;
			IKMaterialTextureBindingPtr preTextureBinding = nullptr;

			// 拷贝过去的参数
			{
				IKMaterialPtr material = renderComponent->GetMaterial();
				if (material)
				{
					IKMaterialParameterPtr vsParameter = material->GetVSParameter();
					if (vsParameter)
					{
						vsParameter->Duplicate(vsPreParameter);
					}

					IKMaterialParameterPtr fsParameter = material->GetFSParameter();
					if (fsParameter)
					{
						fsParameter->Duplicate(fsPreParameter);
					}

					IKMaterialTextureBindingPtr textureBinding = material->GetDefaultMaterialTexture();
					if (textureBinding)
					{
						textureBinding->Duplicate(preTextureBinding);
					}
				}
			}

			renderComponent->UnInit();
			if (previewItem.asset)
			{
				renderComponent->SetAssetPath(previewItem.path);
			}
			else
			{
				renderComponent->SetMeshPath(previewItem.path);
			}

			renderComponent->SetUseMaterialTexture(true);

			if (!m_MaterialPath.empty())
			{
				renderComponent->SetMaterialPath(m_MaterialPath.c_str());
				renderComponent->Init(false);

				IKMaterialPtr material = renderComponent->GetMaterial();
				ASSERT_RESULT(material);

				// 复制过去的参数
				{
					IKMaterialParameterPtr vsParameter = material->GetVSParameter();
					if (vsParameter)
					{
						vsParameter->Paste(vsPreParameter);
					}
					IKMaterialParameterPtr fsParameter = material->GetFSParameter();
					if (fsParameter)
					{
						fsParameter->Paste(fsPreParameter);
					}
					IKMaterialTextureBindingPtr textureBinding = material->GetDefaultMaterialTexture();
					if (textureBinding)
					{
						textureBinding->Paste(preTextureBinding);
					}
				}

				m_PropertyWidget->Init(material ? material.get() : nullptr);

				KAABBBox bound;

				m_PreviewEntity->GetBound(bound);
				float extend = glm::length(bound.GetExtend());
				constexpr float INITIAL_EXTEND = 68.0f;
				float scale = INITIAL_EXTEND / extend;

				IKTransformComponent* transformComponent = nullptr;
				if (m_PreviewEntity->GetComponent(CT_TRANSFORM, &transformComponent))
				{
					transformComponent->SetScale(transformComponent->GetScale() * glm::vec3(scale));
				}

				m_PreviewEntity->GetBound(bound);
				m_CameraController->SetPreviewCenter(bound.GetCenter());

				m_MiniScene->Add(m_PreviewEntity);

				if (preTextureBinding)
				{
					preTextureBinding->Clear();
				}

				return true;
			}
		}
	}
	return false;
}

bool KEMaterialEditWindow::SetEditTarget(IKFileSystem* fileSys, const std::string& path)
{
	setWindowTitle(path.c_str());
	m_FileSys = fileSys;
	m_MaterialPath = path;
	RefreshPreview();
	return true;
}

bool KEMaterialEditWindow::OnSave()
{
	IKRenderComponent* renderComponent = nullptr;
	if (m_PreviewEntity->GetComponent(CT_RENDER, &renderComponent))
	{
		IKMaterialPtr material = renderComponent->GetMaterial();
		ASSERT_RESULT(material);

		std::string fullPath;
		m_FileSys->FullPath(m_MaterialPath, fullPath);
		material->SaveAsFile(fullPath);

		return true;
	}
	return false;
}

bool KEMaterialEditWindow::OnReload()
{
	IKRenderComponent* renderComponent = nullptr;
	if (m_PreviewEntity->GetComponent(CT_RENDER, &renderComponent))
	{
		IKMaterialPtr material = renderComponent->GetMaterial();
		ASSERT_RESULT(material);
		m_PropertyWidget->UnInit();
		material->Reload();
		m_PropertyWidget->Init(material.get());
		renderComponent->ReloadMaterial();
		return true;
	}
	return false;
}