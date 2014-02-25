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
*     Copyright (C) 2007 - 2011 by Alexander Pérez and Jan Rosell          *
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
 
 


#if !defined(_ROBOT_H)
#define _ROBOT_H


#include "obstacle.h"
#include "link.h"
#include <mt/transform.h>
#include <libsampling/robconf.h>
//#include <Inventor/VRMLnodes/SoVRMLExtrusion.h>
#include <libkin/inversekinematic.h>
#include <libkin/constrainedkinematic.h>
#include <list>


using namespace std;
	
//class InverseKinematic;

namespace Kautham {

/** \addtogroup libProblem
 *  @{
 */

//! Struct attObj defines the transformation between an object and the robot link to which it is attached.
  struct attObj{
    attObj(){obs=NULL; link=NULL;}
    ~attObj(){obs=NULL; link=NULL;}
    Obstacle*     obs;
    Link*         link;
    mt::Transform trans;
    bool toLink( string linkName ){
      return link->getName() == linkName;
    }
  };

//! Class robot implements a kinematic tree with a free-flying base
  class Robot {
  private:
      ROBOTTYPE         robType; //!< The robot type FREEFLY,CHAIN, TREE, CLOSEDCHAIN
      LIBUSED           libs; //!< Flag indicating the collision-check library used PQP or SOLID.

      string            name; //!< A descriptive name of robot
      KthReal           scale;//!< This is the global scale for all the links that compound the robot. Normally it is equal to one.
      RobWeight*        _weights; //!< Weights that affect the distance computations.
      mt::Transform     _homeTrans; //!< This is the Home Reference frame at time zero (used to calculate the spatial limits).
      SoSeparator*      visModel; //!< Visualitzation model for the path.
      SoSeparator*      collModel; //!< Collision model for the path.

      string            controlsName; //!< Names of the controls, as a string, separated with the vertical bar character.
      int               numControls;  //!< This is the number of control used to command the robot
      int               numCoupledControls; //!< This is the number of controls used to command the robot that are couped with other robots
                                        //!< From the set of controls defined in the input file *.rob, the first numCoupledControls controls
                                        //!< will be those that are coupled with other robots. This is implemented by maintaining those
                                        //!< controls at the same value for all the robots.
      int               nTrunk; //!< Number of control for the trunk in case of TREE robot
      InverseKinematic* _ikine; //!< Defines the inverse kinematics of the robot, if available.
      ConstrainedKinematic* _constrainKin; //!< Defines the constrained kinematics of the robot, if it has one.
      vector<RobConf>   _proposedSolution; //!< Solution path to be drawn.
      SoMFVec3f*        _graphicalPath; //!< This corresponds to translational part of the link origin selected while following the path to be drawn.
      SoSeparator*      _pathSeparator; //!< This is the SoSeparator to visualize the solution path. It is attached to the robot model.
      int               _linkPathDrawn; //!< This is the number of the link whose path will be drawn
      list<attObj>      _attachedObject; //!< List of objects attached to the robot gripper.

      KthReal           _spatialLimits[7][2]; //!< Limits of motions of the base of the robot, in world coordinates.
      KthReal           _homeLimits[7][2]; //!< Limits of motions of the base of the robo, with respect to the robot reference frame.
      vector<Link*>     links; //!< Vector of the robot links, starting at the base and ending at the end effector.In case of Tree robots, each branch is inserted sequentially.
      bool              _autocoll; //!< Flag that indicates if the robot is auto-colliding
      bool              _hasChanged; //!< Flag that indicates if the robot has changed its configuration. To speed up some computations.
      KthReal           *offMatrix; //!< Offset vector. If copuling is generated with PCA it contains the baricenter coordinates, otherwise 0.5.
      KthReal           **mapMatrix; //!< Matrix to compute the robot configuration from the controls. If copuling is generated with PCA it contains the scaled eignevectors.


      DHAPPROACH        dhApproach;//!< It identifies the D-H description method (Standard/Modified).
      bool              se3Enabled; //!< This attribute is true if the robot has a mobile base.
      bool              armed;//!< Flag that shows if the Robot is complete or still is under construction.
      RobConf           _homeConf;     //!< This attribute is the Home configuration of the robot.
      RobConf           _currentConf;  //!< This attribute is the current configuration of the robot.


