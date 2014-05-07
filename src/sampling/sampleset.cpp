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


#include <stdio.h>
#include "sampleset.h"
#include "sample.h"
#include "sdksample.h"
#include <external/libann/DNN/multiann.h>
#include <cmath>
//#include <crtdbg.h>



namespace Kautham{


  SampleSet::SampleSet(){
    samples.clear();
	  typesearch = ANNMETHOD; //BRUTEFORCE
	  setANN = false;
	  ws = NULL;
    maxNeighs = 1;
    MAG = NULL;
    data_pts = NULL;	
  }

   void SampleSet::resetANNdatastructures(){
		//delete existing data
    if( MAG != NULL ) delete MAG;
		if(data_pts != NULL) annDeallocPts(data_pts);
   }

  void SampleSet::setANNdatastructures(int maxn, int maxs)
  {
		if(maxn!=0) maxNeighs = maxn;
		if(maxs!=0) maxSamples = maxs;
		if(maxNeighs>=maxSamples)
			maxNeighs=maxSamples-1;  //otherwise ANN search crashes

		if(setANN==true)
			resetANNdatastructures();

		if(ws==NULL){
			cout<<"error: setWorkspace() should be called first"<<endl;
			//exit(0);
			setANN = false;
			return;
		}

		int dim=0;
        for(unsigned int i=0; i< ws->getNumRobots(); i++)
		{
			dim += 7;
			dim += ws->getRobot(i)->getNumJoints();
		}

		int *topology = new int[dim];
		double *scale = new double[dim];

		int c=0;
		double rho_t = 1.0;
		double rho_r = 1.0;
        for(unsigned int i=0; i< ws->getNumRobots(); i++)
		{
			rho_t = ws->getRobot(i)->getWeightSE3()[0]; 
			rho_r = ws->getRobot(i)->getWeightSE3()[1]; 

			topology[c] = 1; //x
			scale[c++] = rho_t;  
			topology[c] = 1; //y
			scale[c++] =  rho_t;  
			topology[c] = 1; //z
			scale[c++] =  rho_t;  


 			topology[c] = 3; //quaternion
			scale[c++] = rho_r;  
			topology[c] = 3; //quaternion
			scale[c++] = rho_r;  
			topology[c] = 3; //quaternion
			scale[c++] = rho_r;  
			topology[c] = 3; //quaternion
			scale[c++] = rho_r;  
			for(unsigned int k=0;k<ws->getRobot(i)->getNumJoints(); k++) 
			{
				//TODO:
				//set topology to 2 instead of 1 for the revolute joints without limits
				topology[c] = 1; //Rn 
				scale[c++] = ws->getRobot(i)->getLink(k+1)->getWeight();  
			}
		}

		MAG = new MultiANN(dim, maxNeighs, topology, scale);	// create a data structure	
		data_pts = annAllocPts(maxSamples, dim);		// allocate data points

		setANN = true;
  }

  SampleSet::~SampleSet(){
    clear();
  }

  bool SampleSet::existSample(unsigned long code){
    try{
	    long int pos = findSDKOrder(code);
	    if(samples.size() == 0 || samples.size() == pos)
		    return false;
	    else
		    return (((SDKSample*) samples[pos])->getCode() == code);
    }catch(...){
      return false;
    }
  }

