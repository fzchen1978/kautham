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
*     Copyright (C) 2007 - 2011 by Alexander P�rez and Jan Rosell          *
*            alexander.perez@upc.edu and jan.rosell@upc.edu                *
*                                                                          *
*             This is a motion planning tool to be used into               *
*             academic environment and it's provided without               *
*                     any warranty by the authors.                         *
*                                                                          *
*          Alexander P�rez is also with the Escuela Colombiana             *
*          de Ingenier�a "Julio Garavito" placed in Bogot� D.C.            *
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
 
 
#include "pathtoguide.h"
#define DIMMETRIC 3

namespace libGuiding{

  PathToGuide::PathToGuide( Robot &rob ):_rob(rob){
    _nearestQ = NULL;
    _nearestX = NULL;
    reset();
  }  

  PathToGuide::PathToGuide( Robot* rob ):_rob(*rob){
    _nearestQ = NULL;
    _nearestX = NULL;
    reset();
  }

  bool PathToGuide::reset(){
    try{
      unsigned int i = 0;
      _pathQ.clear();
      _pathX.clear();
      if( _nearestQ != NULL) delete _nearestQ;
      if( _nearestX != NULL) delete _nearestX;

      vector<RobConf>::iterator it;
      mt::Transform trans;
      Xnode anXnode( 7 );
      Qnode anQnode( _rob.getNumJoints() );

      i = 0;
      for(it = _rob.getProposedSolution().begin(); it != _rob.getProposedSolution().end(); ++it){
        // For each item, the algorithm extracts the Q configuration and stores it in the 
        // _pathQ and then it will be converted to the TCP cartesian position with the 
        // inverse kinematic and it will be stored in the _pathX.

        std::copy(it->second.getCoordinates().begin(), it->second.getCoordinates().end(), anQnode.begin() );

        _rob.Kinematics(*it);
        trans = _rob.getLastLinkTransform();
        anXnode = tran2xnode(trans);

        _pathQ.push_back( anQnode );
        _pathX.push_back( anXnode );

        if( _rob.getIkine() != NULL ){
          // Now, the vector of layouts is filled using the Inverse Kinematic Model.
          RobLayout& rbLy = _rob.getIkine()->getRobLayout( it->second.getCoordinates() );
          
          if( !_layouts.empty() )
            if( rbLy != _layouts.at(_layouts.size() -1 ) )
              _singularities.push_back( _layouts.size() );

          _layouts.push_back( rbLy );
        }
      }  

      if( _pathQ.empty() ) return false; // If the robot does not have a path to be guided.

      _uvecQ.clear();
      _uvecX.clear();

      // Create a vector of unit vectors from one node to the next one in Q.
      Uvec avec(anQnode.size());
      float norm = 0.;
      vector<Qnode>::iterator ite,itb;
      itb = _pathQ.begin();
      for(ite = _pathQ.begin() + 1; ite != _pathQ.end(); ++ite,++itb){
        norm = 0.;
        for(int i = 0; i < avec.size(); i++){
          avec.at(i) = (*ite).at(i) - (*itb).at(i);
          norm += avec.at(i) * avec.at(i);
        }
        norm = sqrt(norm);
        avec.dist(norm);
        for(int i =0; i < avec.size(); i++)
          avec.at(i) /= norm;

        _uvecQ.push_back(avec);
      }
      avec.clear();
      avec.resize( anQnode.size(), 0. );
      avec.dist(0.);
      _uvecQ.push_back(avec);

      // Now for the SE3 nodes.
      avec.resize(7, 0.);
      itb = _pathX.begin();
      for(ite = _pathX.begin() + 1; ite != _pathX.end(); ++ite,++itb){
        norm = 0.;
        for(int i = 0; i < avec.size(); i++){
          avec.at(i) = (*ite).at(i) - (*itb).at(i);
          norm += avec.at(i) * avec.at(i);
        }
        norm = sqrt(norm);
        avec.dist(norm);
        for(int i =0; i < avec.size(); i++)
          avec.at(i) /= norm;

        _uvecX.push_back(avec);
      }
      avec.clear();
      avec.resize( 7, 0. );
      avec.dist(0.);
      _uvecX.push_back(avec);

      if( _pathQ.size() == _pathX.size() && _pathQ.size() != 0 ){
        recalculateANN();
        return true;
      }

    }catch(...){}
    return false;
  }

