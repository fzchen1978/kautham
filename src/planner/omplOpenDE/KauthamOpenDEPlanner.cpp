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

/* Author: Joan Fontanals Martinez, Muhayyuddin */

#if defined(KAUTHAM_USE_OMPL)
#if defined(KAUTHAM_USE_ODE)

#include"KauthamOpenDEPlanner.h"
#include "planner/omplg/omplplanner.h"
#include "planner/omplc/omplcplanner.h"
#include <sampling/state.h>


class KauthamDEGoal;

namespace Kautham {

namespace omplcplanner{


//Igual que omplcplanner es la base per a tots els planners de control com omplcrrtplanner , etc.. KauthamOpenDEPlanner té com a objectiu servir de base per a tots els planners que derivessin i que usessin la simulació dinàmica) Sobretot ha de reimplementar el mètode trysolve basantse en la demo d'OMPL OpenDERigidBodyPlanning.
// Like omplcplanner is the basis for all planners as omplcrrtplanner control, etc. .. KauthamOpenDEPlanner aims to serve as a basis for deriving all the planners and would use dynamic simulation) Especially should reimplement the method trysolve On the basis of the demo fill OpenDERigidBodyPlanning.

/*! Constructor define all the necessary parameters for planning in dynamic enviroment.
 *  It define the pointer to the dynamic enviroment and pointer to the start space.
 */
KauthamDEPlanner::KauthamDEPlanner(SPACETYPE stype, Sample *init, Sample *goal, SampleSet *samples, WorkSpace *ws) : Planner(stype, init, goal, samples, ws)
  {
      //set intial values from parent class data


            _speedFactor = 1;
            _solved = false;
            //set own initial data (maxspeed) (onlytcp?) they have to be declared in the .h
           _maxspeed = 10;
           _onlyend = false;
           //add planner parameters
           addParameter("PropagationStepSize", _propagationStepSize);
           addParameter("Max Planning Time", _planningTime);
           //addParameter("Step Size", ssize);
           addParameter("Speed Factor", _speedFactor);
           addParameter("Max Speed motors", _maxspeed);
           addParameter("only final link position?", _onlyend);

           //oc::OpenDEEnvironmentPtr env(new KauthamDEtableEnvironment(ws,_maxspeed));
           // envPtr=env;

            oc::OpenDEEnvironmentPtr envPtr(new KauthamDEtableEnvironment(ws,_maxspeed));
            stateSpace = new KauthamDEStateSpace(envPtr);
            stateSpacePtr = ob::StateSpacePtr(stateSpace);

            //const dReal* l=dBodyGetPosition(envPtr->stateBodies_[0]);
            //const dReal* n=dBodyGetPosition(envPtr->stateBodies_[1]);

            //const dReal* ll=dBodyGetPosition(stateSpace->getEnvironment()->stateBodies_[0]);
            //const dReal* nn=dBodyGetPosition(stateSpace->getEnvironment()->stateBodies_[1]);

            //const dReal* lll=dBodyGetPosition(stateSpacePtr->as<oc::OpenDEStateSpace>()->getEnvironment()->stateBodies_[0]);
            //const dReal* nnn=dBodyGetPosition(stateSpacePtr->as<oc::OpenDEStateSpace>()->getEnvironment()->stateBodies_[1]);

  }

