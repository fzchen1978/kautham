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

 

#if defined(KAUTHAM_USE_OMPL)

#include <libproblem/workspace.h>
#include <libsampling/sampling.h>

#include <boost/bind/mem_fn.hpp>

#include "omplplanner.h"


#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoCube.h>
#include <Inventor/nodes/SoPointSet.h>
#include <Inventor/nodes/SoLineSet.h>

#include <ompl/base/ProblemDefinition.h>
//#include <ompl/base/OptimizationObjective.h>
//#include <ompl/base/objectives/PathLengthOptimizationObjective.h>
//#include <ompl/base/objectives/MaximizeMinClearanceObjective.h>

namespace Kautham {

//! Namespace omplplanner contains the planners based on the OMPL::geometric library
  namespace omplplanner{

  //declaration of class
  class weightedRealVectorStateSpace;


  /////////////////////////////////////////////////////////////////////////////////////////////////
  // weigthedRealVectorStateSpace functions
  /////////////////////////////////////////////////////////////////////////////////////////////////
      //! The constructor initializes all the weights to 1
      weigthedRealVectorStateSpace::weigthedRealVectorStateSpace(unsigned int dim) : RealVectorStateSpace(dim)
      {
          //by default set all the weights to 1
          for(int i=0; i<dim; i++)
          {
              weights.push_back(1.0);
          }
      }

      //! The destructor
      weigthedRealVectorStateSpace::~weigthedRealVectorStateSpace(void){}


      //! This function sets the values of the weights. The values passed as a parameter are scaled in order not to change the maximim extend of the space
      void weigthedRealVectorStateSpace::setWeights(vector<KthReal> w)
      {
          double fitFactor;

          //compute the maximum weigthed distance
          double maxweightdist=0.0;
          for(int i=0; i<dimension_; i++)
          {
              double diff = getBounds().getDifference()[i]*w[i];
              maxweightdist += diff * diff;
          }
          maxweightdist = sqrt(maxweightdist);
          //compute the scale factor
          fitFactor = getMaximumExtent()/maxweightdist;
          //set the weights
          for(int i=0; i<dimension_; i++)
          {
              weights[i] = w[i]*fitFactor;
          }
      }

      //! This function computes the weighted distance between states
      double weigthedRealVectorStateSpace::distance(const ob::State *state1, const ob::State *state2) const
      {
         double dist = 0.0;
         const double *s1 = static_cast<const StateType*>(state1)->values;
         const double *s2 = static_cast<const StateType*>(state2)->values;

        for (unsigned int i = 0 ; i < dimension_ ; ++i)
        {
            double diff = ((*s1++) - (*s2++))*weights[i];
            dist += diff * diff;
        }
        return sqrt(dist);
      }



  /////////////////////////////////////////////////////////////////////////////////////////////////
  // KauthamStateSampler functions
  /////////////////////////////////////////////////////////////////////////////////////////////////
      KauthamStateSampler::KauthamStateSampler(const ob::StateSpace *sspace, Planner *p) : ob::CompoundStateSampler(sspace)
      {
          kauthamPlanner_ = p;
          centersmp = NULL;
  _samplerRandom = new RandomSampler(kauthamPlanner_->wkSpace()->getNumRobControls());
      }

      void KauthamStateSampler::setCenterSample(ob::State *state, double th)
      {
          if(state!=NULL)
          {
            //create sample
            int d = kauthamPlanner_->wkSpace()->getNumRobControls();
            centersmp = new Sample(d);
            //copy the conf of the init smp. Needed to capture the home positions.
            centersmp->setMappedConf(kauthamPlanner_->initSamp()->getMappedConf());
            //load the RobConf of smp form the values of the ompl::state
            ((omplPlanner*)kauthamPlanner_)->omplState2smp(state,centersmp);
          }
          else
              centersmp = NULL;

          //initialize threshold
          threshold = th;
      }      

      void KauthamStateSampler::sampleUniform(ob::State *state)
      {
          //Sample around centersmp
          //this does the same as sampleUniformNear, but the Near configuration is set beforehand as "centersmp" configuration.
          //this has been added to modify the behavior of the randombounce walk of the PRM. It used the sampleUniform and
          //we wanted to use the sampleUniformNear
          if(centersmp != NULL && threshold > 0.0)
          {
            int trials = 0;
            int maxtrials=100;
            bool found = false;
            int d = kauthamPlanner_->wkSpace()->getNumRobControls();
            Sample *smp = new Sample(d);
            double dist;

            bool withinbounds=false;
            do{
                    //sample the kautham control space. Controls are defined in the input xml files. Eeach control value lies in the [0,1] interval
                    vector<KthReal> coords(d);
                    for(int i=0;i<d;i++)
                        coords[i] = rng_.uniformReal(0,1.0);
                    //those controls that are disabled for sampling are now restored to 0.5
                    for(int j=0; j < ((omplPlanner*)kauthamPlanner_)->getDisabledControls()->size(); j++)
                        coords[ ((omplPlanner*)kauthamPlanner_)->getDisabledControls()->at(j) ] = 0.5;


                    //load the obtained coords to a sample, and compute the mapped configurations (i.e.se3+Rn values) by calling MoveRobotsto function.
                    smp->setCoords(coords);
                    kauthamPlanner_->wkSpace()->moveRobotsTo(smp);
                    withinbounds = smp->getwithinbounds();
                    //if within bounds then check if its within the given distance threshold
                    if(withinbounds)
                    {
                        dist = kauthamPlanner_->wkSpace()->distanceBetweenSamples(*smp,*centersmp,CONFIGSPACE);
                        if(dist < threshold)
                            found = true;
                    }
                    trials ++;
            }while(found==false && trials <maxtrials);


            ob::ScopedState<ob::CompoundStateSpace> sstate(  ((omplPlanner*)kauthamPlanner_)->getSpace() );
            if(trials==maxtrials)
            {
                //not found within the limits. return the centersmp
                ((omplPlanner*)kauthamPlanner_)->smp2omplScopedState(centersmp, &sstate);
            }
            else
            {
                //convert the sample found to scoped state
                ((omplPlanner*)kauthamPlanner_)->smp2omplScopedState(smp, &sstate);
            }

            //return in parameter state
            ((omplPlanner*)kauthamPlanner_)->getSpace()->copyState(state, sstate.get());
         }
        //sample the whole workspace
         else
         {
              /*
              //sample the kautham control space. Controls are defined in the input xml files. Eeach control value lies in the [0,1] interval
              int d = kauthamPlanner_->wkSpace()->getNumRobControls();
              vector<KthReal> coords(d);
              for(int i=0;i<d;i++)
                  coords[i] = rng_.uniformReal(0,1.0);

              //load the obtained coords to a sample, and compute the mapped configurations (i.e.se3+Rn values) by calling MoveRobotsto function.
              Sample *smp = new Sample(d);
              smp->setCoords(coords);
              kauthamPlanner_->wkSpace()->moveRobotsTo(smp);

              //convert from sample to scoped state
              ob::ScopedState<ob::CompoundStateSpace> sstate(  ((omplPlanner*)kauthamPlanner_)->getSpace() );
              ((omplPlanner*)kauthamPlanner_)->smp2omplScopedState(smp, &sstate);

              //return in parameter state
             ((omplPlanner*)kauthamPlanner_)->getSpace()->copyState(state, sstate.get());
              */

              bool withinbounds=false;
              int trials=0;
              Sample* smp = NULL;
              do{
                //sample the kautham control space. Controls are defined in the input xml files. Eeach control value lies in the [0,1] interval
                smp = _samplerRandom->nextSample();

                //those controls that are disabled for sampling are now restored to 0.5
                for(int j=0; j<((omplPlanner*)kauthamPlanner_)->getDisabledControls()->size(); j++)
                    smp->getCoords()[ ((omplPlanner*)kauthamPlanner_)->getDisabledControls()->at(j) ] = 0.5;

                //compute the mapped configurations (i.e.se3+Rn values) by calling MoveRobotsto function.
                kauthamPlanner_->wkSpace()->moveRobotsTo(smp);
                withinbounds = smp->getwithinbounds();
                trials++;
              }while(withinbounds==false && trials<100);

              //If trials==100 is because we have not been able to find a sample within limits
              //In this case the config is set to the border in the moveRobotsTo function.
              //The smp is finally converted to state and returned

              //convert from sample to scoped state
              ob::ScopedState<ob::CompoundStateSpace> sstate(  ((omplPlanner*)kauthamPlanner_)->getSpace() );
              ((omplPlanner*)kauthamPlanner_)->smp2omplScopedState(smp, &sstate);
              //return in parameter state
              ((omplPlanner*)kauthamPlanner_)->getSpace()->copyState(state, sstate.get());
         }
      }