  bool SampleSet::add(Sample* samp, bool orderIfCode){
    bool ok=false;
    unsigned long code=0;
    hasChanged = true;

    // Updating the ANN structure
    //data_pts array also created in cspace constructor		
    if( setANN ){
		  vector<KthReal> tmpVec;
		  RobConf* confSmp;
		  vector<KthReal> *p;

		  unsigned int h = indexOf(samp);
		  unsigned int c=0;

          tmpVec.clear();
          for(unsigned int j=0; j < ws->getNumRobControls(); j++) {
              tmpVec.push_back(samp->getCoords()[j]);
          }

          for(unsigned int i=0; i< ws->getNumRobots(); i++)
		  {
              //compute conf of robot i corresponding to the sample samples[k]
			  ws->getRobot(i)->control2Pose(tmpVec);
			  confSmp = ws->getRobot(i)->getCurrentPos();
			  p = &(confSmp->getSE3().getCoordinates());
  				
			  data_pts[h][c++] = p->at(0);
			  data_pts[h][c++] = p->at(1);
			  data_pts[h][c++] = p->at(2);
			  data_pts[h][c++] = p->at(3); //qx
			  data_pts[h][c++] = p->at(4); //qy
			  data_pts[h][c++] = p->at(5); //qz
			  data_pts[h][c++] = p->at(6); //qw
  			
			  for(int m=0; m < confSmp->getRn().getDim(); m++)
			  {
				data_pts[h][c++] = confSmp->getRn().getCoordinates()[m];
			  }
		  }
    }

    if( orderIfCode == true ){
      try{ //if the sample has code is a SDKSample
        unsigned long pos = findSDKOrder(((SDKSample*) samp)->getCode());
		    samples.insert(samples.begin()+pos, samp);
		    return true;
      }catch(...){
        samples.push_back(samp);
        return false;
      }
    }else{
      samples.push_back(samp);
      return true;
    }
    hasChanged = true;
  }

  unsigned int SampleSet::indexOf(Sample* smp){
    for(unsigned int i=0; i < samples.size(); i++){
      if( samples.at(i) == smp)
        return i;
    }
    return samples.size();
  }

  Sample* SampleSet::getSampleAt(unsigned int pos){
    if(pos < samples.size())
      return samples.at(pos);
    return NULL;
  }

  bool SampleSet::removeSampleAt(unsigned int i){
    hasChanged = true;
    try{
      Sample* tmp = samples.at(i);
      samples.erase(samples.begin() + i);
      delete tmp;
	  
	  if(setANN) loadAnnData(); //loads again the ANN array of points from the vector of samples

      hasChanged = true;
      return true;
    }catch(...){
      return false;
    }
  }




  vector<unsigned int>* SampleSet::findBFNeighs(Sample* samp, KthReal thresshold, unsigned int maxneighs){
	  
		unsigned int h = indexOf(samp);
		if( h != samples.size() ) clearNeighs(h);

    vector<unsigned int> neighset;
    KthReal dist=0.0;
    unsigned int indx = 0;
    vector<KthReal> distVec;
    distVec.clear();
    distVec.push_back((KthReal)100.0);
    vector<Sample*>::iterator itera = samples.begin();
    while((itera != samples.end())){
//      if((*itera) != samp ){
      if((*itera) != samp && 
		  ((*itera)->getConnectedComponent() == -1 || 
		    samp->getConnectedComponent()==-1 ||
		   (*itera)->getConnectedComponent() != samp->getConnectedComponent()) ){

		//Normally use the distance function provided by the workspace that has the proper metric of the problem
    //If not, the Sample::getDistance provides the euclidean distance in the coordinates of the sample
    if(samp->getMappedConf().size() != 0)
	{
		//TODO:
		//aqui debería pasarse los pesos...
		//
      dist = samp->getDistance((*itera), CONFIGSPACE);
//      dist = ws->distanceBetweenSamples(samp,(*itera));
	}
    else
      dist = samp->getDistance((*itera), SAMPLEDSPACE);

        if(dist < thresshold ){
          for(unsigned int i = 0; i < distVec.size(); i++)
            if(dist < distVec[i]){
              neighset.insert(neighset.begin()+i, indx);
              distVec.insert(distVec.begin()+i, dist);
              break;
            }
            while(neighset.size() > maxneighs){
              neighset.erase(neighset.end()-1);
              distVec.erase(distVec.end()-1);
            }
        }
      }
      itera++;
      indx++;
    }
    samp->setNeighs(neighset);
    hasChanged = false;


	//cout<<"BF sample="<< h <<" neighs = ";
	//for(int i=0;i<neighset.size();i++) 	cout<<neighset[i]<<"("<<distVec[i]<<"), ";
	//cout<<endl;

    return samp->getNeighs();
	
  }


