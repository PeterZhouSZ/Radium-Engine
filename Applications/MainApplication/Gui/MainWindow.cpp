#include <Gui/MainWindow.hpp>

#include <QSettings>
#include <QFileDialog>
#include <QToolButton>

#include <Core/File/deprecated/OBJFileManager.hpp>
#include <assimp/Importer.hpp>

#include <Engine/Managers/SignalManager/SignalManager.hpp>
#include <Engine/Managers/EntityManager/EntityManager.hpp>

#include <Engine/Renderer/Mesh/Mesh.hpp>
#include <Engine/Renderer/RenderObject/RenderObject.hpp>
#include <Engine/Renderer/RenderObject/RenderObjectManager.hpp>
#include <Engine/Renderer/RenderTechnique/RenderTechnique.hpp>
#include <Engine/Renderer/RenderTechnique/ShaderConfigFactory.hpp>

#include <Engine/Entity/Entity.hpp>

#include <GuiBase/TreeModel/EntityTreeModel.hpp>
#include <GuiBase/Utils/KeyMappingManager.hpp>
#include <GuiBase/Viewer/CameraInterface.hpp>

#include <PluginBase/RadiumPluginInterface.hpp>

#include <Gui/MaterialEditor.hpp>

#include <MainApplication.hpp>

using Ra::Engine::ItemEntry;

namespace Ra
{

    Gui::MainWindow::MainWindow(QWidget* parent)
            : QMainWindow(parent)
    {
        // Note : at this point most of the components (including the Engine) are
        // not initialized. Listen to the "started" signal.

        setupUi(this);

        setWindowIcon(QPixmap(":/Assets/Images/RadiumIcon.png"));
        setWindowTitle(QString("Radium Engine"));

        QStringList headers;
        headers << tr("Entities -> Components");
        m_itemModel = new GuiBase::ItemModel(mainApp->getEngine(), this);
        m_entitiesTreeView->setModel(m_itemModel);
        m_materialEditor = new MaterialEditor();
        m_selectionManager = new GuiBase::SelectionManager(m_itemModel, this);
        m_entitiesTreeView->setSelectionModel(m_selectionManager);

        createConnections();

        m_vertexInfo->setVisible(false);
        m_edgeInfo->setVisible(false);
        m_triangleInfo->setVisible(false);

        mainApp->framesCountForStatsChanged((uint) m_avgFramesCount->value());
    }

    Gui::MainWindow::~MainWindow()
    {
        // Child QObjects will automatically be deleted
    }

    void Gui::MainWindow::cleanup()
    {
        m_viewer->getGizmoManager()->cleanup();
    }