      void KauthamStateSampler::sampleUniformNear(ob::State *state, const ob::State *near, const double distance)
           {
               int trials = 0;
               int maxtrials=100;
               bool found = false;
               Sample *smp;
               do{
                 bool withinbounds=false;
                 int trialsbounds=0;
                 do{
                      //sample the kautham control space. Controls are defined in the input xml files. Eeach control value lies in the [0,1] interval
                     smp = _samplerRandom->nextSample();
                     //load the obtained coords to a sample, and compute the mapped configurations (i.e.se3+Rn values) by calling MoveRobotsto function.
                      kauthamPlanner_->wkSpace()->moveRobotsTo(smp);
                      withinbounds = smp->getwithinbounds();
                      trialsbounds++;
                 }while(withinbounds==false && trialsbounds<100);
                 //convert from sample to scoped state
                 ob::ScopedState<ob::CompoundStateSpace> sstate(  ((omplPlanner*)kauthamPlanner_)->getSpace() );
                 ((omplPlanner*)kauthamPlanner_)->smp2omplScopedState(smp, &sstate);
                 //return the stae in the parameter state and a bool telling if the smp is in collision or not
                 ((omplPlanner*)kauthamPlanner_)->getSpace()->copyState(state, sstate.get());
                 if (((omplPlanner*)kauthamPlanner_)->getSpace()->distance(state,near)> distance)
                     found = false;
                 else
                     found=true;
                  trials ++;
               }while(found==false && trials <maxtrials);

                if (!found){
                ((omplPlanner*)kauthamPlanner_)->getSpace()->copyState(state, near);
                }


               //throw ompl::Exception("KauthamValidStateSampler::sampleNear", "not implemented");
               //return false;
           }


  /////////////////////////////////////////////////////////////////////////////////////////////////
  // KauthamValidStateSampler functions
  /////////////////////////////////////////////////////////////////////////////////////////////////
      //! Creator. The parameter samplername is defaulted to "Random" and the value to "0.0
      KauthamValidStateSampler::KauthamValidStateSampler(const ob::SpaceInformation *si, Planner *p) : ob::ValidStateSampler(si)
      {
          name_ = "kautham sampler";
          kauthamPlanner_ = p;
          si_ = si;
          //be careful these values should be set somehow!
          int level = 3;
          KthReal sigma = 0.1;

          _samplerRandom = new RandomSampler(kauthamPlanner_->wkSpace()->getNumRobControls());
          _samplerHalton = new HaltonSampler(kauthamPlanner_->wkSpace()->getNumRobControls());
          _samplerSDK = new SDKSampler(kauthamPlanner_->wkSpace()->getNumRobControls(), level);
          _samplerGaussian = new GaussianSampler(kauthamPlanner_->wkSpace()->getNumRobControls(), sigma, kauthamPlanner_->wkSpace());
          _samplerGaussianLike = new GaussianLikeSampler(kauthamPlanner_->wkSpace()->getNumRobControls(), level, kauthamPlanner_->wkSpace());

          _samplerVector.push_back(_samplerRandom);
          _samplerVector.push_back(_samplerHalton);
          _samplerVector.push_back(_samplerSDK);
          _samplerVector.push_back(_samplerGaussian);
          _samplerVector.push_back(_samplerGaussianLike);
       }


      //!Gets a sample. The samplername parameter is defaulted to Random.
      //bool KauthamValidStateSampler::sample(ob::State *state, string samplername)
      bool KauthamValidStateSampler::sample(ob::State *state)
      {
          //gets a new sample using the sampler specified by the planner
          Sample* smp = NULL;
          int numSampler = ((omplPlanner*)kauthamPlanner_)->getSamplerUsed();
          if(numSampler>= _samplerVector.size()) numSampler = 0;//set default Random sampler if out of bounds value
          smp = _samplerVector[numSampler]->nextSample();

          //those controls that are disabled for sampling are now restored to 0.5
          for(int j=0; j<((omplPlanner*)kauthamPlanner_)->getDisabledControls()->size(); j++)
              smp->getCoords()[ ((omplPlanner*)kauthamPlanner_)->getDisabledControls()->at(j) ] = 0.5;


          //computes the mapped configurations (i.e.se3+Rn values) by calling MoveRobotsto function.
          kauthamPlanner_->wkSpace()->moveRobotsTo(smp);

          //convert from sample to scoped state
          ob::ScopedState<ob::CompoundStateSpace> sstate(  ((omplPlanner*)kauthamPlanner_)->getSpace() );
          ((omplPlanner*)kauthamPlanner_)->smp2omplScopedState(smp, &sstate);

          //return the stae in the parameter state and a bool telling if the smp is in collision or not
          ((omplPlanner*)kauthamPlanner_)->getSpace()->copyState(state, sstate.get());

          if(  si_->satisfiesBounds(state)==false | kauthamPlanner_->wkSpace()->collisionCheck(smp) )
          //if( kauthamPlanner_->wkSpace()->collisionCheck(smp) )
              return false;
          return true;
      }

      //!Gets a sample near a given state, after several trials (retruns false if not found)
      bool KauthamValidStateSampler::sampleNear(ob::State *state, const ob::State *near, const double distance)
           {
               int trials = 0;
               int maxtrials=100;
               bool found = false;
               do{
                 //get a random sample, and compute the mapped configurations (i.e.se3+Rn values) by calling MoveRobotsto function.
                 Sample* smp = NULL;
                 int numSampler = 0; //Random sampler
                 smp = _samplerVector[numSampler]->nextSample();

                 //those controls that are disabled for sampling are now restored to 0.5
                 for(int j=0; j<((omplPlanner*)kauthamPlanner_)->getDisabledControls()->size(); j++)
                     smp->getCoords()[ ((omplPlanner*)kauthamPlanner_)->getDisabledControls()->at(j) ] = 0.5;


                 kauthamPlanner_->wkSpace()->moveRobotsTo(smp);
                 //convert from sample to scoped state
                 ob::ScopedState<ob::CompoundStateSpace> sstate(  ((omplPlanner*)kauthamPlanner_)->getSpace() );
                 ((omplPlanner*)kauthamPlanner_)->smp2omplScopedState(smp, &sstate);
                 //return the stae in the parameter state and a bool telling if the smp is in collision or not
                 ((omplPlanner*)kauthamPlanner_)->getSpace()->copyState(state, sstate.get());
                 if( kauthamPlanner_->wkSpace()->collisionCheck(smp) | (((omplPlanner*)kauthamPlanner_)->getSpace()->distance(state,near)> distance) | !(si_->satisfiesBounds(state)))
                     found = false;
                 else
                     found=true;
                  trials ++;
               }while(found==false && trials <maxtrials);
               return found;
               //throw ompl::Exception("KauthamValidStateSampler::sampleNear", "not implemented");
               //return false;
           }


