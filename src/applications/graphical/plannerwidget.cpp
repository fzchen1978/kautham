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
 
#include "plannerwidget.h"
#include "gui.h"
#include "libsampling/se3conf.h"
#include "libsampling/robconf.h"

#if defined(KAUTHAM_USE_IOC)
#include "libsplanner/libioc/iocplanner.h"
#endif

namespace Kautham{


  PlannerWidget::PlannerWidget(Planner* plan, SampleSet* samp, bool camera, GUI* gui):KauthamWidget(plan){
    _samples = samp;
    _planner = plan;
    _gui = gui;
    _stepSim = 0;
    _ismoving = false;

    //XXXXXXXXXXXXXXXXXXXXXXXXX
        QGroupBox *groupBox = new QGroupBox(this);
        groupBox->setObjectName(QString::fromUtf8("groupBox"));
        QGridLayout *gridLayoutGB = new QGridLayout(groupBox);
        gridLayoutGB->setObjectName(QString::fromUtf8("gridLayout"));
        QVBoxLayout *verticalLayoutGB = new QVBoxLayout();
        verticalLayoutGB->setObjectName(QString::fromUtf8("verticalLayout"));
        QHBoxLayout *horizontalLayoutGB = new QHBoxLayout();
        horizontalLayoutGB->setObjectName(QString::fromUtf8("horizontalLayout"));
        QLabel *labelGB = new QLabel(groupBox);
        labelGB->setObjectName(QString::fromUtf8("label"));

        horizontalLayoutGB->addWidget(labelGB);

        _spFrom = new QSpinBox(groupBox);
        _spFrom->setObjectName(QString::fromUtf8("_spFrom"));

        horizontalLayoutGB->addWidget(_spFrom);

        QLabel *label_2GB = new QLabel(groupBox);
        label_2GB->setObjectName(QString::fromUtf8("label_2"));

        horizontalLayoutGB->addWidget(label_2GB);

        _spTo = new QSpinBox(groupBox);
        _spTo->setObjectName(QString::fromUtf8("_spTo"));

        horizontalLayoutGB->addWidget(_spTo);


        verticalLayoutGB->addLayout(horizontalLayoutGB);

        QHBoxLayout *horizontalLayout_2GB = new QHBoxLayout();
        horizontalLayout_2GB->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        _cmbTry = new QPushButton(groupBox);
        _cmbTry->setObjectName(QString::fromUtf8("_cmbTry"));

        horizontalLayout_2GB->addWidget(_cmbTry);

        _lblRes = new QLabel(groupBox);
        _lblRes->setObjectName(QString::fromUtf8("label_3"));
        QSizePolicy sizePolicy1(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(_lblRes->sizePolicy().hasHeightForWidth());
        _lblRes->setSizePolicy(sizePolicy1);
        _lblRes->setMinimumSize(QSize(20, 20));
        _lblRes->setPixmap(QPixmap(QString::fromUtf8(":/icons/tryconnect.xpm")));

        horizontalLayout_2GB->addWidget(_lblRes);


        verticalLayoutGB->addLayout(horizontalLayout_2GB);


        gridLayoutGB->addLayout(verticalLayoutGB, 0, 0, 1, 1);


        vboxLayout->addWidget(groupBox);
    //XXXXXXXXXXXXXXXXXXXXXXXXX
    
    tmpLabel = new QLabel(this);
    tmpLabel->setText("Init configuration is the sample:");
    spnInit = new QSpinBox(this);

    hboxLayout = new QHBoxLayout();
    hboxLayout->addWidget(tmpLabel);
    hboxLayout->addWidget(spnInit);
    vboxLayout->addLayout(hboxLayout);

    tmpLabel = new QLabel(this);
    tmpLabel->setText("Goal configuration is the sample:");
    spnGoal = new QSpinBox(this);

    hboxLayout = new QHBoxLayout();
    hboxLayout->addWidget(tmpLabel);
    hboxLayout->addWidget(spnGoal);
    vboxLayout->addLayout(hboxLayout);

    chkCamera = new QCheckBox("Move the camera.");
    chkCamera->setChecked(false);

    if(camera = true){
      vboxLayout->addWidget(chkCamera);
    }

    hboxLayout = new QHBoxLayout();
    hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));

    btnGetPath = new QPushButton(this);
    btnGetPath->setObjectName(QString::fromUtf8("getPathButton"));

    hboxLayout->addWidget(btnGetPath);

    btnSaveData = new QPushButton(this);
    btnSaveData->setObjectName(QString::fromUtf8("saveButton"));

    hboxLayout->addWidget(btnSaveData);
	
    vboxLayout->addLayout(hboxLayout);

    hboxLayout2 = new QHBoxLayout();
    hboxLayout2->setObjectName(QString::fromUtf8("hboxLayout2"));


    btnLoadData = new QPushButton(this);
    btnLoadData->setObjectName(QString::fromUtf8("loadButton"));

    hboxLayout2->addWidget(btnLoadData);

    btnMove = new QPushButton(this);
    btnMove->setObjectName(QString::fromUtf8("moveButton"));
    btnMove->setEnabled(false);