    void Gui::MainWindow::createConnections()
    {
        connect(actionOpenMesh, &QAction::triggered, this, &MainWindow::loadFile);
        connect(actionReload_Shaders, &QAction::triggered, m_viewer, &Viewer::reloadShaders);
        connect(actionOpen_Material_Editor, &QAction::triggered, this, &MainWindow::openMaterialEditor);

        // Toolbox setup
        connect(actionToggle_Local_Global, &QAction::toggled, m_viewer->getGizmoManager(), &GizmoManager::setLocal);
        connect(actionGizmoOff, &QAction::triggered, this, &MainWindow::gizmoShowNone);
        connect(actionGizmoTranslate, &QAction::triggered, this, &MainWindow::gizmoShowTranslate);
        connect(actionGizmoRotate, &QAction::triggered, this, &MainWindow::gizmoShowRotate);

        connect(actionRecord_Frames, &QAction::toggled, mainApp, &MainApplication::setRecordFrames);

        connect(actionReload_configuration, &QAction::triggered, this, &MainWindow::reloadConfiguration);
        connect(actionLoad_configuration_file, &QAction::triggered, this, &MainWindow::loadConfiguration);

        // Loading setup.
        connect(this, &MainWindow::fileLoading, mainApp, &BaseApplication::loadFile);

        // Connect picking results (TODO Val : use events to dispatch picking directly)
        connect(m_viewer, &Viewer::rightClickPicking, this, &MainWindow::handlePicking);
        connect(m_viewer, &Viewer::leftClickPicking, m_viewer->getGizmoManager(), &GizmoManager::handlePickingResult);

        connect(m_avgFramesCount, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
                mainApp, &BaseApplication::framesCountForStatsChanged);
        connect(mainApp, &BaseApplication::updateFrameStats, this, &MainWindow::onUpdateFramestats);

        // Inform property editors of new selections
        connect(m_selectionManager, &GuiBase::SelectionManager::selectionChanged, this, &MainWindow::onSelectionChanged);
        //connect(this, &MainWindow::selectedItem, tab_edition, &TransformEditorWidget::setEditable);

        // Make selected item event visible to plugins
        connect(this, &MainWindow::selectedItem, mainApp, &MainApplication::onSelectedItem);
        connect(this, &MainWindow::selectedItem, m_viewer->getGizmoManager(), &GizmoManager::setEditable);
        connect(this, &MainWindow::selectedItem, m_viewer->getGizmoManager(), &GizmoManager::setEditable);

        // Enable changing shaders
        connect(m_currentShaderBox, static_cast<void (QComboBox::*)(const QString&)>( &QComboBox::currentIndexChanged ),
                this, &MainWindow::changeRenderObjectShader);

        // RO Stuff
        connect(m_toggleRenderObjectButton, &QPushButton::clicked, this, &MainWindow::toggleVisisbleRO);
        connect(m_editRenderObjectButton, &QPushButton::clicked, this, &MainWindow::editRO);
        connect(m_exportMeshButton, &QPushButton::clicked, this, &MainWindow::exportCurrentMesh);
        connect(m_removeEntityButton, &QPushButton::clicked, this, &MainWindow::deleteCurrentItem);
        connect(m_clearSceneButton, &QPushButton::clicked, this, &MainWindow::resetScene);
        connect(m_fitCameraButton, &QPushButton::clicked, this, &MainWindow::fitCamera);

        // Renderer stuff

        //connect(m_currentRendererCombo,
        //        static_cast<void (QComboBox::*)(const QString&)>( &QComboBox::currentIndexChanged ),
        //        this, &MainWindow::changeRenderer);

        connect(m_viewer, &Viewer::rendererReady, this, &MainWindow::onRendererReady);

        connect(m_displayedTextureCombo,
                static_cast<void (QComboBox::*)(const QString&)>( &QComboBox::currentIndexChanged ),
                m_viewer, &Viewer::displayTexture);

        connect(m_enablePostProcess, &QCheckBox::stateChanged, m_viewer, &Viewer::enablePostProcess);
        connect(m_enableDebugDraw,   &QCheckBox::stateChanged, m_viewer, &Viewer::enableDebugDraw);
        connect(m_realFrameRate,     &QCheckBox::stateChanged, mainApp,  &BaseApplication::setRealFrameRate);

        connect(m_printGraph,       &QCheckBox::stateChanged, mainApp,  &BaseApplication::setRecordGraph);
        connect(m_printTimings,     &QCheckBox::stateChanged, mainApp,  &BaseApplication::setRecordTimings);

        // Connect engine signals to the appropriate callbacks
        std::function<void(const Engine::ItemEntry&)> add = std::bind(&MainWindow::onItemAdded, this, std::placeholders::_1);
        std::function<void(const Engine::ItemEntry&)> del = std::bind(&MainWindow::onItemRemoved, this, std::placeholders::_1);
        mainApp->m_engine->getSignalManager()->m_entityCreatedCallbacks.push_back(add);
        mainApp->m_engine->getSignalManager()->m_entityDestroyedCallbacks.push_back(del);

        mainApp->m_engine->getSignalManager()->m_componentAddedCallbacks.push_back(add);
        mainApp->m_engine->getSignalManager()->m_componentRemovedCallbacks.push_back(del);

        mainApp->m_engine->getSignalManager()->m_roAddedCallbacks.push_back(add);
        mainApp->m_engine->getSignalManager()->m_roRemovedCallbacks.push_back(del);

    }