  /////////////////////////////////////////////////////////////////////////////////////////////////
  // AUXILIAR functions
  /////////////////////////////////////////////////////////////////////////////////////////////////

  /////////////////////////////////////////////////////////////////////////////////////////////////
  //! This function is used to allocate a state sampler
  ob::StateSamplerPtr allocStateSampler(const ob::StateSpace *mysspace, Planner *p)
  {
        return ob::StateSamplerPtr(new KauthamStateSampler(mysspace, p));

/*
      //Create sampler
      ob::StateSamplerPtr globalSampler(new ob::CompoundStateSampler(mysspace));
      //weights defined for when sampling near a state
      //they are now all set to 1.0. To be explored later...
      double weightImportanceRobots; //weight of robot i
      double weightSO3; //rotational weight
      double weightR3; //translational weight
      double weightSE3; //weight of the monile base
      double weightRn; //weight of the chain
      //loop for all robots
      for(int i=0; i<p->wkSpace()->getNumRobots(); i++)
      {
          weightImportanceRobots = 1.0; //all robots weight the same

          //Create sampler for robot i
          //ssRoboti is the subspace corresponding to robot i
          ob::StateSpacePtr ssRoboti = ((ob::StateSpacePtr) mysspace->as<ob::CompoundStateSpace>()->getSubspace(i));
          ob::StateSamplerPtr samplerRoboti(new ob::CompoundStateSampler(ssRoboti.get()));

          int numsubspace=0;
          //If SE3 workspace exisits
          if(p->wkSpace()->getRobot(i)->isSE3Enabled())
          {
              //ssRoboti_sub is the subspace corresponding to the SE3 part of robot i
              ob::StateSpacePtr ssRoboti_sub_SE3 =  ((ob::StateSpacePtr) ssRoboti->as<ob::CompoundStateSpace>()->getSubspace(numsubspace));
              numsubspace++;
              //the sampler is a compound sampler
              ob::StateSamplerPtr samplerRoboti_SE3(new ob::CompoundStateSampler(ssRoboti_sub_SE3.get()));
              //ssRoboti_sub_R3 is the R3 subspace of robot i
              ob::StateSpacePtr ssRoboti_sub_R3  = ((ob::StateSpacePtr) ssRoboti_sub_SE3->as<ob::SE3StateSpace>()->getSubspace(0));
              //add the sampler of the R3 part
              weightR3=1.0;
              ob::StateSamplerPtr samplerRoboti_R3(new ob::RealVectorStateSampler(ssRoboti_sub_R3.get()));
              ((ob::CompoundStateSampler*) samplerRoboti_SE3.get())->addSampler(samplerRoboti_R3, weightR3);
              //ssRoboti_sub_SO3 is the SO3 subspace of robot i
              ob::StateSpacePtr ssRoboti_sub_SO3 = ((ob::StateSpacePtr) ssRoboti_sub_SE3->as<ob::SE3StateSpace>()->getSubspace(1));
              //add the sampler of the SO3 part
              weightSO3=1.0;
              ob::StateSamplerPtr samplerRoboti_SO3(new ob::SO3StateSampler(ssRoboti_sub_SO3.get()));
              ((ob::CompoundStateSampler*) samplerRoboti_SE3.get())->addSampler(samplerRoboti_SO3, weightSO3);
              //add the compound sampler of the SE3 part
              weightSE3 = 1.0;
              ((ob::CompoundStateSampler*) samplerRoboti.get())->addSampler(samplerRoboti_SE3, weightSE3);
          }
          //If Rn state space exisits
          if(p->wkSpace()->getRobot(i)->getNumJoints()>0)
          {
              //ssRoboti_sub is the subspace corresponding to the Rn part of robot i
              ob::StateSpacePtr ssRoboti_sub_Rn =  ((ob::StateSpacePtr) ssRoboti->as<ob::CompoundStateSpace>()->getSubspace(numsubspace));
              //add the sampler of the Rn part
              weightRn = 1.0;
              ob::StateSamplerPtr samplerRoboti_Rn(new ob::RealVectorStateSampler(ssRoboti_sub_Rn.get()));
              ((ob::CompoundStateSampler*) samplerRoboti.get())->addSampler(samplerRoboti_Rn, weightRn);
          }
          //add the sampler of robot i to global sampler
          ((ob::CompoundStateSampler*) globalSampler.get())->addSampler(samplerRoboti, weightImportanceRobots);
      }

      return globalSampler;
*/

  }

  /////////////////////////////////////////////////////////////////////////////////////////////////
  //! This function is used to allocate a valid state sampler
  ob::ValidStateSamplerPtr allocValidStateSampler(const ob::SpaceInformation *si, Planner *p)
  {
      return ob::ValidStateSamplerPtr(new KauthamValidStateSampler(si, p));
  }

//  /////////////////////////////////////////////////////////////////////////////////////////////////
//  //! This function converts a state to a smp and tests if it is in collision or not
//  bool isStateValid(const ob::SpaceInformation *si, const ob::State *state, Planner *p)
//  {
//      //verify bounds
//      if(si->satisfiesBounds(state)==false)
//          return false;
//      //create sample
//      int d = p->wkSpace()->getNumRobControls();
//      Sample *smp = new Sample(d);
//      //copy the conf of the init smp. Needed to capture the home positions.
//      smp->setMappedConf(p->initSamp()->getMappedConf());
//      //load the RobConf of smp form the values of the ompl::state
//      ((omplPlanner*)p)->omplState2smp(state,smp);
//      //collision-check
//      if( p->wkSpace()->collisionCheck(smp) )
//          return false;
//      return true;
//  }




