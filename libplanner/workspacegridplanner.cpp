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
 
 

#include <stdio.h>
#include <libproblem/workspace.h>
#include <libsampling/sampling.h>
#include "localplanner.h"
#include "workspacegridplanner.h"

using namespace libSampling;

namespace libPlanner {
  namespace workspacegridplanner{

    workspacegridPlanner::workspacegridPlanner(SPACETYPE stype, Sample *init, Sample *goal, SampleSet *samples, Sampler *sampler, WorkSpace *ws, LocalPlanner *lcPlan, KthReal ssize):
              Planner(stype, init, goal, samples, sampler, ws, lcPlan, ssize)
	{
		//set intial values
		_obstaclePotential = 10.0;
		_goalPotential = -1000.0;

		//set intial values from parent class data
		//_speedFactor = 1;
		//_solved = false;
		//setStepSize(ssize);//also changes stpssize of localplanner
	  
		
		discretizeCspace();
		
	
    }


	workspacegridPlanner::~workspacegridPlanner(){
			
	}

	
		
	void  workspacegridPlanner::readDistances(string file)
	{
		//open file
		FILE *fp;
		fp = fopen(file.c_str(),"rt");
		int nx,ny,nz;

		//read first line and set discrtization steps
		fscanf(fp,"%d %d %d\n",&nx,&ny,&nz);
		
		_stepsDiscretization[0] = nx;
		_stepsDiscretization[1] = ny;
		_stepsDiscretization[2] = nz;
		//read origin
		fscanf(fp,"%f %f %f\n",&originGrid[0],&originGrid[1],&originGrid[2]);
		//read voxelsize
		fscanf(fp,"%f %f %f\n",&voxelSize[0],&voxelSize[1],&voxelSize[2]);

		//read the other lines and load the distance values into the locations vector
		locations.reserve((int)(0.1*nx*ny*nz));

		unsigned int i=0;
		unsigned int id;
		std::pair<unsigned int, unsigned int> labelPair;
		int dist;
		//int x,y,z;
		int id2;
		//int idproof;
		//identifier of cells (id) starts at value 1, as read form file.
		maxdist = -100000000;
		int thresholdDist = -5;
		while(!feof(fp))
		{
			fscanf(fp,"%ld %d\n",&id,&dist);
			
			if(dist<thresholdDist) continue;

			location loc;//coordinates x,y,z of a location start at value 1
			loc.z = (int)floor((double)( (id-1.0)/(nx*ny) )) + 1;
			id2 = (id-1)%(nx*ny) + 1;
			loc.y = (int) floor((double)( (id2-1.0)/nx )) + 1;
			loc.x = (id2-1)%nx + 1;
			loc.d=dist;

			if(dist>maxdist) maxdist = dist;

			//idproof = nx*ny*(z-1)+nx*(y-1)+x;//s(2)*s(1)*(k-1) + s(1)*(j-1) + i;
			//if(id!=idproof)
			//{
			//	int i=0;
			//	i=i+1;
			//}

			locations.push_back(loc); //free cell - locations stores the distance value
			labelPair.first=id;
			labelPair.second=locations.size()-1;
			_cellsMap.insert(labelPair);
		}
		fclose(fp);
		
	}

   bool workspacegridPlanner::getCoordinates(unsigned int label, int *x, int *y, int *z)
   {
		if(_cellsMap.find(label)!=_cellsMap.end()) 
		{
			*x = locations[_cellsMap[label]].x;
			*y = locations[_cellsMap[label]].y;
			*z = locations[_cellsMap[label]].z;
			return true;
		}
		else return false;
   } 

   bool workspacegridPlanner::getDistance(unsigned int label, int *dist)
   {
		if(_cellsMap.find(label)!=_cellsMap.end()) 
		{
			*dist = locations[_cellsMap[label]].d;
			return true;
		}
		else return false;
   } 
   
   bool workspacegridPlanner::getNF1value(unsigned int label, KthReal *NF1value)
   {
		if(_cellsMap.find(label)!=_cellsMap.end()) 
		{
			*NF1value = potmap[_cellsMap[label]];
			return true;
		}
		else return false;
   }