    void Gui::MainWindow::loadFile()
    {
        // Filter the files
        aiString extList;
        Assimp::Importer importer;
        importer.GetExtensionList(extList);
        std::string extListStd(extList.C_Str());
        std::replace(extListStd.begin(), extListStd.end(), ';', ' ');
        QString filter =""/* QString::fromStdString(extListStd)*/;

        QSettings settings;
        QString path = settings.value("files/load", QDir::homePath()).toString();
        path = QFileDialog::getOpenFileName(this, "Open File", path, filter);
        if (path.size() > 0)
        {
            settings.setValue("files/load", path);
            emit fileLoading(path);
        }
    }

    void Gui::MainWindow::onUpdateFramestats(const std::vector<FrameTimerData>& stats)
    {
        QString framesA2B = QString("Frames #%1 to #%2 stats :")
                .arg(stats.front().numFrame).arg(stats.back().numFrame);
        m_frameA2BLabel->setText(framesA2B);


        auto romgr = mainApp->m_engine->getRenderObjectManager();

        uint polycount = romgr->getNumFaces();
        uint vertexcount = romgr->getNumVertices();

        QString polyCountText = QString("Rendering %1 faces and %2 vertices").arg(polycount).arg(vertexcount);
        m_labelCount->setText(polyCountText);

        long sumEvents = 0;
        long sumRender = 0;
        long sumTasks = 0;
        long sumFrame = 0;
        long sumInterFrame = 0;

        for (uint i = 0; i < stats.size(); ++i)
        {
            sumEvents += Core::Timer::getIntervalMicro(stats[i].eventsStart, stats[i].eventsEnd);
            sumRender += Core::Timer::getIntervalMicro(stats[i].renderData.renderStart, stats[i].renderData.renderEnd);
            sumTasks += Core::Timer::getIntervalMicro(stats[i].tasksStart, stats[i].tasksEnd);
            sumFrame += Core::Timer::getIntervalMicro(stats[i].frameStart, stats[i].frameEnd);

            if (i > 0)
            {
                sumInterFrame += Core::Timer::getIntervalMicro(stats[i - 1].frameEnd, stats[i].frameEnd);
            }
        }

        const uint N(stats.size());
        const Scalar T(N * 1000000.f);

        m_eventsTime->setNum(int(sumEvents / N));
        m_eventsUpdates->setNum(int(T / Scalar(sumEvents)));
        m_renderTime->setNum(int(sumRender / N));
        m_renderUpdates->setNum(int(T / Scalar(sumRender)));
        m_tasksTime->setNum(int(sumTasks / N));
        m_tasksUpdates->setNum(int(T / Scalar(sumTasks)));
        m_frameTime->setNum(int(sumFrame / N));
        m_frameUpdates->setNum(int(T / Scalar(sumFrame)));
        m_avgFramerate->setNum(int((N - 1) * Scalar(1000000.0 / sumInterFrame)));
    }

    Gui::Viewer* Gui::MainWindow::getViewer()
    {
        return m_viewer;
    }

    GuiBase::SelectionManager* Gui::MainWindow::getSelectionManager()
    {
        return m_selectionManager;
    }

    void Gui::MainWindow::handlePicking(int pickingResult)
    {
        Ra::Core::Index roIndex(pickingResult);
        Ra::Engine::RadiumEngine* engine = Ra::Engine::RadiumEngine::getInstance();
        if (roIndex.isValid())
        {
            auto ro = engine->getRenderObjectManager()->getRenderObject(roIndex);
            if ( ro->getType() != Ra::Engine::RenderObjectType::UI)
            {
                Ra::Engine::Component* comp = ro->getComponent();
                Ra::Engine::Entity* ent = comp->getEntity();
                const auto& fdata = m_viewer->getFeaturePickingManager()->getFeatureData();

                if (fdata.m_featureType == Ra::Engine::Renderer::VERTEX)
                {
                    m_vertexIdx->setValue(fdata.m_data[0]);
                    m_vertexIdx->setMaximum(ro->getMesh()->getGeometry().m_vertices.size() - 1);
                }
                if (fdata.m_featureType == Ra::Engine::Renderer::TRIANGLE)
                {
                    m_triangleIdx->setValue(fdata.m_data[0]);
                    m_triangleIdx->setMaximum(ro->getMesh()->getGeometry().m_triangles.size() - 1);
                }

                // For now we don't enable group selection.
                m_selectionManager->setCurrentEntry( ItemEntry(ent, comp, roIndex, fdata),
                                                    QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Current);
            }
        }
        else
        {
            m_selectionManager->clear();
        }
    }

