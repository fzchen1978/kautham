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
 
 
 
#if !defined(_GUI_H)
#define _GUI_H

#include <QtGui>
#include <Inventor/Qt/viewers/SoQtExaminerViewer.h>
#include <vector>
#include <string>
#include <Inventor/nodes/SoSeparator.h>
#include <libproblem/robot.h>
#include "ui_RobotSim.h"
#include "viewertype.h"
#include <libsplanner/planner.h>
#include <libproblem/problem.h>
#include <libsampling/sampling.h>
#include "streamlog.h"
#include <libkin/inversekinematic.h>
#include "controlwidget.h"
#include "plannerwidget.h"


using namespace std;

namespace Kautham {
/** \addtogroup libGUI
 *  @{
 */

	enum WHERETYPE{
		TOOLBAR,
		FILEMENU,
		ACTIONMENU,
		FILETOOL,
		ACTIONTOOL,
	};

	struct Viewer{
		VIEWERTYPE type;
		SoQtExaminerViewer *window;
		SoSeparator *root;
		string title;
		QWidget *tab;
    Viewer(){
      window = NULL;
      root = NULL;
      title = "";
      tab = NULL;
    }
    //~Viewer(){
    //  root = NULL;
    //  title = "";
    //  tab = NULL;
    //  if(window != NULL) delete window;
    //}
	};


    class PlannerWidget; //needed here because PlannerWidget has #include "gui.h"

	class GUI:public QMainWindow, private Ui::kauthamMain {
	  Q_OBJECT
	//signals:
	//	void tableChanged(string newContent);
	public slots:
    void                setText(string s);
    void                about();
    void                help();
    void                showPlannerToolBar();
    void                changeActiveBackground();
    void                changeDockAreaForOutput(Qt::DockWidgetArea area);
    //! Enables/Disables the use of bounding boxes as collision models
    void                toogleBBOXflag();
    //! Sets the directories where robot and obstacles models will be looked for
    void                setModelsDefaultPath();

    public:
    GUI(QWidget *p=0);
    void                clearText();
    bool                addViewerTab(string title, VIEWERTYPE typ, SoSeparator *root);
    void                removePropTab(string title);
    void                removeViewerTab(string title);
    SoQtExaminerViewer* getViewerTab(string title);
    SoSeparator*        getRootTab(string title);
    //bool              setTable(string s);
    bool                setAction(WHERETYPE typ, string name, string shortcut, string iconame,
                        QObject* receiver, const char *member);
    bool                setToogleAction(WHERETYPE typ, string name, string shortcut, string iconame,
                        QObject* receiver, const char *member);
    bool                addSeparator(WHERETYPE typ);
    bool                restart();
    bool                addToProblemTree(string problemPath);
    bool                addRobControlWidget(Problem* prob, vector<Kautham::DOFWidget *> robDOFWidgets);

    bool                addExternalWidget1( Robot* rob, Problem* prob, int offset = 0, GUI* gui = NULL);
    bool                addExternalWidget2( Robot* rob, Problem* prob, int offset = 0, GUI* gui = NULL);
    bool                addExternalWidget3( Robot* rob, Problem* prob, int offset = 0, GUI* gui = NULL);


    bool                addConstrainedControlWidget( Robot* rob, Problem* prob, int offset = 0);
    Kautham::DOFWidget *addDOFWidget( Robot* rob );
    bool                setSampleWidget(SampleSet* samples, Sampler* sampler, Problem* prob);
    bool                createPlannerToolBar(string loc, string plan, QObject* receiver, const char* member);
    bool                addPlanner(Planner *plan, SampleSet* samp, GUI* gui = NULL);

    bool                addInverseKinematic(InverseKinematic* ikine);
    bool                addWidgetToTab(QWidget* widget, QString name);

    const mt::Transform getActiveCameraTransfom();
    bool                setActiveCameraPosition(float x, float y, float z );
    bool                setActiveCameraRotation(float qx, float qy, float qz, float qw);
    bool                setActiveCameraPointAt(float x, float y, float z );
    bool                setActiveCameraTransform(mt::Transform tra);
    std::string         getActiveViewTitle();
	
    ControlWidget*		getControlWidget();
    PlannerWidget*		getPlannerWidget();

    //! Hides the Introduction Tab and shows the Properties and DOF tabs
    void showProblemAppearance();

    //! Shows the Introduction Tab and hides the Properties and DOF tabs
    void showInitialAppearance();

    int			 		indexControlsTab;
    int                 indexPlannerTab;
  private:
    vector<Viewer>      viewers;
    StreamLog*          qout;
    bool                boolPlanVis;
	};

    /** @}   end of Doxygen module "libGUI" */
}

#endif  //_GUI_H