  /////////////////////////////////////////////////////////////////////////////////////////////////
  // omplPlanner functions
  /////////////////////////////////////////////////////////////////////////////////////////////////
  //! Constructor
  omplPlanner::omplPlanner(SPACETYPE stype, Sample *init, Sample *goal, SampleSet *samples, WorkSpace *ws, og::SimpleSetup *ssptr):
             Planner(stype, init, goal, samples, ws)
    {
        _family = "ompl";
        //set intial values from parent class data
        _speedFactor = 1;
        _solved = false;
        _guiName = "ompl Planner";
        _idName = "ompl Planner";

        _samplerUsed = 0;

        //set own intial values
        _planningTime = 10;
        _simplify = 2;//by default shorten and smooth
        _incremental=0;//by default makes a clear before any new call to solve in function trysolve().
        _drawnrobot=0; //by default we draw the cspace of robot 0.

        //add planner parameters
        addParameter("Incremental (0/1)", _incremental);
        addParameter("Max Planning Time", _planningTime);
        addParameter("Speed Factor", _speedFactor);
        addParameter("Simplify Solution", _simplify);
        addParameter("Cspace Drawn", _drawnrobot);


        if (ssptr == NULL) {
            //Construct the state space we are planning in. It is a compound state space composed of a compound state space for each robot
            //Each robot has a compound state space composed of a (oprional) SE3 state space and a (optional) Rn state space
            vector<ob::StateSpacePtr> spaceRn;
            vector<ob::StateSpacePtr> spaceSE3;
            vector<ob::StateSpacePtr> spaceRob;
            vector< double > weights;

            spaceRn.resize(_wkSpace->getNumRobots());
            spaceSE3.resize(_wkSpace->getNumRobots());
            spaceRob.resize(_wkSpace->getNumRobots());
            weights.resize(_wkSpace->getNumRobots());

            //loop for all robots
            for(int i=0; i<_wkSpace->getNumRobots(); i++)
            {
                vector<ob::StateSpacePtr> compoundspaceRob;
                vector< double > weightsRob;
                std::stringstream sstm;

                //create state space SE3 for the mobile base, if necessary
                if(_wkSpace->getRobot(i)->isSE3Enabled())
                {
                    //create the SE3 state space
                    spaceSE3[i] = ((ob::StateSpacePtr) new ob::SE3StateSpace());
                    sstm << "ssRobot" << i<<"_SE3";
                    spaceSE3[i]->setName(sstm.str());

                    //set the bounds. If the bounds are equal or its difference is below a given epsilon value (0.001) then
                    //set the higher bound to the lower bound plus this eplsion
                    ob::RealVectorBounds bounds(3);

                    //x-direction
                    double low = _wkSpace->getRobot(i)->getLimits(0)[0];
                    double high = _wkSpace->getRobot(i)->getLimits(0)[1];
                    filterBounds(low, high, 0.001);
                    bounds.setLow(0, low);
                    bounds.setHigh(0, high);

                    //y-direction
                    low = _wkSpace->getRobot(i)->getLimits(1)[0];
                    high = _wkSpace->getRobot(i)->getLimits(1)[1];
                    filterBounds(low, high, 0.001);
                    bounds.setLow(1, low);
                    bounds.setHigh(1, high);

                    //z-direction
                    low = _wkSpace->getRobot(i)->getLimits(2)[0];
                    high = _wkSpace->getRobot(i)->getLimits(2)[1];
                    filterBounds(low, high, 0.001);
                    bounds.setLow(2, low);
                    bounds.setHigh(2, high);

                    spaceSE3[i]->as<ob::SE3StateSpace>()->setBounds(bounds);

                    //create projections evaluator for this spaces -
                    //The default projections (needed for some planners) and
                    //the projections called "drawprojections"for the drawcspace function.
                    //(the use of defaultProjections for drawspace did not succeed because the ss->setup() calls registerProjections() function that
                    //for the case of RealVectorStateSpace sets the projections as random for dim>2 and identity otherwise, thus
                    //resetting what the user could have tried to do.
                    ob::ProjectionEvaluatorPtr peR3; //projection for R3
                    peR3 = (ob::ProjectionEvaluatorPtr) new ob::RealVectorIdentityProjectionEvaluator(spaceSE3[i]->as<ob::SE3StateSpace>()->getSubspace(0));
                    peR3->setup();//??
                    spaceSE3[i]->as<ob::SE3StateSpace>()->getSubspace(0)->registerProjection("drawprojection",peR3);
                    spaceSE3[i]->as<ob::SE3StateSpace>()->getSubspace(0)->registerDefaultProjection(peR3);
                    ob::ProjectionEvaluatorPtr peSE3; //projection for SE3
                    ob::ProjectionEvaluatorPtr projToUse = spaceSE3[i]->as<ob::CompoundStateSpace>()->getSubspace(0)->getProjection("drawprojection");
                    peSE3 = (ob::ProjectionEvaluatorPtr) new ob::SubspaceProjectionEvaluator(&*spaceSE3[i],0,projToUse);
                    peSE3->setup(); //necessary to set projToUse as theprojection
                    spaceSE3[i]->registerProjection("drawprojection",peSE3);
                    spaceSE3[i]->registerDefaultProjection(peSE3);

                    //sets the weights between translation and rotation
                    spaceSE3[i]->as<ob::SE3StateSpace>()->setSubspaceWeight(0,_wkSpace->getRobot(i)->getWeightSE3()[0]);//translational weight
                    spaceSE3[i]->as<ob::SE3StateSpace>()->setSubspaceWeight(1,_wkSpace->getRobot(i)->getWeightSE3()[1]);//rotational weight

                    //load to the compound state space of robot i
                    compoundspaceRob.push_back(spaceSE3[i]);
                    weightsRob.push_back(1);
                }

                //create the Rn state space for the kinematic chain, if necessary
                int nj = _wkSpace->getRobot(i)->getNumJoints();
                if(nj>0)
                {
                    //create the Rn state space
                    spaceRn[i] = ((ob::StateSpacePtr) new weigthedRealVectorStateSpace(nj));
                    sstm << "ssRobot" << i<<"_Rn";
                    spaceRn[i]->setName(sstm.str());

                    //create projections evaluator for this spaces
                    ob::ProjectionEvaluatorPtr peRn;
                    peRn = ((ob::ProjectionEvaluatorPtr) new ob::RealVectorIdentityProjectionEvaluator(spaceRn[i]));
                    peRn->setup();
                    spaceRn[i]->registerProjection("drawprojection",peRn);
                    spaceRn[i]->registerDefaultProjection(peRn);

                    // set the bounds and the weights
                    vector<KthReal> jointweights;
                    ob::RealVectorBounds bounds(nj);
                    double low, high;
                    for(int j=0; j<nj;j++)
                    {
                        //the limits of joint j between link j and link (j+1) are stroed in the data structure of link (j+1)
                        low = *_wkSpace->getRobot(i)->getLink(j+1)->getLimits(true);
                        high = *_wkSpace->getRobot(i)->getLink(j+1)->getLimits(false);
                        filterBounds(low, high, 0.001);
                        bounds.setLow(j, low);
                        bounds.setHigh(j, high);
                        //the weights
                        jointweights.push_back(_wkSpace->getRobot(i)->getLink(j+1)->getWeight());
                    }
                    spaceRn[i]->as<weigthedRealVectorStateSpace>()->setBounds(bounds);
                    spaceRn[i]->as<weigthedRealVectorStateSpace>()->setWeights(jointweights);

                    //load to the compound state space of robot i
                    compoundspaceRob.push_back(spaceRn[i]);
                    weightsRob.push_back(1);
                }
                //the compound state space for robot i is (SE3xRn), and either SE3 or Rn may be missing
                spaceRob[i] = ((ob::StateSpacePtr) new ob::CompoundStateSpace(compoundspaceRob,weightsRob));
                weights[i] = 1;
                sstm.str("");
                sstm << "ssRobot" << i;
                spaceRob[i]->setName(sstm.str());

                ob::ProjectionEvaluatorPtr peRob;
                ob::ProjectionEvaluatorPtr projToUse = spaceRob[i]->as<ob::CompoundStateSpace>()->getSubspace(0)->getProjection("drawprojection");
                peRob = (ob::ProjectionEvaluatorPtr) new ob::SubspaceProjectionEvaluator(&*spaceRob[i],0,projToUse);
                peRob->setup();
                spaceRob[i]->registerProjection("drawprojection",peRob);
                spaceRob[i]->registerDefaultProjection(peRob);
            }
            //the state space for the set of robots. All the robots have the same weight.
            space = ((ob::StateSpacePtr) new ob::CompoundStateSpace(spaceRob,weights));

            /*
        ob::ProjectionEvaluatorPtr peSpace;
        ob::ProjectionEvaluatorPtr projToUse = space->as<ob::CompoundStateSpace>()->getSubspace(0)->getProjection("drawprojection");
        peSpace = (ob::ProjectionEvaluatorPtr) new ob::SubspaceProjectionEvaluator(&*space,0,projToUse);
        peSpace->setup();
        space->registerProjection("drawprojection",peSpace);
        space->registerDefaultProjection(peSpace);
        */
            vector<ob::ProjectionEvaluatorPtr> peSpace;
            for(int i=0; i<_wkSpace->getNumRobots();i++)
            {
                ob::ProjectionEvaluatorPtr projToUse = space->as<ob::CompoundStateSpace>()->getSubspace(i)->getProjection("drawprojection");
                peSpace.push_back( (ob::ProjectionEvaluatorPtr) new ob::SubspaceProjectionEvaluator(&*space,i,projToUse) );
                peSpace[i]->setup();
                string projname = "drawprojection"; //
                string robotnumber = static_cast<ostringstream*>( &(ostringstream() << i) )->str();//the string correspoding to number i
                projname.append(robotnumber); //the name of the projection: "drawprojection0", "drawprojection1",...
                space->registerProjection(projname.c_str(),peSpace[i]);
            }
            space->registerDefaultProjection(peSpace[0]);//the one corresponding to the first robot is set as default

            //create simple setup
            ss = ((og::SimpleSetupPtr) new og::SimpleSetup(space));
            si=ss->getSpaceInformation();
            //set validity checker
            si->setStateValidityChecker(ob::StateValidityCheckerPtr(new ValidityChecker(si,  (Planner*)this)));
            //ss->setStateValidityChecker(boost::bind(&omplplanner::isStateValid, si.get(),_1, (Planner*)this));

            //Start state: convert from smp to scoped state
            ob::ScopedState<ob::CompoundStateSpace> startompl(space);
            smp2omplScopedState(_init, &startompl);
            cout<<"startompl:"<<endl;
            startompl.print();

            //Goal state: convert from smp to scoped state
            ob::ScopedState<ob::CompoundStateSpace> goalompl(space);
            smp2omplScopedState(_goal, &goalompl);
            cout<<"goalompl:"<<endl;
            goalompl.print();

            // set the start and goal states
            ss->setStartAndGoalStates(startompl, goalompl);
        } else {
            ss = (og::SimpleSetupPtr)ssptr;
            si = ss->getSpaceInformation();
            space = ss->getStateSpace();
        }
    }