    void Gui::MainWindow::onSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
    {
        if (m_selectionManager->hasSelection())
        {
            const ItemEntry& ent = m_selectionManager->currentItem();
            emit selectedItem(ent);
            m_selectedItemName->setText(QString::fromStdString(getEntryName(mainApp->getEngine(), ent)));
            m_editRenderObjectButton->setEnabled(false);

            if (ent.isRoNode())
            {
                m_editRenderObjectButton->setEnabled(true);

                m_materialEditor->changeRenderObject(ent.m_roIndex);
                const std::string& shaderName = mainApp->m_engine->getRenderObjectManager()
                                                       ->getRenderObject(ent.m_roIndex)
                                                       ->getRenderTechnique()
                                                       ->getBasicConfiguration().m_name;


                if (m_currentShaderBox->findText(shaderName.c_str()) == -1)
                {
                    m_currentShaderBox->addItem(QString(shaderName.c_str()));
                    m_currentShaderBox->setCurrentText(shaderName.c_str());
                }
                else
                {
                    m_currentShaderBox->setCurrentText(shaderName.c_str());
                }
            }
        }
        else
        {
            emit selectedItem(ItemEntry());
            m_selectedItemName->setText("");
            m_editRenderObjectButton->setEnabled(false);
            m_materialEditor->hide();
        }
    }

    void Gui::MainWindow::closeEvent(QCloseEvent* event)
    {
        emit closed();
        event->accept();
    }

    void Gui::MainWindow::gizmoShowNone()
    {
        m_viewer->getGizmoManager()->changeGizmoType(GizmoManager::NONE);
    }

    void Gui::MainWindow::gizmoShowTranslate()
    {
        m_viewer->getGizmoManager()->changeGizmoType(GizmoManager::TRANSLATION);
    }

    void Gui::MainWindow::gizmoShowRotate()
    {
        m_viewer->getGizmoManager()->changeGizmoType(GizmoManager::ROTATION);
    }

    void Gui::MainWindow::reloadConfiguration()
    {
        KeyMappingManager::getInstance()->reloadConfiguration();
    }

    void Gui::MainWindow::loadConfiguration()
    {
        QSettings settings;
        QString path = settings.value("configs/load", QDir::homePath()).toString();
        path = QFileDialog::getOpenFileName(this, "Open Configuration File", path, "Configuration file (*.xml)");

        if (path.size() > 0)
        {
            settings.setValue("configs/load", path);
            KeyMappingManager::getInstance()->loadConfiguration( path.toStdString().c_str() );
        }
    }

    void Gui::MainWindow::changeRenderer(const QString& rendererName)
    {
        // m_viewer->changeRenderer(m_currentRendererCombo->currentIndex());
    }


    void Gui::MainWindow::changeRenderObjectShader(const QString& shaderName)
    {
        std::string name = shaderName.toStdString();
        if (name == "")
        {
            return;
        }

        const ItemEntry& item = m_selectionManager->currentItem();
        const Engine::ShaderConfiguration config = Ra::Engine::ShaderConfigurationFactory::getConfiguration(name);

        auto vector_of_ros = getItemROs( mainApp->m_engine.get(), item );
        for (const auto& ro_index : vector_of_ros) {
            const auto& ro = mainApp->m_engine->getRenderObjectManager()->getRenderObject(ro_index);
            if (ro->getRenderTechnique()->getBasicConfiguration().m_name != name)
            {
            	ro->getRenderTechnique()->changeShader(config);
            }
        }
    }

