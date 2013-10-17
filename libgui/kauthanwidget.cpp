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
 
 
 
#include "kauthamwidget.h"

namespace Kautham{
/** \addtogroup libGUI
 *  @{
 */

  KauthamWidget::KauthamWidget(KauthamObject* kObj){
		_kauthObject= kObj;
    gridLayout = new QGridLayout(this);
    gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
    vboxLayout = new QVBoxLayout();
    vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));
    table = new QTableWidget(this);
    table->setObjectName(QString::fromUtf8("table"));
    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth(table->sizePolicy().hasHeightForWidth());
    table->setSizePolicy(sizePolicy);
    table->setMinimumSize(QSize(200, 200));
    table->setMaximumSize(QSize(250, 16777215));

    vboxLayout->addWidget(table);
		gridLayout->addLayout(vboxLayout, 0, 0, 1, 1);
    
    if (table->columnCount() < 2)
        table->setColumnCount(2);

    QTableWidgetItem *__colItem = new QTableWidgetItem();
    __colItem->setText(QApplication::translate("Form", "Property", 0, QApplication::UnicodeUTF8));
    table->setHorizontalHeaderItem(0, __colItem);
    __colItem = new QTableWidgetItem();
    __colItem->setText(QApplication::translate("Form", "Value", 0, QApplication::UnicodeUTF8));
    table->setHorizontalHeaderItem(1, __colItem);
    table->verticalHeader()->hide();

    if (table->rowCount() < 1)
        table->setRowCount(0);
    
    if(_kauthObject != NULL ){
      setTable(_kauthObject->getParametersAsString());
      connect(table, SIGNAL(cellChanged(int, int)), this, SLOT(tableChanged(int, int)));
    }
  }

  bool KauthamWidget::setTable(string s){
		if(s.size()!=0){
      disconnect(table, SIGNAL(cellChanged(int, int)), this, SLOT(tableChanged(int, int)));
      table->setSortingEnabled(true);
			QString content(s.c_str());
			QStringList cont = content.split("|");
			QStringList h,v;
			QStringList::const_iterator iterator;
			QTableWidgetItem *item;
			for (iterator = cont.constBegin(); iterator != cont.constEnd();
					++iterator){
				h << (*iterator).toUtf8().constData();
				++iterator;
				v << (*iterator).toUtf8().constData();
			}
			table->setRowCount(v.size());
			int i=0;
			for(iterator = v.constBegin(); iterator != v.constEnd(); ++iterator){
				item = new QTableWidgetItem((*iterator).toUtf8().constData());
				table->setItem(i,1,item);
				item= new QTableWidgetItem(h.at(i));
        table->setItem(i,0,item);
				//table->setVerticalHeaderItem(i,item);
				i++;
			}
      table->sortItems(0);
      connect(table, SIGNAL(cellChanged(int, int)), this, SLOT(tableChanged(int, int)));
      return true;
		}
		return false;
	}

  void KauthamWidget::tableChanged(int row, int col){
		QString sal;
		QTableWidgetItem *item;
    if(_kauthObject != NULL ){
		  item = table->item(row,0);
		  if(item!=NULL){
			  sal.append( item->text() + "|");
			  item = table->item(row,1);
			  sal.append(item->text());
			  string strSal(sal.toUtf8().constData());
        try{
          _kauthObject->setParametersFromString(strSal);
          setTable(_kauthObject->getParametersAsString());
        }catch(...){
          cout << "An error has been made." << endl;
        }
      }
		}
	}

  void KauthamWidget::writeGUI(string text){
    emit sendText(text);
  }

  /** @}   end of Doxygen module "libGUI" */
}
