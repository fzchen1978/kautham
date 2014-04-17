/***************************************************************************
*               Generated by StarUML(tm) C++ Add-In                        *
***************************************************************************/
/***************************************************************************
*                                                                          *
*           Institute of Industrial and Control Engineering                *
*                 Technical University of Catalunya                        *
*                        Barcelona, Spain                                  *
*                                                                          *
*                Project Name:       Kautham Planner                       *
*                                                                          *
*     Copyright (C) 2007 - 2009 by Alexander Pérez and Jan Rosell          *
*            alexander.perez@upc.edu and jan.rosell@upc.edu                *
*                                                                          *
*             This is a motion planning tool to be used into               *
*             academic environment and it's provided without               *
*                     any warranty by the authors.                         *
*                                                                          *
*          Alexander Pérez is also with the Escuela Colombiana             *
*          de Ingeniería "Julio Garavito" placed in Bogotá D.C.            *
*             Colombia.  alexander.perez@escuelaing.edu.co                 *
*                                                                          *
***************************************************************************/
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
 
  
#include <Inventor/Qt/SoQt.h>
#include <QFile>
#include <QString>
#include <QMessageBox>
#include <sstream>
#include <libproblem/ivworkspace.h>
#include <libkthutil/kauthamdefs.h>
#include "application.h"


Application::Application() {
    settings = new QSettings("IOC","Kautham");
    Q_INIT_RESOURCE(kauthamRes);
    initApp();
}

void Application::initApp(){
    mainWindow = new GUI();
    settings->beginGroup("mainWindow");

    mainWindow->resize(settings->value("size",QSize(1024,768)).toSize());
    mainWindow->move(settings->value("pos",QPoint(0,0)).toPoint()-mainWindow->pos());
    if (settings->value("fullScreen",false).toBool()) {
        mainWindow->showFullScreen();
    }
    settings->endGroup();

    SoQt::show(mainWindow);
    setActions();
    mainWindow->setText("Open a problem file to start...");
    appState = INITIAL ;
    _problem = NULL;
}

Application::~Application() {
    settings->beginGroup("mainWindow");
    settings->setValue("size",mainWindow->size());
    settings->setValue("pos",mainWindow->pos());
    settings->setValue("fullScreen",mainWindow->isFullScreen());
    settings->endGroup();

    if (appState == PROBLEMLOADED) {
        saveTabColors();
        closeProblem();
    }

    delete settings;
}

void Application::setActions(){
    mainWindow->setAction(FILETOOL,"&Open","CTRL+O",":/icons/fileopen.xpm",this,SLOT(openFile()));
    mainWindow->setAction(FILETOOL,"&Save","CTRL+S",":/icons/filesave.xpm",this,SLOT(saveFile()));
    mainWindow->setAction(FILETOOL,"Save &as","CTRL+A",":/icons/saveas.xpm",this,SLOT(saveAsFile()));
    mainWindow->addSeparator(TOOLBAR);

    // Creating the planner toolbar in the main Window. This list may change.
    //string loc = Problem::localPlannersNames();
    //string glob = Problem::plannersNames();


    //mainWindow->createPlannerToolBar(loc, glob,this,SLOT(changePlanner(string,string)));
    //mainWindow->setToogleAction(ACTIONTOOL,"&Find path","CTRL+F",":/icons/prm.xpm",mainWindow,SLOT(showPlannerToolBar()));
    mainWindow->addSeparator(TOOLBAR);
    mainWindow->addSeparator(ACTIONMENU);

    mainWindow->addSeparator(TOOLBAR);
    mainWindow->setAction(ACTIONTOOL,"Chan&ge Colour","CTRL+G",
                          ":/icons/determ.xpm", mainWindow, SLOT(changeActiveBackground()));
    if (settings->value("use_BBOX","false").toBool()) {
        mainWindow->setAction(ACTIONTOOL,"Disable BBOX","",":/icons/BBOXdisabled.xpm", mainWindow, SLOT(toogleBBOXflag()));
    } else {
        mainWindow->setAction(ACTIONTOOL,"Enable BBOX","",":/icons/BBOXenabled.xpm", mainWindow, SLOT(toogleBBOXflag()));
    }
    mainWindow->setAction(FILETOOL,"&Close","CTRL+Q",":/icons/close.xpm",this,SLOT(closeProblem()));
}

void Application::openFile(){
    QString last_path, path, dir;
    QDir workDir;
    last_path = settings->value("last_path",workDir.absolutePath()).toString();
    mainWindow->setCursor(QCursor(Qt::WaitCursor));
    path = QFileDialog::getOpenFileName(
                mainWindow,
                "Choose a file to open",
                last_path,
                "All configuration files (*.xml)");
    if(!path.isEmpty()){
        if (appState == PROBLEMLOADED) {
            closeProblem();
            appState = INITIAL;
        }
        mainWindow->setText("Kautham is opening a problem file...");
        dir = path;
        dir.truncate(dir.lastIndexOf("/"));
        if (problemSetup(path.toUtf8().constData())) {
            mainWindow->hideIntroTab();
            settings->setValue("last_path",dir);

            stringstream tmp;
            tmp << "Kautham ";
            tmp << MAJOR_VERSION;
            tmp << ".";
            tmp << MINOR_VERSION;
            tmp << " - ";
            tmp << path.toUtf8().constData();
            mainWindow->setWindowTitle( tmp.str().c_str() );
            mainWindow->setText(QString("File: ").append(path).toUtf8().constData() );
            mainWindow->setText("opened successfully.");
        } else {
            mainWindow->setText("Kautham couldn't open the problem file...");
        }
    }
    mainWindow->setCursor(QCursor(Qt::ArrowCursor));
}