    void Gui::MainWindow::toggleVisisbleRO()
    {
        const ItemEntry& item = m_selectionManager->currentItem();
        // If at least one RO is visible, turn them off.
        bool hasVisible = false;
        for (auto roIdx : getItemROs(mainApp->m_engine.get(), item))
        {
            if (mainApp->m_engine->getRenderObjectManager()->getRenderObject(roIdx)->isVisible())
            {
                hasVisible = true;
                break;
            }
        }
        for (auto roIdx : getItemROs(mainApp->m_engine.get(), item))
        {
            mainApp->m_engine->getRenderObjectManager()->getRenderObject(roIdx)->setVisible(!hasVisible);
        }
    }

    void Gui::MainWindow::editRO()
    {
        ItemEntry item = m_selectionManager->currentItem();
        if (item.isRoNode())
        {
            m_materialEditor->changeRenderObject(item.m_roIndex);
            m_materialEditor->show();
        }
    }

    void Gui::MainWindow::openMaterialEditor()
    {
        m_materialEditor->show();
    }

    void Gui::MainWindow::updateUi(Plugins::RadiumPluginInterface* plugin)
    {
        QString tabName;

        // Add menu
        if (plugin->doAddMenu())
        {
            QMainWindow::menuBar()->addMenu(plugin->getMenu());
        }

        // Add widget
        if (plugin->doAddWidget(tabName))
        {
            toolBox->addTab(plugin->getWidget(), tabName);
        }

        // Add actions
        int nbActions;
        if (plugin->doAddAction( nbActions ))
        {
            for (int i=0; i<nbActions; ++i)
            {
                toolBar->insertAction( 0, plugin->getAction(i) );
            }
            toolBar->addSeparator();
        }

        // Add feature widget
        if (plugin->doAddFeatureTrackingWidget())
        {
            tab_tracking_layout->addWidget( plugin->getFeatureTrackingWidget() );
        }
    }

    void Gui::MainWindow::onRendererReady()
    {
        m_viewer->getCameraInterface()->resetCamera();

        QSignalBlocker blockTextures(m_displayedTextureCombo);

        auto texs = m_viewer->getRenderer()->getAvailableTextures();
        for (const auto& tex : texs)
        {
            m_displayedTextureCombo->addItem(tex.c_str());
        }
    }

    void Gui::MainWindow::onFrameComplete()
    {
        tab_edition->updateValues();
        m_viewer->getGizmoManager()->updateValues();
        updateTrackedFeatureInfo();
    }

    void Gui::MainWindow::onItemAdded(const Engine::ItemEntry& ent)
    {
        m_itemModel->addItem(ent);
    }

    void Gui::MainWindow::onItemRemoved(const Engine::ItemEntry& ent)
    {
        m_itemModel->removeItem(ent) ;
    }

    void Gui::MainWindow::exportCurrentMesh()
    {
        std::string filename;
        Ra::Core::StringUtils::stringPrintf(filename, "radiummesh_%06u", mainApp->getFrameCount());
        ItemEntry e = m_selectionManager->currentItem();

        // For now we only export a mesh if the selected entry is a render object.
        // There could be a virtual method to get a mesh representation for any object.
        if (e.isRoNode())
        {
            Ra::Core::OBJFileManager obj;
            auto ro = Engine::RadiumEngine::getInstance()->getRenderObjectManager()->getRenderObject(e.m_roIndex);
            Ra::Core::TriangleMesh mesh = ro->getMesh()->getGeometry();
            bool result = obj.save( filename, mesh );
            if (result)
            {
                LOG(logINFO)<<"Mesh from "<<ro->getName()<<" successfully exported to "<<filename;
            }
            else
            {
                LOG(logERROR)<<"Mesh from "<<ro->getName()<<"failed to export";
            }
        }
        else
        {
            LOG(logWARNING)<< "Current entry was not a render object. No mesh was exported.";
        }
    }

    void Gui::MainWindow::deleteCurrentItem()
    {
        ItemEntry e = m_selectionManager->currentItem();

        // This call is very important to avoid a potential race condition
        // which happens if an object is selected while a gizmo is present.
        // If we do not do this, the removal of the object will call ItemModel::removeItem() which
        // will cause it to be unselected by the selection model. This in turn will cause
        // the gizmos ROs to disappear, but the RO mutex is already acquired by the call for
        // the object we want to delete, which causes a deadlock.
        // Clearing the selection before deleting the object will avoid this problem.
        m_selectionManager->clear();
        if (e.isRoNode())
        {
            e.m_component->removeRenderObject(e.m_roIndex);
        }
        else if (e.isComponentNode())
        {
            e.m_entity->removeComponent(e.m_component->getName());
        }
        else if (e.isEntityNode())
        {
            Engine::RadiumEngine::getInstance()->getEntityManager()->removeEntity(e.m_entity->idx);
        }
    }