	//! void destructor
    omplPlanner::~omplPlanner(){
			
	}

    /*!
     * disablePMDControlsFromSampling disables from sampling those controls that have the PMD in its name.
     * \param enableall if its true the function is used to enable all, It is defaulted to false.
     */
    void omplPlanner::disablePMDControlsFromSampling(bool enableall)
    {
        //enable all
        if(enableall)
        {
            _disabledcontrols.clear();
            return;
        }
        //else diable those that are called PMD


        string listcontrolsname = wkSpace()->getRobControlsName();
        vector<string*> controlname;
        string *newcontrol = new string;
        for(int i=0; i<listcontrolsname.length();i++)
        {
            if(listcontrolsname[i]=='|')
            {
                controlname.push_back(newcontrol);
                newcontrol = new string;
            }
            else
                newcontrol->push_back(listcontrolsname[i]);
        }
        //add last control (since listcontrolsname does not end with a |)
        controlname.push_back(newcontrol);

        for(int i=0;i<controlname.size();i++)
        {
            if(controlname[i]->find("PMD") != string::npos)
            {
                //Load to the diable vector for disabling sampling. We do not want to sample coupled controls.
                _disabledcontrols.push_back(i);
            }
        }
    }

	
    //! This function setParameters sets the parameters of the planner
    bool omplPlanner::setParameters(){
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


        it = _parameters.find("Cspace Drawn");
        if(it != _parameters.end()){
            _drawnrobot = it->second;
            if(_drawnrobot<0 || _drawnrobot >= _wkSpace->getNumRobots()) {
                _drawnrobot = 0;
                setParameter("Cspace Drawn",0);
            }
        }
        else
          return false;

        it = _parameters.find("Simplify Solution");
        if(it != _parameters.end())
        {
            if(it->second==0) _simplify=0;
            else if(it->second==1) _simplify=1;
            else _simplify=2;
        }
        else
          return false;

        it = _parameters.find("Incremental (0/1)");
        if(it != _parameters.end()){
            if(it->second == 0) _incremental = 0;
            else {
                _incremental = 1;
                //_simplify = 0;//for incremental solution the smootihng of the path is disabled to spped up the process
                //setParameter("Simplify Solution", _simplify);
            }
        }
        else
          return false;

      }catch(...){
        return false;
      }
      return true;
    }

    //! This function is used to verify that the low bound is below the high bound
    void omplPlanner::filterBounds(double &l, double &h, double epsilon)
    {
        if((h - l) < epsilon) h = l + epsilon;
    }

    //! This function creates the separator for the ivscene to show the configuration space.
    SoSeparator *omplPlanner::getIvCspaceScene()
    {
        _sceneCspace = new SoSeparator();
        return Planner::getIvCspaceScene();
    }