   vector<unsigned int>* SampleSet::findNeighs(Sample* samp, KthReal threshold, unsigned int maxneighs)
  {
                if(typesearch==BRUTEFORCE)
                    return findBFNeighs(samp, threshold, maxneighs);
                else //ANN search
				{
					//findBFNeighs(samp, threshold, maxneighs);//for debug: comparison
                    return findAnnNeighs(samp, threshold);
				}
  } 
  
  void SampleSet::findNeighs(KthReal threshold, unsigned int maxneighs)
  {
                if(typesearch==BRUTEFORCE)
                    findBFNeighs(threshold, maxneighs);
                else // ANN search
                    findAnnNeighs(threshold);
  }

  void SampleSet::findBFNeighs(KthReal threshold, unsigned int maxneighs)
  {	  
		unsigned int max;
    if(samples.size() < maxSamples)
        max = samples.size();
    else {
			max = maxSamples;
			cout<<"findBFNeighs::Using a maximum of "<<max<<" samples"<<endl;
		}

		for(unsigned int i=0; i<max;i++)		{
      findBFNeighs(samples[i], threshold, min(maxneighs,(unsigned int)samples.size()));
		}

		hasChanged = false;
  }

  void SampleSet::updateBFNeighs(KthReal threshold, unsigned int maxneighs)
  {	  
	  	unsigned int max;
		if(samples.size() < maxSamples) max = samples.size();
		else 
		{
			max = maxSamples;
			cout<<"findBFNeighs::Using a maximum of "<<max<<" samples"<<endl;
		}

		for(unsigned int i=0; i<max;i++)
		{
			if(samples[i]->getConnectedComponent() == -1) 
			{
				findBFNeighs(samples[i], threshold, maxneighs);
			}
		}
		hasChanged = false;
  }



  void SampleSet::clearNeighs(unsigned int i){
    samples.at(i)->clearNeighs();
    hasChanged = true;
  }

  void SampleSet::clearNeighs(){
    for(unsigned int i = 0; i < samples.size(); i++)
      samples.at(i)->clearNeighs();
    hasChanged = true;
  }

  void SampleSet::clear(){
    //Sample* tmp;
    hasChanged = true;
    vector<Sample*>::iterator itera;
    for(itera = samples.begin(); itera != samples.end(); ++itera)
      delete (*itera);

//    for(unsigned int i = 0; i < samples.size(); i++){
//      tmp = samples.at(i);
//      delete tmp;
//    }
    samples.clear();
    if(setANN) setANNdatastructures();
    hasChanged = true;
  }


  long int SampleSet::findSDKOrder(unsigned long code) {
	  long int inf,sup, mitad;
    inf = 0;
    sup = samples.size()-1;
      
    if(sup<0)
		  return 0;
	  else{
		  if(sup==0){
        if(code > ((SDKSample*)samples[0])->getCode() )
				  return 1;
        else 
				  return 0;
      }else{
			  if(code > ((SDKSample*)samples[sup])->getCode() ) {
				  return sup+1;
			  }else
				  while(inf != sup-1){
            mitad = (inf+sup)/2;
            if(code > ((SDKSample*)samples[mitad])->getCode() ) 
						  inf = mitad;
            else 
						  sup = mitad;
				  }
		  }
		  if(code <= ((SDKSample*)samples[inf])->getCode() ){
			  return inf;
		  }else{
			  return sup;
		  }
    }
  }
  