    void Gui::MainWindow::resetScene()
    {
        // To see why this call is important, please see deleteCurrentItem().
        m_selectionManager->clearSelection();
        Engine::RadiumEngine::getInstance()->getEntityManager()->deleteEntities();
        m_viewer->resetCamera();
    }

    void Gui::MainWindow::on_actionForward_triggered()
    {
        changeRenderer("Forward");
        actionForward->setChecked( true );
        actionDeferred->setChecked( false );
        actionExperimental->setChecked( false );
    }

    void Gui::MainWindow::on_actionDeferred_triggered()
    {
        changeRenderer("Deferred");
        actionForward->setChecked( false );
        actionDeferred->setChecked( true );
        actionExperimental->setChecked( false );
    }

    void Ra::Gui::MainWindow::on_actionExperimental_triggered()
    {
        changeRenderer("Experimental");
        actionForward->setChecked( false );
        actionDeferred->setChecked( false );
        actionDeferred->setChecked( true );
    }

    void Gui::MainWindow::fitCamera()
    {
        m_viewer->fitCameraToScene(Engine::RadiumEngine::getInstance()->getRenderObjectManager()->getSceneAabb());
    }

    void Ra::Gui::MainWindow::on_m_vertexIdx_valueChanged(int arg1)
    {
        m_viewer->getFeaturePickingManager()->setVertexIndex(arg1);
        m_viewer->getFeaturePickingManager()->setSpherePosition();
    }

    void Ra::Gui::MainWindow::on_m_triangleIdx_valueChanged(int arg1)
    {
        m_viewer->getFeaturePickingManager()->setTriangleIndex(arg1);
        m_viewer->getFeaturePickingManager()->setSpherePosition();
    }

    void Gui::MainWindow::updateTrackedFeatureInfo()
    {
        m_vertexInfo->setVisible(false);
        m_edgeInfo->setVisible(false);
        m_triangleInfo->setVisible(false);
        auto fdata = m_viewer->getFeaturePickingManager()->getFeatureData();
        switch (fdata.m_featureType)
        {
        case Engine::Renderer::VERTEX:
        {
            m_vertexInfo->setVisible(true);
            m_vertexPX->setText(QString::number(m_viewer->getFeaturePickingManager()->getFeaturePosition()[0]));
            m_vertexPY->setText(QString::number(m_viewer->getFeaturePickingManager()->getFeaturePosition()[1]));
            m_vertexPZ->setText(QString::number(m_viewer->getFeaturePickingManager()->getFeaturePosition()[2]));
            m_vertexNX->setText(QString::number(m_viewer->getFeaturePickingManager()->getFeatureVector()[0]));
            m_vertexNY->setText(QString::number(m_viewer->getFeaturePickingManager()->getFeatureVector()[1]));
            m_vertexNZ->setText(QString::number(m_viewer->getFeaturePickingManager()->getFeatureVector()[2]));
            break;
        }
        case Engine::Renderer::EDGE:
        {
            m_edgeInfo->setVisible(true);
            m_edgeV0->setText(QString::number(fdata.m_data[0]));
            m_edgeV1->setText(QString::number(fdata.m_data[1]));
            break;
        }
        case Engine::Renderer::TRIANGLE:
        {
            m_triangleInfo->setVisible(true);
            m_triangleV0->setText(QString::number(fdata.m_data[1]));
            m_triangleV1->setText(QString::number(fdata.m_data[2]));
            m_triangleV2->setText(QString::number(fdata.m_data[3]));
            break;
        }
        default:
            break;
        }
        m_viewer->getFeaturePickingManager()->setSpherePosition();
    }

} // namespace Ra