  //! void destructor
  KauthamDEPlanner::~KauthamDEPlanner(){

  }
  /*! this is the main function that compute the path in dynamic enviroment.
   * where the control will be applied by the ODE.If it found the siolution it returns
   * true otherwise false.
   */
  bool KauthamDEPlanner::trySolve(void)
  {
  dInitODE2(0);


       Sample* aux=goalSamp();
     dVector3 pos11;
     dBodyCopyPosition (stateSpacePtr->as<oc::OpenDEStateSpace>()->getEnvironment()->stateBodies_[0], pos11);
     dVector3 pos12;
     dBodyCopyPosition ( stateSpacePtr->as<oc::OpenDEStateSpace>()->getEnvironment()->stateBodies_[1], pos12);
     //dVector3 pos13;
     //dBodyCopyPosition ( stateSpacePtr->as<oc::OpenDEStateSpace>()->getEnvironment()->stateBodies_[2], pos13);

       //ob::ScopedState<oc::OpenDEStateSpace> initstate(stateSpacePtr);
       //stateSpacePtr->as<oc::OpenDEStateSpace>()->readState(initstate.get());
       //ss->setStartState(initstate);
       //const ob:: State *s;

      ss->setGoal(ob::GoalPtr(new KauthamDEGoal(ss->getSpaceInformation(),_wkSpace,_onlyend,aux)));
      ob::RealVectorBounds bounds(3);
      bounds.setLow(-150);
      bounds.setHigh(150);
      stateSpace->setVolumeBounds(bounds);

      bounds.setLow(-20);
      bounds.setHigh(20);
      stateSpace->setLinearVelocityBounds(bounds);
      stateSpace->setAngularVelocityBounds(bounds);

        ss->setup();
        ss->print();

        if (ss->solve(20))
        {

            og::PathGeometric p = ss->getSolutionPath().asGeometric();
            std::vector<ob::State*> &states = p.getStates();
/*
           for (unsigned int i = 0 ; i < states.size() ; ++i)
            {

                //const double *pos = states[i]->as<ob::CompoundState>()->as<ob::RealVectorStateSpace::StateType>(1)->values;
                const double *pos = states[i]->as<oc::OpenDEStateSpace::StateType>()->getBodyPosition(0);
                const ob::SO3StateSpace::StateType &ori = states[i]->as<oc::OpenDEStateSpace::StateType>()->getBodyRotation(0);

               const double *o1 = states[i]->as<ob::CompoundState>()->as<ob::RealVectorStateSpace::StateType>(1)->values;
                //const double *o1 = states[i]->as<oc::OpenDEStateSpace::StateType>()->getBodyPosition(1);
                const ob::SO3StateSpace::StateType &po1 = states[i]->as<oc::OpenDEStateSpace::StateType>()->getBodyRotation(1);

                const double *o2 = states[i]->as<ob::CompoundState>()->as<ob::RealVectorStateSpace::StateType>(2)->values;
                //const double *o2 = states[i]->as<oc::OpenDEStateSpace::StateType>()->getBodyPosition(2);
                const ob::SO3StateSpace::StateType &po2 = states[i]->as<oc::OpenDEStateSpace::StateType>()->getBodyRotation(2);


                std::cout <<"Robot = "<<pos[0] << " " << pos[1]<< " " << ori.w<<" "<<ori.x<<" " <<ori.y<<" " <<ori.z;
                std::cout <<" Obstracle1 = "<<o1[0] << " " << o1[1]<< " " << po1.w<<" "<<po1.x<<" " <<po1.y<<" " <<po1.z;
                std::cout <<" Obstracle2 = "<<o2[0] << " " << o2[1]<< " " << po2.w<<" "<<po2.x<<" " <<po2.y<<" " <<po2.z<<std::endl;


           }
*/
/*
        for (unsigned int i = 0 ; i < states.size() ; ++i)
         {
            for (int j=0;(int)stateSpacePtr->as<oc::OpenDEStateSpace>()->getEnvironment()->stateBodies_.size()>j; j++)
            {

                const double *pos = states[i]->as<ob::CompoundState>()->as<ob::RealVectorStateSpace::StateType>(j)->values;
                const ob::SO3StateSpace::StateType &ori = states[i]->as<oc::OpenDEStateSpace::StateType>()->getBodyRotation(j);
                std::cout <<" "<<pos[0] << " " << pos[1]<< " " << ori.w<<" "<<ori.x<<" " <<ori.y<<" " <<ori.z<<" === ";

            }
            std::cout<<std::endl;

         }
*/
            if (true)
            {


                  Sample *smp;
                 _path.clear();
                 clearSimulationPath();
                 vector<bool> collision;
                 for(int i=0;i<states.size();i=i+20)
                 {
                     collision.push_back(stateSpace->evaluateCollision(states[i]));
                     vector<RobConf> rc;
                     RobConf *rcj = new RobConf;
                     smp=new Sample(_wkSpace->getNumRobControls());
                     smp->setMappedConf(_init.at(0)->getMappedConf());
                     vector<KthReal> se3Robcoords;
                     se3Robcoords.resize(7);

                     const double *posRob = states[i]->as<ob::CompoundState>()->as<ob::RealVectorStateSpace::StateType>(0)->values;
                     //const double *posRob = states[i]->as<oc::OpenDEStateSpace::StateType>()->getBodyPosition(0);
                     const ob::SO3StateSpace::StateType &oriRob = states[i]->as<oc::OpenDEStateSpace::StateType>()->getBodyRotation(0);

                     se3Robcoords[0] = posRob[0];
                     se3Robcoords[1] = posRob[1];
                     se3Robcoords[2] = posRob[2];
                     se3Robcoords[3] = oriRob.x;
                     se3Robcoords[4] = oriRob.y;
                     se3Robcoords[5] = oriRob.z;
                     se3Robcoords[6] = oriRob.w;

                     vector< vector <KthReal> > tmpvec;

                    for ( int j=1;(int)stateSpacePtr->as<oc::OpenDEStateSpace>()->getEnvironment()->stateBodies_.size()>j; j++)
                     {
                       //unsigned int _j4 = j * 4;

                        vector <KthReal> se3ObsCoords;

                        se3ObsCoords.resize(7);
                       // const dReal *posObs = dBodyGetPosition(stateSpacePtr->as<oc::OpenDEStateSpace>()->getEnvironment()->stateBodies_[j]);
                       // const dReal *oriObs = dBodyGetQuaternion(stateSpacePtr->as<oc::OpenDEStateSpace>()->getEnvironment()->stateBodies_[j]);
                        /*
                        const double *posObs = states[i]->as<ob::CompoundState>()->as<ob::RealVectorStateSpace::StateType>(_j4)->values;
                        ++_j4;
                        ++_j4;
                        ++_j4;
                        const ob::SO3StateSpace::StateType &oriObs = *states[i]->as<ob::CompoundState>()->as<ob::SO3StateSpace::StateType>(_j4);
                        ++_j4;
                        */
                        const double *posObs = states[i]->as<oc::OpenDEStateSpace::StateType>()->getBodyPosition(j);
                        const ob::SO3StateSpace::StateType &oriObs = states[i]->as<oc::OpenDEStateSpace::StateType>()->getBodyRotation(j);


                        se3ObsCoords[0]=posObs[0];
                        se3ObsCoords[1]=posObs[1];
                        se3ObsCoords[2]=posObs[2];
                        se3ObsCoords[3]=oriObs.x;
                        se3ObsCoords[4]=oriObs.y;
                        se3ObsCoords[5]=oriObs.z;
                        se3ObsCoords[6]=oriObs.w;
                        tmpvec.push_back(se3ObsCoords);

                     }
                    _StateBodies.push_back(tmpvec);
                    tmpvec.clear();
                    SE3Conf se3;
                    se3.setCoordinates(se3Robcoords);
                    rcj->setSE3(se3);
                    rcj->setRn(smp->getMappedConf()[0].getRn());
                    rc.push_back(*rcj);
                     smp->setMappedConf(rc);
                    _path.push_back(smp);
                    _samples->add(smp);
                 }

           for(int i=0;i<_StateBodies.size();i++)
            {
                for(int j=0;j<_StateBodies[0].size();j++)
                {
                    for(int k=0;k<_StateBodies[0][0].size();k++)
                    {
                        std::cout<<_StateBodies[i][j][k]<< " ";
                    }
                    std::cout<<"===";
                }
                std::cout <<"Coll  "<<collision[i];
                std::cout<<std::endl;
            }
            }
            _solved = true;

           return true;
        }

        dCloseODE();

        return 0;

  }
//! setParameter function set the planning parameters for the planners
  bool KauthamDEPlanner::setParameters()
    {
        try{
          HASH_S_K::iterator it = _parameters.find("Speed Factor");
          if(it != _parameters.end())
            _speedFactor = it->second;
          else
            return false;

          it = _parameters.find("Max Planning Time");
          if(it != _parameters.end())
              _planningTime = it->second;
          else
            return false;

          it = _parameters.find("PropagationStepSize");
          if(it != _parameters.end()){
                _propagationStepSize = it->second;
                ss->getSpaceInformation()->setPropagationStepSize(_propagationStepSize);
          }
         else
           return false;

          it = _parameters.find("Max Speed motors");
          if(it != _parameters.end())
              _maxspeed = it->second;
          else
            return false;

          it = _parameters.find("only final link position?");
          if(it != _parameters.end())
               _onlyend = it->second;
          else
            return false;

        }catch(...){
          return false;
        }
        return true;

    }

}

}

#endif //KAUTHAM_USE_ODE
#endif //KAUTHAM_USE_OMPL