void Application::saveFile(){
    mainWindow->setCursor(QCursor(Qt::WaitCursor));
    if( appState == PROBLEMLOADED ){
        if( _problem->saveToFile() )
            mainWindow->setText( "File saved successfully" );
        else
            mainWindow->setText( "Sorry but the file is not saved" );
    }
    mainWindow->setCursor(QCursor(Qt::ArrowCursor));
}

void Application::saveAsFile(){
    QString path,dir;
    QDir workDir;
    mainWindow->setCursor(QCursor(Qt::WaitCursor));
    switch(appState){
    case PROBLEMLOADED:
        path = QFileDialog::getSaveFileName(
                    mainWindow,
                    "Save as ...",
                    workDir.absolutePath(),
                    "All configuration files (*.xml)");
        if(!path.isEmpty()){
            mainWindow->setText( "Kautham is saving a problem file: " );
            mainWindow->setText( path.toUtf8().constData() );
            dir = path;
            dir.truncate(dir.lastIndexOf("/"));
            if( _problem->saveToFile( path.toUtf8().constData() ) )
                mainWindow->setText( "File saved successfully" );
            else
                mainWindow->setText( "Sorry but the file is not saved" );
        }
    }
    mainWindow->setCursor(QCursor(Qt::ArrowCursor));
}


void Application::closeProblem(){
    mainWindow->setCursor(QCursor(Qt::WaitCursor));
    switch(appState){
    case INITIAL:
        mainWindow->setText("First open a problem");
        break;
    case PROBLEMLOADED:
        if(mainWindow->getPlannerWidget()->ismoving())
            mainWindow->getPlannerWidget()->simulatePath();//stops simulation
        saveTabColors();
        mainWindow->restart();
        delete _problem;
        appState = INITIAL;

        break;
    }
    mainWindow->setCursor(QCursor(Qt::ArrowCursor));
}


void Application::saveTabColors() {
    vector <string> viewers;
    viewers.push_back("WSpace");
    viewers.push_back("CollisionWSpace");
    viewers.push_back("CSpace");
    SbColor color;
    settings->beginGroup("mainWindow");
    for (int i = 0; i < viewers.size(); i++) {
        settings->beginGroup(viewers.at(i).c_str());
        color = mainWindow->getViewerTab(viewers.at(i))->getBackgroundColor();
        settings->setValue("color",QColor((int)(255.0*color.getValue()[0]),
                                          (int)(255.0*color.getValue()[1]),
                                          (int)(255.0*color.getValue()[2])));
        settings->endGroup();
    }
    settings->endGroup();
}


bool Application::problemSetup(string path){
    mainWindow->setCursor(QCursor(Qt::WaitCursor));
    _problem = new Problem();
    if (!_problem->setupFromFile(path)) {
        appState = INITIAL;
        delete _problem;
        return false;
    }

    mainWindow->addToProblemTree( path );

    mainWindow->addViewerTab("WSpace", SPACE, ((IVWorkSpace*)_problem->wSpace())->getIvScene());
    QColor color = settings->value("mainWindow/WSpace/color",QColor("black")).value<QColor>();
    mainWindow->getViewerTab("WSpace")->setBackgroundColor(SbColor(color.redF(),color.greenF(),color.blueF()));

    mainWindow->addViewerTab("CollisionWSpace", SPACE, ((IVWorkSpace*)_problem->wSpace())->getCollisionIvScene());

    color = settings->value("mainWindow/CollisionWSpace/color",QColor("black")).value<QColor>();
    mainWindow->getViewerTab("CollisionWSpace")->setBackgroundColor(SbColor(color.redF(),color.greenF(),color.blueF()));

    //  Used to show the IV models reconstructed from the PQP triangular meshes.
    //mainWindow->addViewerTab("PQP", SPACE, ((IVWorkSpace*)_problem->wSpace())->getIvFromPQPScene());

    mainWindow->addControlWidget(_problem);

    for(unsigned i = 0; i < _problem->wSpace()->robotsCount(); i++){

        if(_problem->wSpace()->getRobot(i)->getCkine() != NULL)
            mainWindow->addConstrainedControlWidget(_problem->wSpace()->getRobot(i), _problem, 0);
        // Use the following widget if the user can modified all the dof instead of the controls.
        //mainWindow->addDOFWidget(_problem->wSpace()->getRobot(i) );


        //Add widget for external applications
        //widget 1 used for virtual bronchoscopy apllication
        mainWindow->addExternalWidget1(_problem->wSpace()->getRobot(i), _problem, 0, mainWindow);
        //widget 2 not used
        mainWindow->addExternalWidget2(_problem->wSpace()->getRobot(i), _problem, 0, mainWindow);
        //widget 3 not used
        mainWindow->addExternalWidget3(_problem->wSpace()->getRobot(i), _problem, 0, mainWindow);


        if(_problem->wSpace()->getRobot(i)->getIkine() != NULL)
            mainWindow->addInverseKinematic(_problem->wSpace()->getRobot(i)->getIkine());
    }

    mainWindow->setSampleWidget(_problem->getSampleSet(), _problem->getSampler(), _problem);

    if( _problem->getPlanner() != NULL ){
        mainWindow->addPlanner(_problem->getPlanner(), _problem->getSampleSet(), mainWindow);
        color = settings->value("mainWindow/CSpace/color",QColor("black")).value<QColor>();
        mainWindow->getViewerTab("CSpace")->setBackgroundColor(SbColor(color.redF(),color.greenF(),color.blueF()));
    }

    appState = PROBLEMLOADED;

    mainWindow->setCursor(QCursor(Qt::ArrowCursor));
    return true;
}


