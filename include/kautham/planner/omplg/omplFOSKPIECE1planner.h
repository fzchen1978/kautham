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

/* Author: Nestor Garcia Hidalgo */

#if !defined(_omplFOSKPIECE1PLANNER_H)
#define _omplFOSKPIECE1PLANNER_H

#if defined(KAUTHAM_USE_OMPL)

#include "omplplanner.h"
#include <kautham/planner/omplg/synergy_tree.h>

namespace Kautham {
/** \addtogroup Planner
 *  @{
 */
  namespace omplplanner {
    class omplFOSKPIECE1Planner : public omplPlanner {
        public:
        omplFOSKPIECE1Planner(SPACETYPE stype, Sample *init, Sample *goal,
                              SampleSet *samples, WorkSpace *ws, og::SimpleSetup *ssptr,
                              const std::string &synergyTreeFilename);

        ~omplFOSKPIECE1Planner();

        virtual bool trySolve();//!< Overloaded trySolve function to include evaluation of final path cost

        bool setParameters();

    protected:
        SynergyTree *st_;

        class Projection : public ompl::base::ProjectionEvaluator {
        public:
            Projection(const ompl::base::StateSpacePtr &space,
                       Kautham::omplplanner::omplPlanner *pl,
                       SynergyTree *st);

            virtual unsigned int getDimension() const;

            virtual void defaultCellSizes();

            virtual void project(const ompl::base::State *state,
                                 ompl::base::EuclideanProjection &projection) const;
        protected:
            void omplState2armaVec(const ompl::base::State *s, arma::vec &q) const;

            Kautham::omplplanner::omplPlanner *pl_;

            std::map<unsigned int,std::pair<std::set<unsigned  int>,std::set<unsigned  int> > > robotJoint;

            unsigned int vfdim_;

            arma::mat proj;

            arma::vec offset;

            arma::vec low;

            arma::vec high;

            arma::vec range;
        };
      };
  }
  /** @}   end of Doxygen module "Planner */
}

#endif // KAUTHAM_USE_OMPL
#endif  //_omplFOSKPIECE1PLANNER_H