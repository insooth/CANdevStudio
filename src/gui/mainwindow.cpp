
#include "log.hpp"
#include "mainwindow.h"
#include "modelvisitor.h"  // apply_model_visitor
#include "ui_mainwindow.h"

#include <QCloseEvent>
#include <QtCore/QFile>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMdiArea>
#include <QtWidgets/QMdiSubWindow>
#include <QtWidgets/QMessageBox>

#include <cassert> // assert
#include <iostream>

#include <candevice/candevicemodel.h>
#include <canrawsender/canrawsendermodel.h>
#include <canrawview/canrawviewmodel.h>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(std::make_unique<Ui::MainWindow>())
{
    ui->setupUi(this);
    ui->centralWidget->layout()->setContentsMargins(0, 0, 0, 0);

    auto modelRegistry = std::make_shared<QtNodes::DataModelRegistry>();
    modelRegistry->registerModel<CanDeviceModel>();
    modelRegistry->registerModel<CanRawSenderModel>();
    modelRegistry->registerModel<CanRawViewModel>();

    graphScene = std::make_shared<QtNodes::FlowScene>(modelRegistry);

    connect(graphScene.get(), &QtNodes::FlowScene::nodeCreated, this, &MainWindow::nodeCreatedCallback);
    connect(graphScene.get(), &QtNodes::FlowScene::nodeDeleted, this, &MainWindow::nodeDeletedCallback);
    connect(graphScene.get(), &QtNodes::FlowScene::nodeDoubleClicked, this, &MainWindow::nodeDoubleClickedCallback);

    setupMdiArea();
    connectToolbarSignals();
    connectMenuSignals();
}

MainWindow::~MainWindow() { delete graphView; }

void MainWindow::closeEvent(QCloseEvent* e)
{
    QMessageBox::StandardButton userReply;
    userReply = QMessageBox::question(
        this, "Exit", "Are you sure you want to quit CANdevStudio?", QMessageBox::Yes | QMessageBox::No);
    if (userReply == QMessageBox::Yes) {
        QApplication::quit();
    } else {
        e->ignore();
    }
}

void MainWindow::nodeCreatedCallback(QtNodes::Node& node)
{
    auto dataModel = node.nodeDataModel();

    assert(nullptr != dataModel);

    apply_model_visitor(*dataModel
        , [this, dataModel](CanRawViewModel& m)
          {
            auto rawView = &m.canRawView;
            auto widget = rawView->impl()->backend().getMainWidget();

            ui->mdiArea->addSubWindow(widget);
            connect(ui->actionstart, &QAction::triggered, rawView, &CanRawView::startSimulation);
            connect(ui->actionstop, &QAction::triggered, rawView, &CanRawView::stopSimulation);
            connect(rawView, &CanRawView::dockUndock, this, [this, widget] { handleDock(widget, ui->mdiArea); });
          }
        , [this, dataModel](CanRawSenderModel& m)
          {
            QWidget* crsWidget = m.canRawSender.getMainWidget();
            auto& rawSender = m.canRawSender;
            ui->mdiArea->addSubWindow(crsWidget);
            connect(&rawSender, &CanRawSender::dockUndock, this, [this, crsWidget] { handleDock(crsWidget, ui->mdiArea); });
            connect(ui->actionstart, &QAction::triggered, &rawSender, &CanRawSender::startSimulation);
            connect(ui->actionstop, &QAction::triggered, &rawSender, &CanRawSender::stopSimulation);
          }
        , [this](CanDeviceModel&) {});
}

void handleWidgetDeletion(QWidget* widget)
{
    assert(nullptr != widget);
    if (widget->parentWidget()) {

        widget->parentWidget()->close();
    } // else path not needed
}

void MainWindow::nodeDeletedCallback(QtNodes::Node& node)
{
    auto dataModel = node.nodeDataModel();

    assert(nullptr != dataModel);

    apply_model_visitor(*dataModel
        , [this, dataModel](CanRawViewModel& m)
          {
            handleWidgetDeletion(m.canRawView.impl()->backend().getMainWidget());
          }
        , [this, dataModel](CanRawSenderModel& m)
          {
            handleWidgetDeletion(m.canRawSender.getMainWidget());
          }
        , [this](CanDeviceModel&) {});

}

void handleWidgetShowing(QWidget* widget)
{
    assert(nullptr != widget);
    if (widget->parentWidget()) {

        widget->parentWidget()->show();
    } else {
        widget->show();
    }
}