  void SampleSet::loadAnnData() {
    vector<KthReal> tmpVec;
    RobConf* confSmp;
    vector<KthReal> *p;


    setANNdatastructures();//to reset the ANN array of points and the kdtree data structure

    unsigned int max;
    if(samples.size() < maxSamples)
        max = samples.size();
    else{
        max = maxSamples;
        cout<<"loadAnnData::Using a maximum of "<<max<<" samples"<<endl;
    }

    for(unsigned int h = 0; h < max; h++){
        int c=0;

        tmpVec.clear();
        for(int j=0; j < ws->getNumRobControls(); j++ ){
          tmpVec.push_back(samples[h]->getCoords()[j]);
        }

        for(unsigned int i=0; i< ws->getNumRobots(); i++){
            //compute conf of robot i corresponding to the sample samples[k]          
            ws->getRobot(i)->control2Pose(tmpVec);
            confSmp = ws->getRobot(i)->getCurrentPos();
            p = &(confSmp->getSE3().getCoordinates());

            data_pts[h][c++] = p->at(0);
            data_pts[h][c++] = p->at(1);
            data_pts[h][c++] = p->at(2);
            data_pts[h][c++] = p->at(3); //qx
            data_pts[h][c++] = p->at(4); //qy
            data_pts[h][c++] = p->at(5); //qz
            data_pts[h][c++] = p->at(6); //qw

            for(int m=0; m < confSmp->getRn().getDim(); m++){
              data_pts[h][c++] = confSmp->getRn().getCoordinates()[m];
            }

            //for(int mpk= 0; mpk < c; mpk++)
            //  std::cout << data_pts[h][mpk] << "\t" ;

            //std::cout << std::endl;
        }
        //add sample to tree
        MAG->AddPoint(data_pts[h], data_pts[h]);		// add new data point
      }
  }



  void SampleSet::findAnnNeighs(KthReal threshold)
  {	  
		if(!setANN) return;	
		
		//search structure MAG is created in cspace constructor
		//data_pts array also created in cspace constructor		
		loadAnnData();

		//if not enough data then return
		if(samples.size()<maxNeighs) return;

		//assert(_CrtCheckMemory());

		//BEGIN QUERY
		double *d_ann = new double[maxNeighs];
		int *idx_ann = new int[maxNeighs]; // This number must be lesser than samples.size() because if not the libANN crashes
		ANNpoint *result_pt = new ANNpoint[std::min(maxNeighs,(unsigned int)samples.size())];
    
		//assert(_CrtCheckMemory());

		int	numIt = 1; // maximum number of nearest neighbor calls ¿?

		unsigned int max;
		if(samples.size() < maxSamples)
			max = samples.size();
		else{
			max = maxSamples;
			cout<<"findAnnNeighs::Using a maximum of "<<max<<" samples"<<endl;
		}
		
		clearNeighs();
		for(unsigned int h=0;h<max;h++){
			//query tree
			for (unsigned int k = 0; k < maxNeighs; k++)
				d_ann[k] = INFINITY;
      
			//assert(_CrtCheckMemory());

			for (int i = 0; i < numIt; i++)	{
				MAG->NearestNeighbor(data_pts[h], d_ann, idx_ann, (void**&)result_pt);	// compute nearest neighbor using library
			}	

			//assert(_CrtCheckMemory());

			//add neighs
			//d_ann contains the distances in increasing order, starting at 0 (autodistance) which is discarded (i.e. loop startsat i=1)
			for (int i = 1; i < maxNeighs; i++) 
			{
				if(d_ann[i]<threshold)
				{
					samples[h]->addNeigh(idx_ann[i]);
					samples[h]->addNeighDistance(d_ann[i]);
				}
				else break;
			}
		}
		hasChanged = false;

    //assert(_CrtCheckMemory());

    delete[] d_ann;
    delete[] idx_ann;
    delete[] result_pt ; 
  }