    //! This routine allows to draw the 2D projection of a roadmap or tree. The one corresponding to robot number numrob is drawn.
   void omplPlanner::drawCspace(int numrob)
    {
       if(_sceneCspace==NULL) return;

            //first delete whatever is already drawn
            while (_sceneCspace->getNumChildren() > 0)
            {
                _sceneCspace->removeChild(0);
            }

            //to draw points
            SoSeparator *psep = new SoSeparator();
            SoCoordinate3 *points  = new SoCoordinate3();
            SoPointSet *pset  = new SoPointSet();

            //get the first subspace
            ob::StateSpacePtr ssRoboti = ((ob::StateSpacePtr) space->as<ob::CompoundStateSpace>()->getSubspace(numrob));
            ob::StateSpacePtr ssRobotifirst =  ((ob::StateSpacePtr) ssRoboti->as<ob::CompoundStateSpace>()->getSubspace(0));

            //space bounds
            int k;
            KthReal xmin;
            KthReal xmax;
            KthReal ymin;
            KthReal ymax;
            KthReal zmin;
            KthReal zmax;

            if(_wkSpace->getRobot(numrob)->isSE3Enabled())
            {

                xmin=ssRobotifirst->as<ob::SE3StateSpace>()->getBounds().low[0];
                xmax=ssRobotifirst->as<ob::SE3StateSpace>()->getBounds().high[0];
                ymin=ssRobotifirst->as<ob::SE3StateSpace>()->getBounds().low[1];
                ymax=ssRobotifirst->as<ob::SE3StateSpace>()->getBounds().high[1];
                zmin=ssRobotifirst->as<ob::SE3StateSpace>()->getBounds().low[2];
                zmax=ssRobotifirst->as<ob::SE3StateSpace>()->getBounds().high[2];
                k = ssRobotifirst->as<ob::SE3StateSpace>()->getDimension();
            }
            else
            {
                k = ssRobotifirst->as<ob::RealVectorStateSpace>()->getDimension();
                if(k<=2)
                {
                    xmin=ssRobotifirst->as<ob::RealVectorStateSpace>()->getBounds().low[0];
                    xmax=ssRobotifirst->as<ob::RealVectorStateSpace>()->getBounds().high[0];
                    ymin=ssRobotifirst->as<ob::RealVectorStateSpace>()->getBounds().low[1];
                    ymax=ssRobotifirst->as<ob::RealVectorStateSpace>()->getBounds().high[1];
                }
                else
                {
                    xmin=ssRobotifirst->as<ob::RealVectorStateSpace>()->getBounds().low[0];
                    xmax=ssRobotifirst->as<ob::RealVectorStateSpace>()->getBounds().high[0];
                    ymin=ssRobotifirst->as<ob::RealVectorStateSpace>()->getBounds().low[1];
                    ymax=ssRobotifirst->as<ob::RealVectorStateSpace>()->getBounds().high[1];
                    zmin=ssRobotifirst->as<ob::RealVectorStateSpace>()->getBounds().low[2];
                    zmax=ssRobotifirst->as<ob::RealVectorStateSpace>()->getBounds().high[2];
                }
            }


            KthReal x,y,z;
            //load the planner data to be drawn
            ob::PlannerDataPtr pdata;
            pdata = ((ob::PlannerDataPtr) new ob::PlannerData(ss->getSpaceInformation()));
            ss->getPlanner()->getPlannerData(*pdata);

            if(ss->getPlanner()->getProblemDefinition()->hasOptimizationObjective())
                pdata->computeEdgeWeights( *ss->getPlanner()->getProblemDefinition()->getOptimizationObjective() );
            else
                pdata->computeEdgeWeights();


            //Use the rpojection associated to the subspace of the robot index passed as a parameter.
            string projname = "drawprojection"; //
            string robotnumber = static_cast<ostringstream*>( &(ostringstream() << numrob) )->str();//the string correspoding to number numrob
            projname.append(robotnumber); //the name of the projection: "drawprojection0", "drawprojection1",...
            ob::ProjectionEvaluatorPtr projToUse = space->getProjection(projname.c_str());

            //draw path:
            if(_solved)
            {
                //separator for the solution path
                SoSeparator *pathsep = new SoSeparator();
                //get the states of the solution path
                std::vector< ob::State * > & pathstates = ss->getSolutionPath().getStates();

                //loop for al the states of the solution path
                for(int i=0; i<ss->getSolutionPath().getStateCount()-1; i++)
                {
                    //initial edgepoint
                    SoCoordinate3 *edgepoints  = new SoCoordinate3();
                    if(_wkSpace->getRobot(numrob)->isSE3Enabled())
                    {
                        ob::EuclideanProjection projection(k);
                        //space->getProjection("drawprojection")->project(pathstates[i], projection);
                        projToUse->project(pathstates[i], projection);
                        x=projection[0];
                        y=projection[1];
                        z=projection[2];
                        edgepoints->point.set1Value(0,x,y,z);

                    //final edgepoint
                        //space->getProjection("drawprojection")->project(pathstates[i+1], projection);
                        projToUse->project(pathstates[i+1], projection);
                        x=projection[0];
                        y=projection[1];
                        z=projection[2];
                        edgepoints->point.set1Value(1,x,y,z);
                    }
                    else
                    {
                        k = ssRobotifirst->as<ob::RealVectorStateSpace>()->getDimension();
                        if(k<=2)
                        {
                            ob::EuclideanProjection projection(k);
                            //space->getProjection("drawprojection")->project(pathstates[i], projection);
                            projToUse->project(pathstates[i], projection);
                            x=projection[0];
                            y=projection[1];
                            z=0.0;
                            edgepoints->point.set1Value(0,x,y,z);
                            //space->getProjection("drawprojection")->project(pathstates[i+1], projection);
                            projToUse->project(pathstates[i+1], projection);
                            x=projection[0];
                            y=projection[1];
                            edgepoints->point.set1Value(1,x,y,z);
                        }
                        else
                        {
                            ob::EuclideanProjection projection(k);
                            //space->getProjection("drawprojection")->project(pathstates[i], projection);
                            projToUse->project(pathstates[i], projection);
                            x=projection[0];
                            y=projection[1];
                            z=projection[2];
                            edgepoints->point.set1Value(0,x,y,z);
                            //space->getProjection("drawprojection")->project(pathstates[i+1], projection);
                            projToUse->project(pathstates[i+1], projection);
                            x=projection[0];
                            y=projection[1];
                            z=projection[2];
                            edgepoints->point.set1Value(1,x,y,z);
                        }
                    }

                    //edge of the path
                    pathsep->addChild(edgepoints);
                    SoLineSet *ls = new SoLineSet;
                    ls->numVertices.set1Value(0,2);//two values
                    SoDrawStyle *lstyle = new SoDrawStyle;
                    lstyle->lineWidth=6;//3;
                    SoMaterial *path_color = new SoMaterial;
                    path_color->diffuseColor.setValue(0.8,0.2,0.2);
                    pathsep->addChild(path_color);
                    pathsep->addChild(lstyle);
                    pathsep->addChild(ls);
                }
                _sceneCspace->addChild(pathsep);
            }




            //loop for all vertices of the roadmap or tree and create the coin3D points
            for(int i=0;i<pdata->numVertices();i++)
            {
                if(_wkSpace->getRobot(numrob)->isSE3Enabled())
                {
                    ob::EuclideanProjection projection(k);
                //&(projection) = new ob::EuclideanProjection;
                //try
                //{
                    //space->getProjection("drawprojection")->project(pdata->getVertex(i).getState(), projection);
                    projToUse->project(pdata->getVertex(i).getState(), projection);
                // }
                //catch(ompl::Exception e){
                  //  e.what();
                //}
                    x = projection[0];
                    y = projection[1];
                    z = projection[2];
                    points->point.set1Value(i,x,y,z);
                }
                else
                {
                    k = ssRobotifirst->as<ob::RealVectorStateSpace>()->getDimension();
                    if(k<=2)
                    {
                        ob::EuclideanProjection projection(k);
                        //space->getProjection("drawprojection")->project(pdata->getVertex(i).getState(), projection);
                        projToUse->project(pdata->getVertex(i).getState(), projection);
                        x = projection[0];
                        y = projection[1];
                        points->point.set1Value(i,x,y,0);
                    }
                    else
                    {
                        ob::EuclideanProjection projection(k);
                        //space->getProjection("drawprojection")->project(pdata->getVertex(i).getState(), projection);
                        projToUse->project(pdata->getVertex(i).getState(), projection);
                        x = projection[0];
                        y = projection[1];
                        z = projection[2];
                        points->point.set1Value(i,x,y,z);
                    }
                }
            }
            SoDrawStyle *pstyle = new SoDrawStyle;
            pstyle->pointSize = 3;
            SoMaterial *color = new SoMaterial;
            color->diffuseColor.setValue(0.2,0.8,0.2);

            //draw the points
            psep->addChild(color);
            psep->addChild(points);
            psep->addChild(pstyle);
            psep->addChild(pset);
            _sceneCspace->addChild(psep);

            //draw edges:
            SoSeparator *lsep = new SoSeparator();
            int numOutgoingEdges;
            std::vector< unsigned int > outgoingVertices;
            ob::Cost edgeweight;

            //loop for all nodes
            for(int i=0;i<pdata->numVertices();i++)
            {
                 numOutgoingEdges = pdata->getEdges (i, outgoingVertices);

                 //for each node loop for all the outgoing edges
                 for ( int j=0; j<numOutgoingEdges; j++ )
                 {
                    SoCoordinate3 *edgepoints  = new SoCoordinate3();

                    //initial edgepoint
                    float x1,y1,x2,y2,z1,z2;
                    if(_wkSpace->getRobot(numrob)->isSE3Enabled())
                    {
                        ob::EuclideanProjection projection(k);
                        //space->getProjection("drawprojection")->project(pdata->getVertex(i).getState(), projection);
                        projToUse->project(pdata->getVertex(i).getState(), projection);
                        x1=projection[0];
                        y1=projection[1];
                        z1=projection[2];
                        edgepoints->point.set1Value(0,x1,y1,z1);

                    //final edgepoint
                        //space->getProjection("drawprojection")->project(pdata->getVertex(outgoingVertices.at(j)).getState(), projection);
                        projToUse->project(pdata->getVertex(outgoingVertices.at(j)).getState(), projection);
                        x2=projection[0];
                        y2=projection[1];
                        z2=projection[2];
                        edgepoints->point.set1Value(1,x2,y2,z2);
                    }
                    else
                    {
                        k = ssRobotifirst->as<ob::RealVectorStateSpace>()->getDimension();
                        if(k<=2)
                        {
                            ob::EuclideanProjection projection(k);
                            //space->getProjection("drawprojection")->project(pdata->getVertex(i).getState(), projection);
                            projToUse->project(pdata->getVertex(i).getState(), projection);
                            x1=projection[0];
                            y1=projection[1];
                            z=0.0;
                            edgepoints->point.set1Value(0,x1,y1,z);
                            //space->getProjection("drawprojection")->project(pdata->getVertex(outgoingVertices.at(j)).getState(), projection);
                            projToUse->project(pdata->getVertex(outgoingVertices.at(j)).getState(), projection);
                            x2=projection[0];
                            y2=projection[1];
                            edgepoints->point.set1Value(1,x2,y2,z);
                        }
                        else
                        {
                            ob::EuclideanProjection projection(k);
                            //space->getProjection("drawprojection")->project(pdata->getVertex(i).getState(), projection);
                            projToUse->project(pdata->getVertex(i).getState(), projection);
                            x1=projection[0];
                            y1=projection[1];
                            z1=projection[2];
                            edgepoints->point.set1Value(0,x1,y1,z1);
                            //space->getProjection("drawprojection")->project(pdata->getVertex(outgoingVertices.at(j)).getState(), projection);
                            projToUse->project(pdata->getVertex(outgoingVertices.at(j)).getState(), projection);
                            x2=projection[0];
                            y2=projection[1];
                            z2=projection[2];
                            edgepoints->point.set1Value(1,x2,y2,z2);
                        }
                    }
                    //the edge
                    pdata->getEdgeWeight(i, outgoingVertices.at(j), &edgeweight);
                    SoMaterial *edge_color = new SoMaterial;
                    edge_color->diffuseColor.setValue(1.0,1.0,1.0);

                    //BE CAREFUL! a magic number!
                    /*
                    if(edgeweight.v>0.1) edge_color->diffuseColor.setValue(1.0,0.8,0.8);
                    else edge_color->diffuseColor.setValue(1.0,1.0,1.0);
                    */
                    lsep->addChild(edge_color);
                    lsep->addChild(edgepoints);
                    SoLineSet *ls = new SoLineSet;
                    ls->numVertices.set1Value(0,2);//two values
                    lsep->addChild(ls);
                 }
            }
           _sceneCspace->addChild(lsep);



            SoSeparator *floorsep = new SoSeparator();
            SoCube *cs = new SoCube();
            SoTransform *cub_transf = new SoTransform;
            SbVec3f centre;
            SoMaterial *cub_color = new SoMaterial;
            //draw floor
            if(_wkSpace->getRobot(numrob)->isSE3Enabled())
            {
                cs->width = xmax-xmin;
                cs->depth = (zmax-zmin);
                cs->height = ymax-ymin;

                centre.setValue(xmin+(xmax-xmin)/2,ymin+(ymax-ymin)/2,zmin+(zmax-zmin)/2);
                cub_transf->translation.setValue(centre);
                cub_transf->recenter(centre);

                //SoMaterial *cub_color = new SoMaterial;
                cub_color->diffuseColor.setValue(0.2,0.2,0.2);
                cub_color->transparency.setValue(0.5);

                floorsep->addChild(cub_color);
                floorsep->addChild(cub_transf);
                floorsep->addChild(cs);
                _sceneCspace->addChild(floorsep);
            }
            else
            {
                k = ssRobotifirst->as<ob::RealVectorStateSpace>()->getDimension();
                if(k<=2)
                {
                    cs->width = xmax-xmin;
                    cs->depth = (xmax-xmin)/50.0;
                    cs->height = ymax-ymin;
                    centre.setValue(xmin+(xmax-xmin)/2,ymin+(ymax-ymin)/2,-cs->depth.getValue());
                    cub_transf->translation.setValue(centre);
                    cub_transf->recenter(centre);
                    cub_color->diffuseColor.setValue(0.2,0.2,0.2);
                    //cub_color->transparency.setValue(0.98);
                    floorsep->addChild(cub_color);
                    floorsep->addChild(cub_transf);
                    floorsep->addChild(cs);
                    _sceneCspace->addChild(floorsep);
                }
                else
                {
                    cs->width = xmax-xmin;
                    cs->depth = (zmax-zmin);
                    cs->height = ymax-ymin;
                    centre.setValue(xmin+(xmax-xmin)/2,ymin+(ymax-ymin)/2,zmin+(zmax-zmin)/2);
                    cub_transf->translation.setValue(centre);
                    cub_transf->recenter(centre);
                    cub_color->diffuseColor.setValue(0.2,0.2,0.2);
                    cub_color->transparency.setValue(0.98);
                    floorsep->addChild(cub_color);
                    floorsep->addChild(cub_transf);
                    floorsep->addChild(cs);
                    _sceneCspace->addChild(floorsep);
                }
             }
    }