void MainWindow::nodeDoubleClickedCallback(QtNodes::Node& node)
{
    auto dataModel = node.nodeDataModel();

    assert(nullptr != dataModel);

    apply_model_visitor(*dataModel
        , [this, dataModel](CanRawViewModel& m)
          {
            handleWidgetShowing(m.canRawView.impl()->backend().getMainWidget());
          }
        , [this, dataModel](CanRawSenderModel& m)
          {
            handleWidgetShowing(m.canRawSender.getMainWidget());
          }
        , [this](CanDeviceModel&) {});
}

void MainWindow::handleDock(QWidget* component, QMdiArea* mdi)
{
    // check if component is already displayed by mdi area
    if (mdi->subWindowList().contains(static_cast<QMdiSubWindow*>(component->parentWidget()))) {
        // undock
        auto parent = component->parentWidget();
        mdi->removeSubWindow(component); // removeSubwWndow only removes widget, not window

        component->show();
        parent->close();
    } else {
        // dock
        mdi->addSubWindow(component)->show();
    }
}

void MainWindow::connectToolbarSignals()
{
    connect(ui->actionstart, &QAction::triggered, ui->actionstop, &QAction::setDisabled);
    connect(ui->actionstart, &QAction::triggered, ui->actionstart, &QAction::setEnabled);
    connect(ui->actionstop, &QAction::triggered, ui->actionstop, &QAction::setEnabled);
    connect(ui->actionstop, &QAction::triggered, ui->actionstart, &QAction::setDisabled);
}

void MainWindow::handleSaveAction()
{
    QString fileName = QFileDialog::getSaveFileName(
        nullptr, "Project Configuration", QDir::homePath(), "CANdevStudio Files (*.cds)");

    if (!fileName.isEmpty()) {
        if (!fileName.endsWith(".cds", Qt::CaseInsensitive))
            fileName += ".cds";

        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(graphScene->saveToMemory()); // FIXME
        }
    } else {
        cds_error("File name empty");
    }
}

void MainWindow::handleLoadAction()
{
    graphScene->clearScene();

    QString fileName
        = QFileDialog::getOpenFileName(nullptr, "Project Configuration", QDir::homePath(), "CANdevStudio (*.cds)");

    if (!QFileInfo::exists(fileName)) {
        cds_error("File does not exist");
        return;
    }

    QFile file(fileName);

    if (!file.open(QIODevice::ReadOnly)) {
        cds_error("Could not open file");
        return;
    }

    QByteArray wholeFile = file.readAll();

    // TODO check if file is correct, nodeeditor library does not provide it and will crash if incorrect file is
    // supplied

    graphScene->loadFromMemory(wholeFile); // FIXME
}

void MainWindow::connectMenuSignals()
{
    QActionGroup* ViewModes = new QActionGroup(this);
    ViewModes->addAction(ui->actionTabView);
    ViewModes->addAction(ui->actionSubWindowView);
    connect(ui->actionAbout, &QAction::triggered, this, [this] { QMessageBox::about(this, "About", "<about body>"); });
    connect(ui->actionExit, &QAction::triggered, this, &MainWindow::close);
    connect(ui->actionLoad, &QAction::triggered, this, &MainWindow::handleLoadAction);
    connect(ui->actionSave, &QAction::triggered, this, &MainWindow::handleSaveAction);
    connect(ui->actionTile, &QAction::triggered, ui->mdiArea, &QMdiArea::tileSubWindows);
    connect(ui->actionCascade, &QAction::triggered, ui->mdiArea, &QMdiArea::cascadeSubWindows);
    connect(ui->actionTabView, &QAction::triggered, this, [this] { ui->mdiArea->setViewMode(QMdiArea::TabbedView); });
    connect(ui->actionTabView, &QAction::toggled, ui->actionTile, &QAction::setDisabled);
    connect(ui->actionTabView, &QAction::toggled, ui->actionCascade, &QAction::setDisabled);
    connect(ui->actionSubWindowView, &QAction::triggered, this,
        [this] { ui->mdiArea->setViewMode(QMdiArea::SubWindowView); });
}

void MainWindow::setupMdiArea()
{
    graphView = new QtNodes::FlowView(graphScene.get());
    graphView->setWindowTitle("Project Configuration");
    ui->mdiArea->addSubWindow(graphView);
    ui->mdiArea->setAttribute(Qt::WA_DeleteOnClose, false);
    ui->mdiArea->tileSubWindows();
}