  //! This method is looking for neighs of a Sample samp and finally push it to the ANN
  //! tree.
  vector<unsigned int>* SampleSet::findAnnNeighs(Sample* samp, KthReal threshold)
  {
		if(!setANN) return samp->getNeighs();


		vector<KthReal> tmpVec;	
		RobConf* confSmp;
		vector<KthReal> *p;

		//search structure MAG is created in cspace constructor
		//data_pts array also created in cspace constructor		
	  
		unsigned int c=0;


		int dim=0;
        for(unsigned int i=0; i< ws->getNumRobots(); i++)
		{
			dim += 7;
			dim += ws->getRobot(i)->getNumJoints();
		}

		ANNpoint pts = annAllocPt(dim);
        tmpVec.clear();
        for(unsigned int j=0; j < ws->getNumRobControls(); j++)
        {
            tmpVec.push_back(samp->getCoords()[j]);
        }

        for(unsigned int i=0; i< ws->getNumRobots(); i++)
		{
			//compute conf of robot i corresponding to the sample samples[k]
			ws->getRobot(i)->control2Pose(tmpVec);
			confSmp = ws->getRobot(i)->getCurrentPos();
			p = &(confSmp->getSE3().getCoordinates());
				
			pts[c++] = p->at(0);
			pts[c++] = p->at(1);
			pts[c++] = p->at(2);
			pts[c++] = p->at(3); //qx
			pts[c++] = p->at(4); //qy
			pts[c++] = p->at(5); //qz
			pts[c++] = p->at(6); //qw
			
			for(int m=0; m < confSmp->getRn().getDim(); m++)
			{
				pts[c++] = confSmp->getRn().getCoordinates()[m];
			}
		}

		//if not enough data then return
		if(samples.size()<maxNeighs)
		{
			//add sample to tree
			MAG->AddPoint(pts, pts);		// add new data point
			return samp->getNeighs();
		}

		//BEGIN QUERY
		double *d_ann = new double[maxNeighs];
		int *idx_ann = new int[maxNeighs];
		ANNpoint *result_pt = new ANNpoint[maxNeighs]; 
		int	numIt = 1; // maximum number of nearest neighbor calls ¿?
		
		//query tree
		for (int k = 0; k < maxNeighs; k++) d_ann[k] = INFINITY;
		for (int i = 0; i < numIt; i++) 
		{
			MAG->NearestNeighbor(pts, d_ann, idx_ann, (void**&)result_pt);	// compute nearest neighbor using library
		}	
		//add neighs
		
		unsigned int h = indexOf(samp);
		if( h != samples.size() ) clearNeighs(h);
		//cout<<"ANN sample="<< h <<" neighs = ";
		for (int i = 0; i < maxNeighs; i++) 
		{
			if(d_ann[i]<threshold)
			{
				//cout<<idx_ann[i]<<"("<<d_ann[i]<<"), ";

				//add idx_ann[i] as neighbor of sample h
				//the list of neighbors of h results in an increasing order as this is how ANN returns this info
				samp->addNeigh(idx_ann[i]);
				samp->addNeighDistance(d_ann[i]);
				//tries to add sample h as a neighbor of sample idx_ann[i] in an ordered way
				//i.e. it is inserted in the list of neighbors, which is always maintained with a 
				//maximum of maxNeighs
				samples[idx_ann[i]]->addNeighOrdered(h, d_ann[i], maxNeighs);
			}
			else break;
		}
		//cout<<endl;
		//add sample to tree
		MAG->AddPoint(pts, pts);		// add new data point

		hasChanged = false;


		delete[] d_ann;
		delete[] idx_ann;
		delete[] result_pt ;



		return samp->getNeighs();
  }
	
  
  bool SampleSet::writeSamples()
	{
		FILE *fp;
		fp = fopen("samples.txt","wt");
		if(fp==NULL) return false;

		for(int i=0; i<samples.size(); i++ ){
			fprintf(fp,"%d: ", indexOf(samples.at(i)));
            fprintf(fp,"numNeighs %d: ", (int)samples.at(i)->getNeighs()->size());
			for(int j=0;j<samples.at(i)->getNeighs()->size(); j++)
				fprintf(fp,"%d ", samples.at(i)->getNeighs()->at(j));
				
            fprintf(fp," distsize %d: ", (int)samples.at(i)->getNeighDistances()->size());
			for(int j=0;j<samples.at(i)->getNeighDistances()->size(); j++)
				fprintf(fp,"%.1f ", samples.at(i)->getNeighDistances()->at(j));

			fprintf(fp,"\n");
		}
		fclose(fp);
		return true;
	}

}
