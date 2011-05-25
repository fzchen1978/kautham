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
 
 

#include "application.h" 
#include <QApplication>
#include <QWidget>
#include <Inventor/Qt/SoQt.h>

// Included to use the shared memory between the Kautham and the publisher
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <libutil/data_ioc_cell.hpp>

using namespace boost::interprocess;

int main(int argc, char* argv[]){       
  //Q_INIT_RESOURCE(kauthamRes);
  try{
    //  Remove shared memory on construction and destruction
    struct shm_remove 
    {
        shm_remove() { shared_memory_object::remove("KauthamSharedMemory"); }
        ~shm_remove(){ shared_memory_object::remove("KauthamSharedMemory"); }
    } remover;
      //Create a shared memory object.
      shared_memory_object shm( create_only,               //only create
							                         "KauthamSharedMemory",     //name
							                         read_write );				      //read-write mode

      //Set size
	    shm.truncate(sizeof(kautham::data_ioc_cell));

      //Map the whole shared memory in this process
      mapped_region region( shm,					        //What to map
						                read_write);          //Map it as read-write

      //Get the address of the mapped region
      void * addr       = region.get_address();

      //Construct the shared structure in memory
      kautham::data_ioc_cell* dataCell = new (addr) kautham::data_ioc_cell;
      QWidget *app = SoQt::init(argv[0]);//argc, argv,argv[0]);
	    app->setVisible(false);
	    Application kauthApp;
      SoQt::mainLoop();
      return 0;
    }catch(interprocess_exception &ex){
      std::cout << "Kautham error: " << ex.what() << std::endl;
   }catch(...){
     std::cout << "Unexpected error in the Kautham initialization.\n";
   }
	
}

