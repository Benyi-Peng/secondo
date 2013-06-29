/*
 ----
 This file is part of SECONDO.

 Copyright (C) 2009, University in Hagen, Faculty of Mathematics and
 Computer Science, Database Systems for New Applications.

 SECONDO is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published
 by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 SECONDO is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with SECONDO; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 ----

 //paragraph [1] Title: [{\Large \bf \begin{center}] [\end{center}}]
 //paragraph [10] Footnote: [{\footnote{] [}}]
 //[TOC] [\tableofcontents]

 [TOC]

 0 Overview

 1 Includes and defines

*/

#ifndef TOOLBOX_H_
#define TOOLBOX_H_

#include <stdio.h>
#include <gmp.h>
#include <gmpxx.h>
#include "NestedList.h"
//#include "Point2.h"

namespace p2d {

class Point2;

bool createCoordinate(ListExpr& value, int& grid, mpq_class& precise);

void createValue(double value, int& gx, mpq_class& py);

void createPoint2(const double x, const double y, Point2** result);

bool AlmostEqual(double a, double b);

mpq_class computeMpqFromDouble(double value);

mpz_class ceil_mpq(mpq_class& value);

mpz_class floor_mpq(mpq_class& value);

} /* namespace p2d */
#endif /* TOOLBOX_H_ */