  public:

    Robot(string robFile, KthReal scale, LIBUSED lib = IVPQP); //!<  Constructor

    inline KthReal** getMapMatrix() const {return mapMatrix;} //!< Returns the mapMatrix.

    inline KthReal* getOffMatrix() const {return offMatrix;} //!< Returns the offMatrix.

    inline string getName() const {return name;} //!< Returns the robot name.

    inline RobConf* getCurrentPos(){return &_currentConf;} //!< Returns the current RobConf used to represent the SE3 position and Rn configuration

    inline RobConf* getHomePos(){ return &_homeConf;} //!< Returns the Home robot configuration

    inline LIBUSED whatLibs(){return libs;}//!< Returns the type of libraries used to build the models.

    inline KthReal* getLimits(int member){return _spatialLimits[member];} //!< Returns the limits of the robot (needed for mobile bases).

    inline ROBOTTYPE getRobotType(){return robType;} //!< Returns the robot type.

    inline KthReal getScale() const {return scale;} //!< Returns the scale.

    inline int getTrunk() const {return nTrunk;} //!< Returns the number of links that compose the trunk of the kinematic tree.

    inline DHAPPROACH getDHApproach(){return dhApproach;} //!< Returns the typs of D-H parameters used

    inline unsigned int getNumJoints(){return ((unsigned int)links.size()) - 1;}//!< Returns the number of joints of the robot (nlinks-1).

    inline unsigned int getNumLinks(){return (unsigned int)links.size();} //!< Returns the number of links of the robot.

    inline bool isSE3Enabled() const {return se3Enabled;} //!< retruns wether the robot has a mobile base

    inline string getControlsName() const {return controlsName;} //!< Returns the string containing the control names, separated by the veritcal line character

    inline int getNumControls(){if( armed ) return numControls; return -1;} //!< Retruns the number of controls

    inline int getNumCoupledControls(){if( armed ) return numCoupledControls; return -1;} //!< Retruns the number of controls that are copupled with another robot

    inline mt::Transform& getLastLinkTransform(){ return
                                    *(((Link*)links.at(links.size()-1))->getTransformation());}

    inline mt::Transform& getLinkTransform(unsigned int numLink){
                                    if(numLink<0 || numLink>=links.size()) numLink = links.size()-1;
                                    return
                                        *(((Link*)links.at(numLink))->getTransformation());} //!< R

    inline mt::Transform& getHomeTransform(){return *(links[0]->getTransformation());} //!< Retruns the transform of the robot base wrt the world

    inline vector<RobConf>& getProposedSolution(){return _proposedSolution;} //!< Returns the Proposed Solution as a vector of RobConf; for visualization purposes.

    inline void setName(string nam){name = nam;} //!< Sets the robot name.

    inline void setRobotType(ROBOTTYPE rob){robType = rob;} //!< Sets the robot type.

    inline void setDHApproach(DHAPPROACH dhA){dhApproach = dhA;} //!< Sets the type of D-H parameters to be used

    inline int setNumCoupledControls(int n){numCoupledControls=n;}

    inline void setLinkPathDrawn(int n){_linkPathDrawn = n;}


    //! Returns the values that weight translations vs. rotations in SE3 distance computations.
    KthReal* getWeightSE3();

    //!< Returns the values that weights the motions of each link in Rn distance computations.
    vector<KthReal>& getWeightRn();

    //! Test for autocollision
    bool autocollision(int t=0);

    //! Add link to the robot
    bool addLink(string name, string ivFile, KthReal theta, KthReal d, KthReal a,
                 KthReal alpha, bool rotational, bool movable, KthReal low,
                 KthReal hi, KthReal w, string parentName, KthReal preTrans[] = NULL);

    //! Add link to the robot
    bool addLink(string name, string ivFile, string collision_ivFile, KthReal scale,
                    Unit3 axis, bool rotational, bool movable, KthReal low, KthReal hi,
                 KthReal w, string parentName, KthReal preTrans[], ode_element ode);

    //! Returns the pointer to link number i
    Link* getLink(unsigned int i);

    //! Retunrs the pointer to the link named linkName
    Link* getLinkByName( string linkName );

    //! Computes direct kinematics
    bool Kinematics(RobConf *robq);

    //! Computes direct kinematics
    bool Kinematics(RobConf& robq);

