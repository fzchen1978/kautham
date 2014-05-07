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

 
#if !defined(_PROBLEM_H)
#define _PROBLEM_H

#include <QString>

#include <libsplanner/planner.h>
#include <sampling/sampling.h>
#include <ompl/geometric/SimpleSetup.h>
#include "robot.h"
#include "ivworkspace.h"
#include "workspace.h"
#include <sampling/state.h>
#include <pugixml.hpp>

#include <string>
#include <fstream>
#include <iostream>
#include <boost/algorithm/string.hpp>

//solving convertions problems
#include <locale.h>


#if defined(KAUTHAM_USE_IOC)
#include <libioc/myplanner.h>
#include <libioc/prmplanner.h>
#include <libioc/prmhandplannerICRA.h>
#include <libioc/prmAUROhandarmplanner.h>
#include <libioc/prmPCAhandarmplanner.h>
#include <libioc/prmrobothandconstplannerICRA.h>
#include <libioc/prmhandplannerIROS.h>
#include <libioc/myprmplanner.h>
#include <libioc/mygridplanner.h>
#include <libioc/NF1planner.h>
#include <libioc/HFplanner.h>
#endif

#if defined(KAUTHAM_USE_OMPL)
#include <libompl/omplPRMplanner.h>
#include <libompl/omplRRTplanner.h>
#include <libompl/omplRRTStarplanner.h>
#include <libompl/omplTRRTplanner.h>
#include <libompl/omplpRRTplanner.h>
#include <libompl/omplLazyRRTplanner.h>
#include <libompl/omplcRRTplanner.h>
#include <libompl/omplcRRTcarplanner.h>
#include <libompl/omplcRRTf16planner.h>
#include <libompl/omplRRTConnectplanner.h>
#include <libompl/omplESTplanner.h>
#include <libompl/omplSBLplanner.h>
#include <libompl/omplKPIECEplanner.h>
#include <libompl/omplKPIECEplanner.h>
#endif

#if defined(KAUTHAM_USE_ODE)
#include <libkauthamopende/KauthamOpenDERRTPlanner.h>
#include<libkauthamopende/KauthamOpenDEKPIECEPlanner.h>
#endif

#if defined(KAUTHAM_USE_GUIBRO)
#include <libguibro/consbronchoscopykin.h>
#include <libguibro/guibrogridplanner.h>
#endif // KAUTHAM_USE_GUIBRO

#if !defined(M_PI)
#define M_PI 3.1415926535897932384626433832795
#endif


using namespace std;
using namespace pugi;


namespace Kautham {

/** \addtogroup libProblem
 *  @{
 */

	class Problem {
  public:
    Problem();
    ~Problem();

    //!	This member function create the work phisycal space.
    /*!	With this fuction you can to create the work phisycal space that represent
    *		the problem. One or more Robot and one or more obstacle  compose it.
    *		These Robots could be both the freefly type or cinematic chain type, it
    *		mean that if a problem contain many robots all of them should be the
    *		same class.
    *		\sa WSpace Robot ChainRobot Obstacle*/
    bool 	               createWSpace(pugi::xml_document *doc);

    //! This method is deprecated. Please take care with the problem XML file.
    //bool			              createWSpace(ProbStruc *reader);

    bool                    createPlanner(string name, ompl::geometric::SimpleSetup *ssptr = NULL);
    bool                    createPlannerFromFile(pugi::xml_document *doc, ompl::geometric::SimpleSetup *ssptr = NULL);
    bool                    createCSpace();
    bool                    createCSpaceFromFile(pugi::xml_document *doc);
    bool                    tryToSolve();
    bool                    setCurrentRobControls(vector<KthReal> &val);
    bool                    setCurrentObsControls(vector<KthReal> &val);
    //! Returns WSpace
    WorkSpace*		        wSpace();
    //! Returns CSpace
    SampleSet*              cSpace();
    //! Sets WSpace
    inline void             setWSpace(WorkSpace* WSpace) {
        _wspace = WSpace;
    }

    //! Sets CSpace
    inline void             setCSpace(SampleSet* CSpace) {
        _cspace = CSpace;
    }

