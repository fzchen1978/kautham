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
#if !defined(_omplPLANNER_H)
#define _omplPLANNER_H

#if defined(KAUTHAM_USE_OMPL)
#include <ompl/base/SpaceInformation.h>
#include <ompl/geometric/SimpleSetup.h>
#include <ompl/control/SimpleSetup.h>
#include <ompl/config.h>
#include <ompl/base/spaces/RealVectorStateSpace.h>
#include <ompl/base/spaces/SE3StateSpace.h>
#include <ompl/base/spaces/SO3StateSpace.h>
#include <ompl/base/samplers/UniformValidStateSampler.h>
#include <ompl/base/ProjectionEvaluator.h>
#include <ompl/base/spaces/RealVectorStateProjections.h>
#include <ompl/util/Exception.h>
namespace og = ompl::geometric;
namespace oc = ompl::control;
namespace ob = ompl::base;


#include <libproblem/workspace.h>
#include <libsampling/sampling.h>
#include "planner.h"


using namespace std;
using namespace libSampling;

namespace libPlanner {
  namespace omplplanner{


  /////////////////////////////////////////////////////////////////////////////////////////////////
  //AUXILIAR Functions
    ob::StateSamplerPtr allocStateSampler(const ob::StateSpace *mysspace, Planner *p);
    ob::ValidStateSamplerPtr allocValidStateSampler(const ob::SpaceInformation *si, Planner *p);
    bool isStateValid(const ob::SpaceInformation *si, const ob::State *state, Planner *p);

    /////////////////////////////////////////////////////////////////////////////////////////////////
    // Class weigthedRealVectorStateSpace
    /////////////////////////////////////////////////////////////////////////////////////////////////
    //! This class represents a RealVectorStateSpace with a weighted distance. It is used to describe the state space of
    //! the kinematic chains where different weights are set to the joints.
    class weigthedRealVectorStateSpace:public ob::RealVectorStateSpace
    {
      public:
        weigthedRealVectorStateSpace(unsigned int dim=0);
        ~weigthedRealVectorStateSpace(void);
        void setWeights(vector<KthReal> w);
        double distance(const ob::State *state1, const ob::State *state2) const;
      protected:
        vector<KthReal> weights;
    };

    /////////////////////////////////////////////////////////////////////////////////////////////////
    // Class KauthamValidStateSampler
    /////////////////////////////////////////////////////////////////////////////////////////////////
    //! This class represents a valid state sampler based on the sampling of the Kautham control space and
    //! its conversion to samples. Does a collision-check to verify validity.
    class KauthamValidStateSampler : public ob::ValidStateSampler
    {
      public:
        KauthamValidStateSampler(const ob::SpaceInformation *si, Planner *p);
        virtual bool sample(ob::State *state);
        virtual bool sampleNear(ob::State *state, const ob::State *near, const double distance);

      protected:
        ompl::RNG rng_; //random generator
        Planner *kauthamPlanner_; //pointer to planner in order to have access to the workspace
        const ob::SpaceInformation *si_;
    };


    /////////////////////////////////////////////////////////////////////////////////////////////////
    // Class KauthamStateSampler
    /////////////////////////////////////////////////////////////////////////////////////////////////
    //! This class represents a  state sampler based on the sampling of the Kautham control space and
    //! its conversion to samples.
    class KauthamStateSampler : public ob::CompoundStateSampler
    {
      public:
        KauthamStateSampler(const ob::StateSpace *sspace, Planner *p);
        void setCenterSample(ob::State *state, double th);
        virtual void sampleUniform(ob::State *state);
        virtual void sampleUniformNear(ob::State *state, const ob::State *near, const double distance);


      protected:
        ompl::RNG rng_; //random generator
        Planner *kauthamPlanner_; //pointer to planner in order to have access to the workspace
        Sample *centersmp;
        double threshold;
    };



    ////////////////////////////////////////////////////////////////////////////////////////
    //Class omplPlanner
    ////////////////////////////////////////////////////////////////////////////////////////
    class omplPlanner:public Planner {
	    public:
        //Add public data and functions
        omplPlanner(SPACETYPE stype, Sample *init, Sample *goal, SampleSet *samples, Sampler *sampler,
          WorkSpace *ws, LocalPlanner *lcPlan, KthReal ssize);
        ~omplPlanner();
        
        bool trySolve();//reimplemented
        bool setParameters();//reimplemented
        SoSeparator *getIvCspaceScene();//reimplemented
        void drawCspace();
        //void drawCspaceSE3();
        //void drawCspaceRn();
        //void drawCspaceprojections();

        void omplState2smp(const ob::State *state, Sample* smp);
        void smp2omplScopedState(Sample* smp, ob::ScopedState<ob::CompoundStateSpace> *sstate);
        void omplScopedState2smp(ob::ScopedState<ob::CompoundStateSpace> sstate, Sample* smp);
        inline ob::StateSpacePtr getSpace(){return space;};
        void filterBounds(double &l, double &h, double epsilon);

		protected:
		//Add protected data and functions
        KthReal _planningTime;
        og::SimpleSetupPtr ss;
        ob::StateSpacePtr space;

	    private:
		//Add private data and functions
	  };
  }
}

#endif // KAUTHAM_USE_OMPL
#endif  //_omplPLANNER_H