	void  workspacegridPlanner::connectgrid(vector<int> &index, int coord_i)
	{
			//for coordinate coord_i, loop for all discretization steps 
			for(int j=0; j<_stepsDiscretization[coord_i]; j++) 
			{
				index[coord_i] = j;
				//if not last coordinate, continue  
				//i.e. by means of a recursive call, all the coordinates are swept
				//until the last coordinate is reached
				if(coord_i != 0) 
					connectgrid(index, coord_i - 1);	
				
				//if coord_i is the last coordinate, then the indeices of the cell are completed
				//and the edges connecting it t its neighbors are computed
				else
				{
					//find sample label from indices
					int smplabel = 0;
					int coef;
					for(int k=0;k<3;k++){ 
						if(k==0) coef=1;
						else coef = coef * _stepsDiscretization[k-1];
						smplabel += coef*index[k];
					}
					
					if(_cellsMap.find(smplabel)!=_cellsMap.end())
					{
						//sweep for all directions to find (Manhattan) neighbors
						//neighbors are looked for in the positive drection of the axis
						//i.e. incrementing the index of the current sample
						for(int n=0;n<3;n++)
						{
							//find the label of the neighbor samples from indices
							//a Manhattan neighbor (in the positive direction of an axis)
							//has all the indices equal, minus one that has 
							//the value of the index incremented by one
							int smplabelneighplus = 0;
							bool plusneighexists = true;
						
							for(int k=0;k<3;k++) 
							{
								if(k==0) coef=1;
								else coef = coef*_stepsDiscretization[k-1];
							
								if(k==n) 
								{
									if(index[k]+1 >= _stepsDiscretization[k]) plusneighexists = false;
									smplabelneighplus  += coef*(index[k]+1);
								}
								else
								{
									smplabelneighplus  += coef*index[k];
								}
							}
							//if neighor cell is a cell of the grid but it is non-free, then ignore it
							if(plusneighexists==true && _cellsMap.find(smplabelneighplus)==_cellsMap.end()) plusneighexists=false;
							
							//connect samples (if neighbor sample did exist)
							if(plusneighexists==true)
							{
								//edges are defined as pairs of ints indicating the label of the vertices
								edge_descriptor e; 
								bool inserted;//when the efge already exisits or is a self-loop
											  //then this flag is set to false and the edge is not inserted 
								tie(e, inserted) = add_edge(_cellsMap[smplabel], _cellsMap[smplabelneighplus], *g);
								
								if(inserted){
									//put a weight to the edge, depending on the collison nature of the cells
									//the value of the weight coincides wiht the maximum distance of the vertices it connects
									WeightMap weightmap = get(edge_weight, *g);
									if(locations[_cellsMap[smplabel]].d  < locations[_cellsMap[smplabelneighplus]].d)
										weightmap[e] = locations[_cellsMap[smplabelneighplus]].d;
									else 
										weightmap[e] = locations[_cellsMap[smplabel]].d;
								}
							}
						}
					}
				}
			}
		}


		
		void  workspacegridPlanner::prunegrid(cost threshold)
		{
			//WeightMap weightmap = get(edge_weight, *g);
			//		
			//graph_traits<gridGraph>::edge_iterator i, end;
			//cost c;
			//int ne=0;
			//for(tie(i,end)=boost::edges(*g); i!=end; ++i)
			//{
			//	ne++;
			//	gridVertex s=source(*i,*g);
			//	gridVertex t=target(*i,*g);
			//	c=weightmap[*i];
			//}
			
			//the filtered graph is obtained using the threshold_edge_weight function
			//that filters out edges with  weight below the given threshold passed as a parameter
			threshold_edge_weight<WeightMap> filter(get(edge_weight, *g), threshold);

			fg = new filteredGridGraph(*g, filter);

					
			//graph_traits<filteredGridGraph>::edge_iterator fi, fend;
			//ne=0;
			//for(tie(fi,fend)=boost::edges(*fg); fi!=fend; ++fi)
			//{
			//	ne++;
			//	gridVertex s=source(*fi,*fg);
			//	gridVertex t=target(*fi,*fg);
			//	c=weightmap[*fi];
			//}
			//
			//int nk=0;

		}

		//discretize and load graph
		void  workspacegridPlanner::discretizeCspace()
		{
			//create graph vertices, i.e. read distanceMap and size of grid from file
			readDistances(_wkSpace->getDistanceMapFile());

			//resize graph
			//int nx = _stepsDiscretization[0] - 1;
			//int ny = _stepsDiscretization[1] - 1;
			//int nz = _stepsDiscretization[2] - 1;
			//unsigned int maxNodes = nx*ny*nz;
			//unsigned int numEdges = nx*ny*nz*3+(nx*ny+nx*nz+ny*nz)*2+nx+ny+nz;
      
			// create graph with maxNodes nodes
			//g = new gridGraph(maxNodes);
			g = new gridGraph(locations.size());

			//create graph edges: connect neighbor grid cells
			vector<int> index(3);
			connectgrid(index, 2);
			//connectGridFromFile(_wkSpace->getNeighborhoodMapFile());

			//Create grid as graph and set the initial potential values:		
			//set initial potential values with random numbers in the range [0,1]
			//if the cell is free, or to value 10 if it is in collsion
			potmap = get(potential_value_t(), *g);
		
			LCPRNG* rgen = new LCPRNG(15485341);//15485341 is a big prime number
			for(unsigned int i=0;i<num_vertices(*g); i++)
			{
				if(locations[i].d>0)//free cell
				{
					setPotential(i, rgen->d_rand()); //random initialization of potential
				}
				else setPotential(i, _obstaclePotential);
			}
			_isGraphSet = true;

			//compute the filtered graph that do not consider edges with
			//negative weight (those connecting to a collision vertex)
			cost threshold = -5;//2.0;
			prunegrid(threshold);

		}