  KthReal PathToGuide::unitVectors(Qnode &qi, Qnode &qd, Uvec &um, Uvec &up ){
    if( _pathQ.empty() || _uvecQ.empty() ) return -1.;
    try{
      int idx = nearQ(qi);
      const Qnode& nea = _pathQ.at(idx);
      Uvec avec(qi.size());
      for(int i = 0; i < nea.size(); i++)
        avec.at(i) = qi.at(i) - nea.at(i);
      
      KthReal proj = 0.;
      for(int i = 0; i < nea.size(); i++)
        proj += avec.at(i) * _uvecQ.at(idx).at(i);

      if( proj > 0 ){
        //qd.clear();
        //qd.resize( _pathQ.at(0).resize() );
        for(int i = 0; i < nea.size(); i++){
          qd.at(i) = nea.at(i) + proj * _uvecQ.at(idx).at(i);
          up.at(i) = _uvecQ.at(idx).at(i);
        }
      }else{// calculates the projection over the segment before.
        proj = 0.;
        for(int i = 0; i < nea.size(); i++)
          proj += avec.at(i) * _uvecQ.at(idx-1).at(i);
        
        //qd.clear();
        //qd.resize( _pathQ.at(0).resize() );
        for(int i = 0; i < nea.size(); i++){
          qd.at(i) = nea.at(i) - proj * _uvecQ.at(idx-1).at(i);
          up.at(i) = _uvecQ.at(idx-1).at(i);
        }
      }

      proj = 0.;
      for(int i = 0; i < nea.size(); i++){
        um.at(i) = qd.at(i) - qi.at(i);
        proj += um.at(i) * um.at(i);
      }

      proj = sqrt(proj);
      for(int i = 0; i < nea.size(); i++)
        um.at(i) /= proj;

      return proj;
    }catch(...){}

    return -1.;
  }