    hboxLayout2->addWidget(btnMove);

    vboxLayout->addLayout(hboxLayout2);

    btnGetPath->setText(QApplication::translate("Form", "Get Path", 0, QApplication::UnicodeUTF8));
    btnSaveData->setText(QApplication::translate("Form", "Save Data", 0, QApplication::UnicodeUTF8));
    btnLoadData->setText(QApplication::translate("Form", "Load Data", 0, QApplication::UnicodeUTF8));
    btnMove->setText(QApplication::translate("Form", "Start Move ", 0, QApplication::UnicodeUTF8));
    
    groupBox->setTitle(QApplication::translate("Form", "Local Planner", 0, QApplication::UnicodeUTF8));
    labelGB->setText(QApplication::translate("Form", "From:", 0, QApplication::UnicodeUTF8));
    label_2GB->setText(QApplication::translate("Form", "To:", 0, QApplication::UnicodeUTF8));
    _cmbTry->setText(QApplication::translate("Form", "Try Connect", 0, QApplication::UnicodeUTF8));
    _lblRes->setText(QString());
    
    _plannerTimer = new QTimer( this ); 

    if(_planner != NULL ){
      connect(btnGetPath, SIGNAL( clicked() ), this, SLOT( getPathCall() ) );
      connect(btnSaveData, SIGNAL( clicked() ), this, SLOT( saveDataCall() ) );
      connect(btnLoadData, SIGNAL( clicked() ), this, SLOT( loadDataCall() ) );
      connect(btnMove, SIGNAL( clicked() ), this, SLOT( simulatePath() ) );
      connect(_plannerTimer, SIGNAL(timeout()), this, SLOT(moveAlongPath()) );
      connect(spnInit, SIGNAL( valueChanged( int )), this, SLOT( showSample( int )));
      connect(spnGoal, SIGNAL( valueChanged( int )), this, SLOT( showSample( int )));
      connect(_spFrom, SIGNAL( valueChanged( int )), this, SLOT( showSample( int )));
      connect(_spTo, SIGNAL( valueChanged( int )), this, SLOT( showSample( int )));
      connect(_cmbTry, SIGNAL( clicked() ), this, SLOT( tryConnect( )));
      connect(chkCamera, SIGNAL( clicked() ), this, SLOT( chkCameraClick( )));

      //init sample is defaulted to zero, i.e. spnInit->setValue( 0 );
      //goal sample is dafaulted to 1 since in problem.cpp it is loaded in this order when there is a query to solve
//      if(_planner->goalSamp()!=NULL && _planner->goalSamp()->isFree())
//      {
//          spnGoal->setValue( 1 );
//      }

       spnInit->setValue( 0 );
       spnGoal->setValue( 1 );
       _spFrom->setValue( 0 );
       _spTo->setValue( 1 );

    }
	}

  void PlannerWidget::tryConnect(){
#if defined(KAUTHAM_USE_IOC)
    if(_planner != NULL ){
        if(_planner->getFamily()=="ioc")
        {
            ((IOC::iocPlanner*)_planner)->getLocalPlanner()->setInitSamp(_samples->getSampleAt(_spFrom->text().toInt()));
            ((IOC::iocPlanner*)_planner)->getLocalPlanner()->setGoalSamp(_samples->getSampleAt(_spTo->text().toInt()));
	  
            KthReal d = ((IOC::iocPlanner*)_planner)->getLocalPlanner()->distance(_samples->getSampleAt(_spFrom->text().toInt()),
												   _samples->getSampleAt(_spTo->text().toInt()));
            char str[30];
            sprintf(str,"Distance:  %f",d);
            writeGUI(str);
            if( ((IOC::iocPlanner*)_planner)->getLocalPlanner()->canConect() ){
                _lblRes->setPixmap(QPixmap(QString::fromUtf8(":/icons/connect.xpm")));
                writeGUI("The samples can be connected.");
            }else{
                _lblRes->setPixmap(QPixmap(QString::fromUtf8(":/icons/noconnect.xpm")));
                writeGUI("The samples can NOT be connected.");
            }

        }
        else  writeGUI("Sorry: Nothing implemented yet for non-ioc planners");
    }else
      writeGUI("The planner is not configured properly!!. Something is wrong with your application.");
#endif
  }

  void PlannerWidget::getPathCall(){
    _gui->setCursor(QCursor(Qt::WaitCursor));
    
    //assert(_CrtCheckMemory());

    if(_planner != NULL ){
      _planner->wkSpace()->moveObstaclesTo(_planner->wkSpace()->getInitObsSample());
      _planner->setInitSamp(_samples->getSampleAt(spnInit->text().toInt()));
      _planner->setGoalSamp(_samples->getSampleAt(spnGoal->text().toInt()));

    if(_planner->solveAndInherit())
      btnMove->setEnabled(true);
    else 
      btnMove->setEnabled(false);
    }
    //assert(_CrtCheckMemory());
    _gui->setCursor(QCursor(Qt::ArrowCursor));
  }

  void PlannerWidget::saveDataCall(){
#if defined(KAUTHAM_USE_IOC)
    if(_planner != NULL ){
        if(_planner->getFamily()=="ioc")
        {
            QString path,dir;
            QDir workDir;
            _gui->setCursor(QCursor(Qt::WaitCursor));
            path = QFileDialog::getSaveFileName( _gui,
		      "Save planner data as...", workDir.absolutePath(),
		      "Kautham Planner Solution (*.kps)");
            if(!path.isEmpty()){
                sendText(QString("Kautham is saving a planner data in a file: " + path).toUtf8().constData() );
                dir = path;
                dir.truncate(dir.lastIndexOf("/"));
                ((IOC::iocPlanner*)_planner)->saveData(path.toUtf8().constData());
            }
            _gui->setCursor(QCursor(Qt::ArrowCursor));
            setTable(_planner->getParametersAsString());
         }
        else  writeGUI("Sorry: Nothing implemented yet for non-ioc planners");
    }
#endif
}

  void PlannerWidget::loadDataCall(){
#if defined(KAUTHAM_USE_IOC)
    if(_planner != NULL ){
        if(_planner->getFamily()=="ioc")
        {
            QString path,dir;
            QDir workDir;
            _gui->setCursor(QCursor(Qt::WaitCursor));
            path = QFileDialog::getOpenFileName( _gui,
		      "Load a file...", workDir.absolutePath(),
		      "Kautham Planner Solution (*.kps)");
            if(!path.isEmpty()){
                sendText(QString("The solution file in " + path + " is being loaded.").toUtf8().constData() );
                dir = path;
                dir.truncate(dir.lastIndexOf("/"));
                ((IOC::iocPlanner*)_planner)->loadData(path.toUtf8().constData());
                if( _planner->isSolved() ) btnMove->setEnabled(true);
            }
            _gui->setCursor(QCursor(Qt::ArrowCursor));
            setTable(_planner->getParametersAsString());
            }
        }
        else  writeGUI("Sorry: Nothing implemented yet for non-ioc planners");
    #endif
  }
  

  void PlannerWidget::simulatePath(){
    if(btnMove->text() == QApplication::translate("Form", "Start Move ", 0, QApplication::UnicodeUTF8)){
      _plannerTimer->start(200);
      _ismoving = true;
      //_stepSim = 0;
      btnMove->setText(QApplication::translate("Form", "Stop Move ", 0, QApplication::UnicodeUTF8));
    }else{
      _plannerTimer->stop();
      btnMove->setText(QApplication::translate("Form", "Start Move ", 0, QApplication::UnicodeUTF8));
      _ismoving = false;
    }

  }



  void PlannerWidget::moveAlongPath(){
    _planner->moveAlongPath(_stepSim);
    // It moves the camera if the associated planner provides the
    // transformation information of the camera
    if( chkCamera->isChecked() && _gui != NULL
       && _planner->getCameraMovement(_stepSim) != NULL )
        _gui->setActiveCameraTransform(*_planner->getCameraMovement( _stepSim ));
    
    _stepSim += _planner->getSpeedFactor();

  }

  void PlannerWidget::showSample(int index){
	  int max;

    _lblRes->setPixmap(QPixmap(QString::fromUtf8(":/icons/tryconnect.xpm")));

    max = _samples->getSize();
//    if(_samples->getSize() < _planner->getMaxNumSamples())
//      max = _samples->getSize();
//	  else{
//		  max = _planner->getMaxNumSamples();
//		  if(index>max) cout<<"Using a maximum of "<<max<<" samples"<<endl;
//	  }
  	
    if( _samples->getSize() > 1 ){
      spnInit->setMaximum(max- 1 );
      spnGoal->setMaximum(max- 1 );
      _spFrom->setMaximum(max- 1 );
      _spTo->setMaximum(max- 1 );
    }

	  if( index >= 0 && index < max ){
		  Sample *smp =  _samples->getSampleAt(index);
      _planner->wkSpace()->moveRobotsTo(smp );
  	
	    vector<KthReal> c = smp->getCoords();
	    cout << "sample: ";

		  for(int i=0; i<c.size(); i++) 
        cout << c[i] << ", ";

	    cout << endl;

        if(smp->getMappedConf().size()!=0)
        {
            SE3Conf &s = smp->getMappedConf()[0].getSE3();
            cout << s.getPos().at(0) << " ";
            cout << s.getPos().at(1) << " ";
            cout << s.getPos().at(2) << endl;

        }

      // print neighbours
      //cout << "Neights: " ;
      //for(int i =0; i< smp->getNeighs()->size(); i++)
      //  cout << smp->getNeighs()->at(i) << ", ";
      //cout << endl;

    }else{
        spnInit->setValue( 0 );
        spnGoal->setValue( 0 );
        _spFrom->setValue( 0 );
        _spTo->setValue( 0 );
	  }
  }

  void PlannerWidget::chkCameraClick(){
    if( chkCamera->isChecked() )
      _planner->wkSpace()->setPathVisibility( false );
    else
      _planner->wkSpace()->setPathVisibility( true );
  }

}