    //! This function converts a Kautham sample to an ompl scoped state.
    void omplPlanner::smp2omplScopedState(Sample* smp, ob::ScopedState<ob::CompoundStateSpace> *sstate)
    {
        //Extract the mapped configuration of the sample. It is a vector with as many components as robots.
        //each component has the RobConf of the robot (the SE3 and the Rn configurations)
        if(smp->getMappedConf().size()==0)
        {
            _wkSpace->moveRobotsTo(smp); // to set the mapped configuration
        }
        std::vector<RobConf>& smpRobotsConf = smp->getMappedConf();


        //loop for all the robots
        for(int i=0; i<_wkSpace->getNumRobots(); i++)
        {
            int k=0; //counter of subspaces contained in subspace of robot i

            //get the subspace of robot i
            ob::StateSpacePtr ssRoboti = ((ob::StateSpacePtr) space->as<ob::CompoundStateSpace>()->getSubspace(i));
            string ssRobotiname = ssRoboti->getName();

            //if it has se3 part
            if(_wkSpace->getRobot(i)->isSE3Enabled())
            {
                //get the kautham SE3 configuration
                SE3Conf c = smpRobotsConf.at(i).getSE3();
                vector<KthReal>& pp = c.getPos();
                vector<KthReal>& aa = c.getAxisAngle();

                //set the ompl SE3 configuration
                ob::StateSpacePtr ssRobotiSE3 =  ((ob::StateSpacePtr) ssRoboti->as<ob::CompoundStateSpace>()->getSubspace(k));
                string ssRobotiSE3name = ssRobotiSE3->getName();

                ob::ScopedState<ob::SE3StateSpace> cstart(ssRobotiSE3);
                cstart->setX(pp[0]);
                cstart->setY(pp[1]);
                cstart->setZ(pp[2]);
                cstart->rotation().setAxisAngle(aa[0],aa[1],aa[2],aa[3]);

                //load the global scoped state with the info of the se3 data of robot i
                (*sstate)<<cstart;
                k++;
            }

            //has Rn part
            if(_wkSpace->getRobot(i)->getNumJoints()>0)
            {
                //get the kautham Rn configuration
                RnConf r = smpRobotsConf.at(i).getRn();

                //set the ompl Rn configuration
                ob::StateSpacePtr ssRobotiRn =  ((ob::StateSpacePtr) ssRoboti->as<ob::CompoundStateSpace>()->getSubspace(k));
                ob::ScopedState<weigthedRealVectorStateSpace> rstart(ssRobotiRn);

                for(int j=0; j<_wkSpace->getRobot(i)->getNumJoints();j++)
                    rstart->values[j] = r.getCoordinate(j);

                //cout<<"sstate[0]="<<rstart->values[0]<<"sstate[1]="<<rstart->values[1]<<endl;


                //load the global scoped state with the info of the Rn data of robot i
                (*sstate) << rstart;
                k++;//dummy
            }
        }
    }