  KthReal PathToGuide::unitVectors(Xnode &xi, Xnode &xd, int &k, KthReal &ratio, Uvec &um, Uvec &up, bool& projD, 
                  const KthReal EPSILON ) {
    if( _pathX.empty() || _uvecX.empty() ) return -1.;
    try{
      k = nearX(xi);
      const Xnode& nea = _pathX.at(k);
      Uvec avec(7);
      KthReal normV = 0.;
      for(int i = 0; i < DIMMETRIC; i++){
        avec.at(i) = xi.at(i) - nea.at(i);
        normV+=avec.at(i) * avec.at(i);
      }
      normV=sqrt(normV);
      
      // Computes the projection over the segment x(nea) -> x(nea+1)
      KthReal proj = 0.;
      for(int i = 0; i < DIMMETRIC; i++)
        proj += avec.at(i) * _uvecX.at(k).at(i);

      // First the goal and init points were chosen because their behaviour is fixed
      if( (k == _pathX.size() -1) || (k == 0 && proj <= 0) ){// The Goal and init points
        projD = true;
        for(int i = 0; i < DIMMETRIC; i++){
          xd.at(i) = nea.at(i);
          up.at(i) = 0.;
        }
        ratio = 1.;
      }else{ // This part processes the rest of the path
        // For each other point, the force is generated to the point inside a segment if 
        // its distance to the vertex is greater than Epsilon
        if( normV <= EPSILON && proj <= 0){   // In this case the force is generated using the unit vector of the 
          projD = true;                       // segment nea to nea+1 and attracting to nearest point.
          for(int i = 0; i < DIMMETRIC; i++){
            xd.at(i) = nea.at(i);
            up.at(i) = _uvecX.at(k).at(i);
          }
          ratio = 1.;                   
        }else{ // normV > EPSILON || (normV <= EPSILON && proj > 0)
          if( proj < 0 ){ // calculates the projection over the segment before.
            proj = 0.;
            for(int i = 0; i < DIMMETRIC; i++){
              avec.at(i) = xi.at(i) - _pathX.at(k-1).at(i);
              normV+=avec.at(i) * avec.at(i);
            }
            normV=sqrt(normV);
            for(int i = 0; i < DIMMETRIC; i++)
              proj += avec.at(i) * _uvecX.at(k-1).at(i);
            
            for(int i = 0; i < DIMMETRIC; i++){
              xd.at(i) = _pathX.at(k-1).at(i) + proj * _uvecX.at(k-1).at(i);
              up.at(i) = _uvecX.at(k-1).at(i);
            }
            ratio = 1. - proj / _uvecX.at(k-1).dist() ;
          }else{ //proj >= 0 % // The rest of the path then it calculates the projection over the segment
            projD = true;
            for(int i = 0; i < DIMMETRIC; i++){
              xd.at(i) = nea.at(i) + proj * _uvecX.at(k).at(i);
              up.at(i) = _uvecX.at(k).at(i);
            }
            ratio = proj/_uvecX.at(k).dist();
          }//end if( proj < 0 )
        }// end if( normV <= EPSILON && proj <= 0)
      }// end if( (k == _pathX.size() -1) || (k == 0 && proj <= 0) )

      proj = 0.;
      for(int i = 0; i < DIMMETRIC; i++){
        um.at(i) = xd.at(i) - xi.at(i);   
        proj += um.at(i) * um.at(i);
      }
      proj = sqrt(proj);

      for(int i = 0; i < DIMMETRIC; i++)
        um.at(i) /= proj;

      return proj;
    }catch(...){}

    return -1.;  
}


 int PathToGuide::nearQ(const Qnode &qn ){
		double d_ann = 0.;
		int idx_ann = 0;
		//ANNpoint *result_pt = new ANNpoint;
    ANNpoint pts = annAllocPt( qn.size() );

    std::copy(qn.begin(), qn.end(), pts );
    
    // compute nearest neighbor using library
		_nearestQ->NearestNeighbor(pts, idx_ann, d_ann );// (void**&)result_pt);	

    annDeallocPt( pts );

    return idx_ann;
  }

  bool PathToGuide::nearQ(const Qnode &qn, int* indexs ){
    try{
      indexs[0] = indexs[1] = -1;

      double *d_ann = new double[2];
      ANNpoint *result_pt = annAllocPts(2, qn.size());

      ANNpoint pts = annAllocPt( qn.size() );
      std::copy(qn.begin(), qn.end(), pts );
  		
		  d_ann[0] = d_ann[1] = INFINITY;

      // compute nearest neighbor using library
      _nearestQ->NearestNeighbor(pts, d_ann, indexs, (void**&)result_pt);	

      annDeallocPt( pts );
      annDeallocPts( result_pt );

      return true;
    }catch(...){
      return false;
    }
  }

  void PathToGuide::nearQ(const Qnode &qn, Qnode& res ){
    int idx = nearQ( qn );
    res = _pathQ.at( idx );
  }

  bool PathToGuide::nearQ(const Qnode &qn, Qnode res[] ){
    try{
      int idx[]={-1, -1};
      res[0].clear();
      res[1].clear();

      nearQ(qn, idx);
      res[0] = _pathQ.at(idx[0]);
      res[1] = _pathQ.at(idx[1]);
      return true;
    }catch(...){
      return false;
    }
  }

  int PathToGuide::nearX(const Xnode &xn ){
    double d_ann = 0.;
		int idx_ann = 0;
		
    ANNpoint pts = annAllocPt( DIMMETRIC );

    for(size_t i= 0; i < DIMMETRIC; i++)
      pts[i] = xn.at(i);
    
    // compute nearest neighbor using library
		_nearestX->NearestNeighbor(pts, idx_ann, d_ann );// (void**&)result_pt);	

    annDeallocPt( pts );
    return idx_ann;
  }