    void                    setHomeConf(Robot* rob, HASH_S_K* param);
    void                    setPlanner(Planner* plan){if(_planner==NULL)_planner = plan;}
    inline Planner*         getPlanner(){return _planner;}
    inline SampleSet*       getSampleSet(){return _cspace;}
    inline Sampler*         getSampler(){return _sampler;}
    inline void             setSampler(Sampler* smp){_sampler = smp;}
    inline int              getDimension(){return _wspace->getNumRobControls();}
    inline vector<KthReal>& getCurrentRobControls(){return _currentRobControls;}
    inline vector<KthReal>& getCurrentObsControls(){return _currentObsControls;}
    inline string           getFilePath(){return _filePath;}
    bool                    inheritSolution();
    bool                    setupFromFile(string xml_doc);
    bool                    setupFromFile(ifstream* xml_inputfile, string modelsfolder);
    bool                    setupFromFile(pugi::xml_document *doc);


    //! This method saves the information of the problem's planner . 
    //! This method checks if the file_path file exists or not. In 
    //! case the file doesn't exist, the method copies the current 
    //! problem file and adds the planner's attributes.
    bool                    saveToFile(string file_path = "");

  

  private:
    const static KthReal    _toRad;
    WorkSpace*              _wspace;
    SampleSet*              _cspace;
    vector<State>           _sspace;
    CONFIGTYPE              _problemType;
    Sampler*                _sampler;
    Planner*                _planner;
    vector<KthReal>         _currentRobControls;
    vector<KthReal>         _currentObsControls;
    string                  _filePath;


    /*!
     * \brief loads a robot node of the problem file,
     creates the robot and adds it to the workspace
     * \param robot_node robot node with the information of the robot
     to add to the workspace
     */
    bool addRobot2WSpace(xml_node *robot_node);

    /*!
     * \brief loads the robot controls file of the problem file,
     creates the controls, adds them to the workspace and creates the mapMatrix and
     offMatrix of every robot. All the robots must have already been loaded.
     * \param cntrFile file where controls are defined
     * \return true if controls could be loaded to the workspace
     */
    bool setRobotControls(string cntrFile);

    /*!
     * \brief loads an obstacle node of the problem file,
     creates the obstacle and adds it to the workspace
     * \param obstacle_node obstacle node with the information of the obstacle
     to add to the workspace
     */
    bool addObstacle2WSpace(xml_node *obstacle_node);

    /*!
     * \brief loads the obstacle controls file of the problem file,
     creates the controls, adds them to the workspace and creates the mapMatrix and
     offMatrix of every osbtacle. All the obstacles must have already been loaded.
     * \param cntrFile file where controls are defined
     * \return true if controls could be loaded to the workspace
     */
    bool setObstacleControls(string cntrFile);

    /*!
     * \brief isFileOK checks if all the information required
     to setup a problem is defined in the dile
     * \param doc problem file correctly parsed
     * \return true if the file seems to be OK
     */
    bool isFileOK (xml_document *doc);

    /*!
     * \brief exists checks if a spcified file exists
     * \param file is the file which existence is to be checked
     * \return true if the file exists
     */
    bool exists(string file);

    /*!
     * \brief checks the file, finds the files defined in the problem file and
     completes their absolute path
     * \param doc problem file to prepare before setting up the porblem
     * \return true if the file could be prepared
     */
    bool prepareFile (xml_document *doc);

    /*!
     * \brief finds all the files defined in the atribute called \param attribute
     of the nodes called \param child that children of the \param parent node.
     Files will be looked for in the specified paths. If file is found, its absolute
     path will be completed
     * \param parent node that contains the files to be found
     * \param child name of the children nodes that contains the files to be found
     * \param attribute name of the atribute that contains the file to be found
     * \param path vector of paths where the file will be recursively looked for until
     the file is found
     * \return true if and only if all the files were found
     */
    bool findAllFiles(xml_node *parent, string child, string attribute,
                      vector<string> path);
	};

    /** @}   end of Doxygen module "libProblem" */
}

#endif  //_PROBLEM_H