    //! Computes direct kinematics
    bool Kinematics(SE3Conf& q) ;

    //! Computes direct kinematics
    bool Kinematics(RnConf& q);

    //! Computes direct kinematics
    bool Kinematics(Conf *q);

    //! Computes inverse kinematics
    RobConf& InverseKinematics(vector<KthReal> &target);

    //! Computes inverse kinematics
    RobConf& InverseKinematics(vector<KthReal> &target, vector<KthReal> masterconf,
                               bool maintainSameWrist);

    //! Sets the inverse kinematics to be used
    bool setInverseKinematic(INVKINECLASSES type);

    //! Sets parameters inverse kinematics
    bool setInverseKinematicParameter(string name, KthReal value);

    //! Returns the inverse kinematics used
    InverseKinematic* getIkine(){return _ikine;}

    //! Sets the constrained kinematics
    bool setConstrainedKinematic(CONSTRAINEDKINEMATICS type);

    //! Sets parameters constrained kinematics
    bool setConstrainedKinematicParameter(string name, KthReal value);

    //! Returns the constrained kinematics used
    ConstrainedKinematic* getCkine(){return _constrainKin;}

    //! Computes direct constrrained kinematics
    RobConf& ConstrainedKinematics(vector<KthReal> &target);

    //! Sets the home position of the robot
    void setHomePos(Conf* qh);

    //! Verifies collision with another robot
    bool collisionCheck(Obstacle *obs);

    //! Verifies collision with an obstacle
    bool collisionCheck(Robot *rob);

    //! Verifies distance with another robot
    KthReal distanceCheck(Robot *rob, bool min = true);

    //! Verifies distance with an obstacle
    KthReal distanceCheck(Obstacle *obs, bool min = true);

    //! Sets the values of _spatialLimits[member]
    bool setLimits(int member, KthReal min, KthReal max);

    //! Returns a pointer to the visualitzation model
    SoSeparator* getModel();

    //! Returns a pointer to the collision model
    SoSeparator* getCollisionModel();

    //! Returns a pointer to visualize the model used for collisions
    SoSeparator* getModelFromColl();

    //! Maps from control values to configurations.
    bool control2Pose(vector<KthReal> &values);

    //! Maps from control values to parameters (normalized configurations).
    bool control2Parameters(vector<KthReal> &control, vector<KthReal> &parameters);

    //! Loads the robot configuration _currentConf from the normalized values of the configuration (parameters)
    void parameter2Pose(vector<KthReal> &values);

    //! Depending on the type specified, retruns the SE3 config or the Rn configuration from the normalized values of the configuration (parameters)
    Conf& parameter2Conf(vector<KthReal> &values, CONFIGTYPE type);

    //! Retunrs the weights of the robot used in the computation of the distances
    RobWeight* getRobWeight(){return _weights;}

    //! Sets the value of the mapMatrix corresponding to the column control and row dof.
    bool setControlItem(string control, string dof, KthReal value);

    //! Returns the string with the names of the DOFs, separated by |
    string getDOFNames();

    //! Sets a solution path to be visualized (corresponding to the origin of a given selected link of the robot)
    bool setProposedSolution(vector<RobConf*>& path);

    //! Deletes the solution path to be visualized.
    bool cleanProposedSolution();

    //! Toggles on/off the vision of the solution path.
    bool setPathVisibility(bool visible);

    //! Attachs an object to a given link, usually the end effector.
    bool attachObject(Obstacle* obs, string linkName );

    //! Moves the attached object.
    void moveAttachedObj();

    //! Dettaches the attached object.
    bool detachObject( string linkName );

    //! This method returns the maximum value of the D_H parameters.
    KthReal maxDHParameter();

  private:
    //! This method updates the absolute position and orientation of each link in the robot.
    void updateRobot();

    //! Recomputes the limits in the home frame
    void recalculateHomeLimits();

    //! Denormalizes the SE3 unit representation and returns the positon and the rotation (quaternion) in a single vector.
    vector<KthReal>  deNormalizeSE3(vector<KthReal> &values);

    //! Returns the diagonal of the cube defined by the home-limits of the robot
    float diagLimits();
  };

  /** @}   end of Doxygen module "libProblem" */
}

#endif  //_ROBOT_H

