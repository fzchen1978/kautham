/*************************************************************************\
   Copyright 2014 Institute of Industrial and Control Engineering (IOC)
                 Universitat Politecnica de Catalunya
                 BarcelonaTech
    All Rights Reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the
    Free Software Foundation, Inc.,
    59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 \*************************************************************************/

/* Author: Alexander Perez, Jan Rosell, Nestor Garcia Hidalgo */
 
 
#if !defined(_CONTROLWIDGET_H)
#define _CONTROLWIDGET_H

#include <QtGui>
#include <Inventor/Qt/viewers/SoQtExaminerViewer.h>
#include <vector>
#include <string>
#include <problem/robot.h>
#include <problem/problem.h>
#include <kthutil/kauthamdefs.h>
#include "dofwidget.h"


using namespace std;


namespace Kautham {
/** \addtogroup libGUI
 *  @{
 */

	class ControlWidget:public QWidget{
		Q_OBJECT

	private slots:
		void              sliderChanged(int val);
		void              updateControls();
	public:
        ControlWidget(Problem* prob,vector<DOFWidget*> DOFWidgets, bool robot);
		~ControlWidget();
		inline vector<KthReal>   *getValues(){return &values;}
        void setValues(vector <KthReal> coords);
	private:
        vector<DOFWidget*> _DOFWidgets;
		vector<QSlider*>  sliders;
		vector<QLabel*>   labels;
		QGridLayout       *gridLayout;
		QVBoxLayout       *vboxLayout;
		QVBoxLayout       *vboxLayout1;
        QPushButton		  *btnUpdate;
		vector<KthReal>   values;
        Problem*          _ptProblem;
        bool              robWidget;
	};


    /** @}   end of Doxygen module "libGUI" */
}

#endif  //_CONTROLWIDGET_H