  bool PathToGuide::nearX(const Xnode &xn, int* indexs ){
    try{
      indexs[0] = indexs[1] = -1;

      double *d_ann = new double[2];
      ANNpoint *result_pt = annAllocPts( 2, DIMMETRIC );

      ANNpoint pts = annAllocPt( DIMMETRIC );

      for(size_t i= 0; i < DIMMETRIC; i++)
      pts[i] = xn.at(i);
  		
		  d_ann[0] = d_ann[1] = INFINITY;

      // compute nearest neighbor using library
      _nearestX->NearestNeighbor(pts, d_ann, indexs, (void**&)result_pt);

      annDeallocPt( pts );
      annDeallocPts( result_pt );
      return true;
    }catch(...){
      return false;
    }
  }

  void PathToGuide::nearX(const Xnode &xn, Xnode& res ){
    int idx = nearX( xn );
    res = _pathX.at( idx );
  }

  bool PathToGuide::nearX(const Xnode &xn, Xnode res[] ){
    try{
      int idx[]={-1, -1};
      res[0].clear();
      res[1].clear();

      nearX(xn, idx);
      res[0] = _pathX.at(idx[0]);
      res[1] = _pathX.at(idx[1]);
      return true;
    }catch(...){
      return false;
    }
  }
  
  void PathToGuide::recalculateANN(){
    if( _nearestQ != NULL ){
      delete _nearestQ;
      annDeallocPts(_ptsQ);
    }

    if( _nearestX != NULL ) {
      delete _nearestX;
      annDeallocPts(_ptsX);
    }

    // First setup the Q structure.
    int dim = _pathQ.at(0).size();
		int *topology = new int[dim];
		double *scale = new double[dim];

		for(unsigned int k = 0; k < dim ; k++){
			topology[k] = 1; //Rn 
			scale[k] = _rob.getLink(k+1)->getWeight();  
		}
		

    _nearestQ = new MultiANN(dim, _pathQ.size(), topology, scale);	// create a data structure	
		_ptsQ = annAllocPts(_pathQ.size(), dim);		// allocate data points

    // Copying data
    for(unsigned int i = 0; i < _pathQ.size();++i){
      for(unsigned int k = 0; k < _rob.getNumJoints(); k++)
		    _ptsQ[i][k] = _pathQ.at(i).at(k);

      _nearestQ->AddPoint( _ptsQ[i], _ptsQ[i] );
    }

    //XXXXXXXXXXXXXXXXX

    

    delete topology;
    delete scale;

    topology = new int[DIMMETRIC];
		scale = new double[DIMMETRIC];
		int c=0;
    double rho_t = 1.0;
    double rho_r = 1.0;  

		topology[c] = 1; //x
		scale[c++] = rho_t;  
		topology[c] = 1; //y
		scale[c++] =  rho_t;  
		topology[c] = 1; //z
		scale[c++] =  rho_t;  
    
    if(DIMMETRIC==7){
		  topology[c] = 3; //quaternion
		  scale[c++] = rho_r;  
		  topology[c] = 3; //quaternion
		  scale[c++] = rho_r;  
		  topology[c] = 3; //quaternion
		  scale[c++] = rho_r;  
		  topology[c] = 3; //quaternion
		  scale[c++] = rho_r;  
    }

    _nearestX = new MultiANN(DIMMETRIC, _pathX.size(), topology, scale);	// create a data structure	
		_ptsX = annAllocPts(_pathX.size(), DIMMETRIC);		// allocate data points

    //XXXXXXXXXXX  Copying data  XXXXXXX
    for(unsigned int i = 0; i < _pathX.size();++i){
      for(unsigned int k = 0; k < DIMMETRIC; k++)
		    _ptsX[i][k] = _pathX.at(i).at(k);

      _nearestX->AddPoint( _ptsX[i], _ptsX[i] );
    }

    //XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
  }

 
}