    void workspacegridPlanner::clearGraph()
	{
	    if(_isGraphSet){
		    locations.clear();
		    delete g;
		    delete fg;
	    }
	    _isGraphSet = false;
		_solved = false;
    }
  
	//! getGraphVertex returns true if a vertex with label "label" exisits in the graph
	//! and retunrs it in the parameter v
	bool workspacegridPlanner::getGraphVertex(unsigned int label, gridVertex *v)
	{
		if(_cellsMap.find(label)==_cellsMap.end())
		{
			//this cell is not in the graph!
			cout<<"Error. The label does not correspond to any vertex of the graph"<<endl;
			return false;
		}
		*v = _cellsMap[label];
		return true;
	}


	



	bool workspacegridPlanner::computeNF1(gridVertex vgoal)
	{

		graph_traits<filteredGridGraph>::vertex_iterator vi, vi_end;

		/* Prints adjacencies 

		graph_traits<filteredGridGraph>::adjacency_iterator avi, avi_end;
		for(tie(vi,vi_end)=vertices(*fg); vi!=vi_end; ++vi)
		{
			cout<<*vi<< " adjacent to: ";
			for(tie(avi,avi_end)=adjacent_vertices(*vi, *fg); avi!=avi_end; ++avi)
			{
				cout<<*avi<<" ,";
			}
			cout<<endl;
		}
		*/

		//initialize potential to -1 and goal to 0
		for(tie(vi,vi_end)=vertices(*fg); vi!=vi_end; ++vi)	setPotential(*vi, -1);
		setPotential(vgoal, 0);
		//propagate potential
		
		breadth_first_search(*fg, vgoal, visitor(bfs_distance_visitor<PotentialMap>(getpotmat(),locations,(KthReal)maxdist/2.0)));

		//graph_traits<filteredGridGraph>::vertex_iterator i, end;
		//for(tie(i,end)=vertices(*fg); i!=end; ++i)
		//{
		//	cout<<"vertex "<< *i<<" dist "<<getPotential(*i)<<endl;
		//}
		

		/* to improve the valleys
		KthReal maxNF1value=0.0;
		for(tie(vi,vi_end)=vertices(*fg); vi!=vi_end; ++vi)
		{
			//cout<<"vertex "<< *i<<" dist "<<getPotential(*i)<<endl;
			if(maxNF1value<getPotential(*vi))
				maxNF1value = getPotential(*vi);
		}
		
		cout<<"maxNF1 value = "<<maxNF1value<<endl;
		for(tie(vi,vi_end)=vertices(*fg); vi!=vi_end; ++vi)
		{
			//cout<<"vertex "<< *i<<" dist "<<getPotential(*i)<<endl;
			//normalize values between 0 and 1
			KthReal val=getPotential(*vi)/maxNF1value;
			//square the values to make the valeys more sharp¿?
			val=val*val*val*val;
			setPotential(*vi,val);
		}
		*/
		
		return true;
	}


	/* NO VA BE, NECESSITA MASSES ITERACIONS
bool workspacegridPlanner::computeHF(gridVertex vgoal)
	{

//initialize potential to -1 and goal to 0
		setPotential(vgoal, _goalPotential);
		//relax potential
		graph_traits<filteredGridGraph>::vertex_iterator vi, vend;
		graph_traits<filteredGridGraph>::adjacency_iterator avi, avi_end;


		int nvfg = num_vertices(*fg);
		int nvg = num_vertices(*g); 
		int nefg = num_edges(*fg);
		int neg = num_edges(*g);
		int k=-1;
		KthReal p_avi;
		for(int i=0; i<10; i++)
		{
			k=0;
			for(tie(vi,vend)=vertices(*fg); vi!=vend; ++vi)
			{
				k=*vi;
				KthReal pi = getPotential(*vi);

				if(getPotential(*vi) == _goalPotential ||
				   getPotential(*vi) == _obstaclePotential) continue;
				

				KthReal p=0;
				int count=0;
				for(tie(avi,avi_end)=adjacent_vertices(*vi, *fg); avi!=avi_end; ++avi)
				{
					count++;
					//use Neuman
					p_avi = getPotential(*avi);
					if(p_avi!=_obstaclePotential) p+=p_avi;
				}
				setPotential(*vi, p/count);
			}
		}
		return true;
	}
*/



  }
}