    //! This member function converts an ompl State to a Kautham sample
    void omplPlanner::omplState2smp(const ob::State *state, Sample* smp)
    {
        ob::ScopedState<ob::CompoundStateSpace> sstate(space);
        sstate = *state;
        omplScopedState2smp( sstate, smp);
    }

    //! This member function converts an ompl ScopedState to a Kautham sample
    void omplPlanner::omplScopedState2smp(ob::ScopedState<ob::CompoundStateSpace> sstate, Sample* smp)
    {
        int k=0;
        vector<RobConf> rc;

        //loop for all the robots
        for(int i=0; i<_wkSpace->getNumRobots(); i++)
        {
            //RobConf to store the robots configurations read form the ompl state
            RobConf *rcj = new RobConf;

            //Get the subspace corresponding to robot i
            ob::StateSpacePtr ssRoboti = ((ob::StateSpacePtr) space->as<ob::CompoundStateSpace>()->getSubspace(i));

            //Get the SE3 subspace of robot i, if it exisits, and extract the SE3 configuration
            int k=0; //counter of subspaces of robot i
            if(_wkSpace->getRobot(i)->isSE3Enabled())
            {
                //Get the SE3 subspace of robot i
                 ob::StateSpacePtr ssRobotiSE3 =  ((ob::StateSpacePtr) ssRoboti->as<ob::CompoundStateSpace>()->getSubspace(k));

                 //create a SE3 scoped state and load it with the data extracted from the global scoped state
                 ob::ScopedState<ob::SE3StateSpace> pathscopedstatese3(ssRobotiSE3);
                 sstate >> pathscopedstatese3;

                 //convert to a vector of 7 components
                 vector<KthReal> se3coords;
                 se3coords.resize(7);
                 se3coords[0] = pathscopedstatese3->getX();
                 se3coords[1] = pathscopedstatese3->getY();
                 se3coords[2] = pathscopedstatese3->getZ();
                 se3coords[3] = pathscopedstatese3->rotation().x;
                 se3coords[4] = pathscopedstatese3->rotation().y;
                 se3coords[5] = pathscopedstatese3->rotation().z;
                 se3coords[6] = pathscopedstatese3->rotation().w;
                 //create the sample
                 SE3Conf se3;
                 se3.setCoordinates(se3coords);
                 rcj->setSE3(se3);
                 k++;
             }
             //If the robot does not have movile SE3 dofs then the SE3 configuration of the sample is maintained
             else
             {
                 if(smp->getMappedConf().size()==0)
                     throw ompl::Exception("omplPlanner::omplScopedState2smp", "parameter smp must be a sample with the MappedConf");
                 else
                     rcj->setSE3(smp->getMappedConf()[i].getSE3());
             }


            //Get the Rn subspace of robot i, if it exisits, and extract the Rn configuration
             if(_wkSpace->getRobot(i)->getNumJoints()>0)
             {
                 //Get the Rn subspace of robot i
                 ob::StateSpacePtr ssRobotiRn =  ((ob::StateSpacePtr) ssRoboti->as<ob::CompoundStateSpace>()->getSubspace(k));

                 //create a Rn scoped state and load it with the data extracted from the global scoped state
                 ob::ScopedState<weigthedRealVectorStateSpace> pathscopedstateRn(ssRobotiRn);
                 sstate >> pathscopedstateRn;

                 //convert to a vector of n components
                 vector<KthReal> coords;
                 for(int j=0;j<_wkSpace->getRobot(i)->getNumJoints();j++) coords.push_back(pathscopedstateRn->values[j]);
                 rcj->setRn(coords);
                 k++;//dummy
             }
             //If the robot does not have movile Rn dofs then the Rn configuration of the sample is maintained
             else
             {
                 if(smp->getMappedConf().size()==0)
                     throw ompl::Exception("omplPlanner::omplScopedState2smp", "parameter smp must be a sample with the MappedConf");
                 else
                     rcj->setRn(smp->getMappedConf()[i].getRn());
             }
             //load the RobConf with the data of robot i
             rc.push_back(*rcj);
        }
        //create the sample with the RobConf
        //the coords (controls) of the sample are kept void
        smp->setMappedConf(rc);
    }


	//! function to find a solution path
    bool omplPlanner::trySolve()
    {
        //Start state: convert from smp to scoped state
        ob::ScopedState<ob::CompoundStateSpace> startompl(space);
        smp2omplScopedState(_init, &startompl);
        cout<<"startompl:"<<endl;
        startompl.print();

        //Goal state: convert from smp to scoped state
         ob::ScopedState<ob::CompoundStateSpace> goalompl(space);
         smp2omplScopedState(_goal, &goalompl);
         cout<<"goalompl:"<<endl;
         goalompl.print();

         // set the start and goal states
         ss->setStartAndGoalStates(startompl, goalompl);

         //remove previous solutions, if any
         if(_incremental == 0)
         {
             ss->clear();       
             ss->getPlanner()->clear();
         }
         else
             ss->getProblemDefinition()->clearSolutionPaths();

         // attempt to solve the problem within _planningTime seconds of planning time
         ss->setup();
         ob::PlannerStatus solved = ss->solve(_planningTime);

         //ss->print();

         //the following line is added to restore the problem definition and its optimization objective that was lost after solve
         //needed in drawcspace that wants to access the weights of the edges that have to be recomputed with the optimization fro rrtstar.
         this->setParameters();


         //retrieve all the states. Load the SampleSet _samples
         Sample *smp;
         ob::PlannerData data(ss->getSpaceInformation());
         ss->getPlannerData(data);
         /*
         for(int i=0; i<data.numVertices();i++)
         {
                smp=new Sample(_wkSpace->getNumRobControls());
                smp->setMappedConf(_init->getMappedConf());//copy the conf of the start smp
                omplState2smp(data.getVertex(i).getState(), smp);
                _samples->add(smp);
         }
         */

         if (solved)
         {
                std::cout << "Found solution:" << std::endl;

                // print the path to screen
                if(_simplify==1) {//smooth
                    //ss->getPathSimplifier()->shortcutPath(ss->getSolutionPath(),0,0,0.15);
                    ss->getPathSimplifier()->smoothBSpline(ss->getSolutionPath(),5);
                }
                else if(_simplify==2) {//shorten and smoot
                    ss->simplifySolution();
                }
                std::cout<<"Path: ";
                ss->getSolutionPath().print(std::cout);

                ss->getSolutionPath().interpolate();
                //std::cout<<"Path after interpolation: ";
                //ss->getSolutionPath().interpolate(10);
                //ss->getSolutionPath().print(std::cout);


                //refine
                //ss->getSolutionPath().interpolate();
                std::vector< ob::State * > & pathstates = ss->getSolutionPath().getStates();

                Sample *smp;

                _path.clear();
                clearSimulationPath();



               //load the kautham _path variable from the ompl solution
                for(int j=0;j<ss->getSolutionPath().getStateCount();j++){
                   //create a smp and load the RobConf of the init configuration (to have the same if the state does not changi it)
                    smp=new Sample(_wkSpace->getNumRobControls());
                    smp->setMappedConf(_init->getMappedConf());
                    //convert form state to smp
                    omplState2smp(ss->getSolutionPath().getState(j)->as<ob::CompoundStateSpace::StateType>(), smp);

                    _path.push_back(smp);
                    _samples->add(smp);
                }
                _solved = true;
                drawCspace(_drawnrobot);
                return _solved;
            }
            //solution not found
            else{
                std::cout << "No solution found" << std::endl;
                _solved = false;
                drawCspace(_drawnrobot);
                return _solved;
            }
		}
    }
}


#endif // KAUTHAM_USE_OMPL

