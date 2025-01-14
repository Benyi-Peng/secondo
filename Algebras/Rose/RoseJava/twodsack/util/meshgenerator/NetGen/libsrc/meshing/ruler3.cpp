#include <mystdlib.h>
#include "meshing.hpp"

// #define MARK
// #include <prof.h>

using namespace std;


namespace netgen
{
extern double minother;
extern double minwithoutother;


static double CalcElementBadness (const ARRAY<Point3d> & points,
				  const Element & elem)
{
  double vol, l, l4, l5, l6;
  if (elem.GetNP() != 4) 
    {
      if (elem.GetNP() == 5)
	{
	  double z = points.Get(elem.PNum(5)).Z();
	  if (z > -1e-8) return 1e8;
	  return (-1 / z) - z; //  - 2;
	}
      return 0;
    }
  
  Vec3d v1 = points.Get(elem.PNum(2)) - points.Get(elem.PNum(1));
  Vec3d v2 = points.Get(elem.PNum(3)) - points.Get(elem.PNum(1));
  Vec3d v3 = points.Get(elem.PNum(4)) - points.Get(elem.PNum(1));
  
  vol = - (Cross (v1, v2) * v3);
  l4 = Dist (points.Get(elem.PNum(2)), points.Get(elem.PNum(3)));
  l5 = Dist (points.Get(elem.PNum(2)), points.Get(elem.PNum(4)));
  l6 = Dist (points.Get(elem.PNum(3)), points.Get(elem.PNum(4)));

  l = v1.Length() + v2.Length() + v3.Length() + l4 + l5 + l6;
  
  //  testout << "vol = " << vol << " l = " << l << endl;
  if (vol < 1e-8) return 1e10;
  //  (*testout) << "l^3/vol = " << (l*l*l / vol) << endl;
  
  double err = pow (l*l*l/vol, 1.0/3.0) / 12;
  return err;
}






int Meshing3 :: ApplyRules 
(
 ARRAY<Point3d> & lpoints,     // in: local points, out: old+new local points
 ARRAY<int> & allowpoint,      // in: 1 .. it is allowed to use pointi
 ARRAY<Element2d> & lfaces,    // in: local faces, out: old+new local faces
 INDEX lfacesplit,	       // for local faces in outer radius
 INDEX_2_HASHTABLE<int> & connectedpairs,  // connected pairs for prism-meshing
 ARRAY<Element> & elements,    // out: new elements
 ARRAY<INDEX> & delfaces,      // out: face indices of faces to delete
 int tolerance,                // quality class: 1 best 
 double sloppy,                // quality strength
 int rotind1,                  // how to rotate base element
 float & retminerr             // element error 
 )

{
  int i, j, k, ri, nfok, npok, incnpok, refpi, locpi, locfi, locfr;
  int hi, minn, hpi;
  float hf, err, minerr, teterr, minteterr;
  char ok, found, hc;
  vnetrule * rule;
  Vector oldu, newu, newu1, newu2, allp;
  Vec3d ui;
  Point3d np;
  int oldnp, noldlp, noldlf;
  const Element2d * locface = NULL;
  int loktestmode;
  
  static ARRAY<int> pused;       // point is already mapped
  static ARRAY<int> fused;       // face is already mapped
  static ARRAY<int> pmap;        // map of reference point to local point
  static ARRAY<int> pfixed;      // ???
  static ARRAY<int> fmapi;       // face in reference is mapped to face nr ...
  static ARRAY<int> fmapr;       // face in reference is rotated to map 
  static ARRAY<Point3d> transfreezone;  // transformed free-zone
  static INDEX cnt = 0;
  INDEX_2_HASHTABLE<int> ledges(lfaces.Size());  // edges in local environment
  
  static ARRAY<Point3d> tempnewpoints;
  static ARRAY<Element2d> tempnewfaces;
  static ARRAY<int> tempdelfaces;
  static ARRAY<Element> tempelements;
  static ARRAY<Box3d> triboxes;         // bounding boxes of local faces


  static ARRAY<int> pnearness;
  static ARRAY<int> fnearness;
  
  cnt++;
  
  delfaces.SetSize (0);
  elements.SetSize (0);


  // determine topological distance of faces and points to
  // base element


  pnearness.SetSize (lpoints.Size());
  fnearness.SetSize (lfacesplit);

  for (i = 1; i <= pnearness.Size(); i++)
    pnearness.Set(i, INT_MAX/10);

  for (j = 1; j <= lfaces.Get(1).GetNP(); j++)
    pnearness.Set(lfaces.Get(1).PNum(j), 0);


  // MARK(appl1);
  do
    {
      ok = 1;
      
      for (i = 1; i <= lfacesplit; i++)
	{
	  const Element2d & hface = lfaces.Get(i);

	  minn = INT_MAX-1;
	  for (j = 1; j <= hface.GetNP(); j++)
	    {
	      hi = pnearness.Get(hface.PNum(j));
	      if (hi < minn) minn = hi;
	    }
	  
	  for (j = 1; j <= hface.GetNP(); j++)
	    {
	      hpi = hface.PNum(j);
	      if (pnearness.Get(hpi) > minn+1)
		{
		  ok = 0;
		  pnearness.Set(hpi, minn+1);
		}
	    }
	}

      for (i = 1; i <= connectedpairs.GetNBags(); i++)
	for (j = 1; j <= connectedpairs.GetBagSize(i); j++)
	  {
	    INDEX_2 edge;
	    int val;
	    connectedpairs.GetData (i, j, edge, val);

	    
	    if (edge.I1() > pnearness.Size() ||
		edge.I2() > pnearness.Size() ||
		edge.I1() < 1 ||
		edge.I2() < 1)
	      {
		cerr << "pair out of range" << endl;
		exit (1);
	      }
		
	    if (pnearness.Get(edge.I1()) >
		pnearness.Get(edge.I2()) + 1)
	      {
		;
		ok = 0;
		pnearness.Elem(edge.I1()) = 
		  pnearness.Get(edge.I2()) + 1;
	      }
	    if (pnearness.Get(edge.I2()) >
		pnearness.Get(edge.I1()) + 1)
	      {
		;
		ok = 0;
		pnearness.Elem(edge.I2()) = 
		  pnearness.Get(edge.I1()) + 1;
	      }
	  }
    }
  while (!ok);

  
  for (i = 1; i <= fnearness.Size(); i++)
    {
      fnearness.Set(i, 0);
      for (j = 1; j <= lfaces.Get(i).GetNP(); j++)
	fnearness.Elem(i) += pnearness.Get(lfaces.Get(i).PNum(j));
    }


  // MARK(appl2);

  // find bounding boxes of faces

  triboxes.SetSize (lfaces.Size());
  for (i = 1; i <= lfaces.Size(); i++)
    {
      const Element2d & face = lfaces.Get(i);
      triboxes.Elem(i).SetPoint (lpoints.Get(face.PNum(1)));
      for (j = 2; j <= face.GetNP(); j++)
	triboxes.Elem(i).AddPoint (lpoints.Get(face.PNum(j)));
    }

  // MARK(appl3);  

  /*
  for (j = 1; j <= lfacesplit; j++)
    for (k = 1; k <= lfaces.Get(j).GetNP(); k++)
      {
	INDEX_2 i2;
	i2.I1() = lfaces.Get(j).PNumMod(k);
	i2.I2() = lfaces.Get(j).PNumMod(k+1);
	i2.Sort();
	ledges.Set (i2, 1);
      }
  */  
  
  for (j = 1; j <= lfacesplit; j++)
    {
      const Element2d & face = lfaces.Get(j);
      int newp, oldp;

      newp = face.PNum(face.GetNP());
      for (k = 1; k <= face.GetNP(); k++)
	{
	  oldp = newp;
	  newp = face.PNum(k);
	  
	  INDEX_2 i2(oldp, newp);
	  i2.Sort();
	  ledges.Set (i2, 1);
	}
    }


  pused.SetSize (lpoints.Size());
  fused.SetSize (lfaces.Size());
  


  found = 0;
  minerr = tolfak * tolerance * tolerance;
  minteterr = sloppy * tolerance;

  if (testmode)
    (*testout) << "cnt = " << cnt << " class = " << tolerance << endl;


  // check each rule:

  // MARK(applyml);

  for (ri = 1; ri <= rules.Size(); ri++)
    { 
      sprintf (problems.Elem(ri), "");

      rule = rules.Get(ri);
      
      if (rule->GetNP(1) != lfaces.Get(1).GetNP())
	continue;

      if (rule->GetQuality() > tolerance)
	{
	  if (testmode)
	    sprintf (problems.Elem(ri), "Quality not ok");
	  continue;
	}
      
      if (testmode)
	sprintf (problems.Elem(ri), "no mapping found");
      
      loktestmode = testmode || rule->TestFlag ('t');
      /*
      if (tolerance > 8)
	loktestmode = 1;
      */

      if (loktestmode)
	(*testout) << "Rule " << ri << " = " << rule->Name() << endl;
      
      pmap.SetSize (rule->GetNP());
      fmapi.SetSize (rule->GetNF());
      fmapr.SetSize (rule->GetNF());
      
      for (i = 1; i <= lfaces.Size(); i++)
	fused.Set (i, 0);
      for (i = 1; i <= lpoints.Size(); i++)
	pused.Set (i, 0);
      for (i = 1; i <= pmap.Size(); i++)
	pmap.Set(i, 0);
      for (i = 1; i <= fmapi.Size(); i++)
	fmapi.Set(i, 0);
      for (i = 1; i <= fmapr.Size(); i++)
	fmapr.Set(i, rule->GetNP(i));
      
      fused.Set (1, 1);
      
      fmapi.Set (1, 1);
      fmapr.Set (1, rotind1);

      
      for (j = 1; j <= lfaces.Get(1).GetNP(); j++)
	{
	  locpi = lfaces.Get(1).PNumMod (j+rotind1);
	  pmap.Set (rule->GetPointNr (1, j), locpi);
	  pused.Elem(locpi)++;
	}



      /*
	map all faces
	nfok .. first nfok-1 faces are mapped properly
	*/

      nfok = 2;
      while (nfok >= 2)
	{
	  
	  if (nfok <= rule->GetNOldF())
	    {
	      // not all faces mapped

	      ok = 0;
	      locfi = fmapi.Get(nfok);
	      locfr = fmapr.Get(nfok);

	      int actfnp = rule->GetNP(nfok);

	      while (!ok)
		{
		  locfr++;
		  if (locfr == actfnp + 1)
		    {
		      locfr = 1;
		      locfi++;
		      if (locfi > lfacesplit) break;
		    }
		  
		  
		  if (fnearness.Get(locfi) > rule->GetFNearness (nfok) ||
		      fused.Get(locfi) ||
		      actfnp != lfaces.Get(locfi).GetNP() )
		    {
		      // face not feasible in any rotation

		      locfr = actfnp;
		    }
		  else
		    {
		      
		      ok = 1;
		      
		      locface = &lfaces.Get(locfi);

		      
		      // reference point already mapped differently ?
		      for (j = 1; j <= actfnp && ok; j++)
			{
			  locpi = pmap.Get(rule->GetPointNr (nfok, j));
			  
			  if (locpi && locpi != locface->PNumMod(j+locfr))
			    ok = 0;
			}
		      
		      // local point already used or point outside tolerance ?
		      for (j = 1; j <= actfnp && ok; j++)
			{
			  refpi = rule->GetPointNr (nfok, j);
			  
			  if (pmap.Get(refpi) == 0)
			    {
			      locpi = locface->PNumMod (j + locfr);

			      if (pused.Get(locpi))
				ok = 0;
			      else
				{
				  const Point3d & lp = lpoints.Get(locpi);
				  const Point3d & rp = rule->GetPoint(refpi);

				  if ( Dist2 (lp, rp) * rule->PointDistFactor(refpi) > minerr)
				    ok = 0;
				}
			    }
			}
		    }
		}
	      
	      
	      if (ok)
		{
		  // map face nfok

		  fmapi.Set (nfok, locfi);
		  fmapr.Set (nfok, locfr);
		  fused.Set (locfi, 1);
		  
		  for (j = 1; j <= rule->GetNP (nfok); j++)
		    {
		      locpi = locface->PNumMod(j+locfr);
		      
		      if (rule->GetPointNr (nfok, j) <= 3 &&
			  pmap.Get(rule->GetPointNr(nfok, j)) != locpi)
			(*testout) << "change face1 point, mark1" << endl;
		      
		      pmap.Set(rule->GetPointNr (nfok, j), locpi);
		      pused.Elem(locpi)++;
		    }
		  
		  nfok++;
		}
	      else
		{
		  // backtrack one face
		  fmapi.Set (nfok, 0);
		  fmapr.Set (nfok, rule->GetNP(nfok));
		  nfok--;
		  
		  fused.Set (fmapi.Get(nfok), 0);
		  for (j = 1; j <= rule->GetNP (nfok); j++)
		    {
		      refpi = rule->GetPointNr (nfok, j);
		      pused.Elem(pmap.Get(refpi))--;
		      
		      if (pused.Get(pmap.Get(refpi)) == 0)
			{
			  pmap.Set(refpi, 0);
			}
		    }
		}
	    }
	  
	  else
	    
	    { 

	      // all faces are mapped
	      // now map all isolated points:
	      
	      if (loktestmode)
		{
		  (*testout) << "Faces Ok" << endl;
		  sprintf (problems.Elem(ri), "Faces Ok");
		}
	      
	      npok = 1;
	      incnpok = 1;
	      
	      pfixed.SetSize (pmap.Size());
	      for (i = 1; i <= pmap.Size(); i++)
		pfixed.Set(i, (pmap.Get(i) != 0) );
	      
	      while (npok >= 1)
		{
		  
		  if (npok <= rule->GetNOldP())
		    {
		      
		      if (pfixed.Get(npok))
			
			{
			  if (incnpok)
			    npok++;
			  else
			    npok--;
			}
		      
		      else
			
			{
			  locpi = pmap.Elem(npok);
			  ok = 0;
			  
			  if (locpi)
			    pused.Elem(locpi)--;
			  
			  while (!ok && locpi < lpoints.Size())
			    {
			      ok = 1;
			      locpi++;
			      
			      if (pused.Get(locpi) || !allowpoint.Get(locpi) ||
				  pnearness.Get(locpi) > rule->GetPNearness(npok))
				{
				  ok = 0;
				}
			      else
				{
				  const Point3d & lp = lpoints.Get(locpi);
				  const Point3d & rp = rule->GetPoint(npok);

				  if ( Dist2 (lp, rp) * rule->PointDistFactor(npok) > minerr)
				    ok = 0;
				}
			    }
			  
			  
			  if (ok)
			    {
			      pmap.Set (npok, locpi);
			      
			      if (npok <= 3)
				(*testout) << "set face1 point, mark3" << endl;
			      
			      pused.Elem(locpi)++;
			      npok++;
			      incnpok = 1;
			    }
			  
			  else
			    
			    {
			      pmap.Set (npok, 0);
			      
			      if (npok <= 3)
				(*testout) << "set face1 point, mark4" << endl;
			      
			      npok--;
			      incnpok = 0;
			    }
			}
		    }
		  
		  else
		    
		    {
		      
		      // all points are mapped
		      
		      if (loktestmode)
			{
			  (*testout) << "Mapping found!!: Rule " << rule->Name() << endl;
			  for (i = 1; i <= pmap.Size(); i++)
			    (*testout) << pmap.Get(i) << " ";
			  (*testout) << endl;
			  sprintf (problems.Elem(ri), "mapping found");
			  (*testout) << rule->GetNP(1) << " = " << lfaces.Get(1).GetNP() << endl;
			}
		      
		      ok = 1;
		      
		      
		      // check mapedges:
		      
		      for (i = 1; i <= rule->GetNEd(); i++)
			{
			  int i1, i2;
			  i1 = pmap.Get(rule->GetEdge(i).i1);
			  i2 = pmap.Get(rule->GetEdge(i).i2);

			  INDEX_2 in2(i1, i2);
			  in2.Sort();
			  if (!ledges.Used (in2)) ok = 0;
			}


		      // check prism edges:
		      for (i = 1; i <= rule->GetNE(); i++)
			{
			  const Element & el = rule->GetElement (i);
			  if (el.GetType() == PRISM) 
			    { 
			      for (j = 1; j <= 3; j++)
				{
				  int i1, i2;
				  i1 = pmap.Get(el.PNum(j));
				  i2 = pmap.Get(el.PNum(j+3));
				  
				  INDEX_2 in2(i1, i2);
				  in2.Sort();
				  if (!connectedpairs.Used (in2)) ok = 0;
				}
			    }
			  if (el.GetType() == PYRAMID) 
			    { 
			      if (loktestmode)
				(*testout) << "map pyramid, rule = " << rule->Name() << endl;
			      for (j = 1; j <= 2; j++)
				{
				  INDEX_2 in2;
				  if (j == 1)
				    {
				      in2.I1() = pmap.Get(el.PNum(2));
				      in2.I2() = pmap.Get(el.PNum(3));
				    }
				  else
				    {
				      in2.I1() = pmap.Get(el.PNum(1));
				      in2.I2() = pmap.Get(el.PNum(4));
				    }
				  in2.Sort();
				  if (!connectedpairs.Used (in2)) 
				    {
				      ok = 0;
				      if (loktestmode)
					(*testout) << "no pair" << endl;
				    }
				}
			    }

			}
		      

		      
		      for (i = rule->GetNOldF() + 1; i <= rule->GetNF(); i++)
			fmapi.Set(i, 0);
		      

		      if (ok)
			{
			  foundmap.Elem(ri)++;
			}

		      


		      // deviation of existing points

		      oldu.SetSize (3 * rule->GetNOldP());
		      newu.SetSize (3 * (rule->GetNP() - rule->GetNOldP()));
		      allp.SetSize (3 * rule->GetNP());
		      
		      for (i = 1; i <= rule->GetNOldP(); i++)
			{
			  const Point3d & lp = lpoints.Get(pmap.Get(i));
			  const Point3d & rp = rule->GetPoint(i);
			  oldu.Set (3*i-2, lp.X()-rp.X());
			  oldu.Set (3*i-1, lp.Y()-rp.Y());
			  oldu.Set (3*i  , lp.Z()-rp.Z());
			  
			  allp.Set (3*i-2, lp.X());
			  allp.Set (3*i-1, lp.Y());
			  allp.Set (3*i  , lp.Z());
			}

		      if (rule->GetNP() > rule->GetNOldP())
			{
			  newu.SetSize (rule->GetOldUToNewU().Height());
			  rule->GetOldUToNewU().Mult (oldu, newu);
			}

		      //		      int idiff = 3 * (rule->GetNP()-rule->GetNOldP());
		      int idiff = 3 * rule->GetNOldP();
		      for (i = rule->GetNOldP()+1; i <= rule->GetNP(); i++)
			{
			  const Point3d & rp = rule->GetPoint(i);
			  allp.Set (3*i-2, rp.X() + newu.Get(3*i-2 - idiff));
			  allp.Set (3*i-1, rp.Y() + newu.Get(3*i-1 - idiff));
			  allp.Set (3*i  , rp.Z() + newu.Get(3*i   - idiff));
			}
		      
		      rule->SetFreeZoneTransformation (allp, 
						       tolerance + int(sloppy));

		      if (!rule->ConvexFreeZone())
			{
			  ok = 0;
			  sprintf (problems.Elem(ri), "Freezone not convex");

			  if (loktestmode)
			    (*testout) << "Freezone not convex" << endl;
			}

		      if (loktestmode)
			{
			  const ARRAY<Point3d> & fz = rule->GetTransFreeZone();
			  (*testout) << "Freezone: " << endl;
			  for (i = 1; i <= fz.Size(); i++)
			    (*testout) << fz.Get(i) << endl;
			}
		      

		      // check freezone:
		      
		      for (i = 1; i <= lpoints.Size(); i++)
			{
			  if ( !pused.Get(i) )
			    {
			      const Point3d & lp = lpoints.Get(i);

			      if (rule->fzbox.IsIn (lp))
				{
				  if (rule->IsInFreeZone(lp))
				    {
				      if (loktestmode)
					{
					  (*testout) << "Point " << i 
						     << " in Freezone" << endl;
					  sprintf (problems.Elem(ri), 
						   "locpoint %d in Freezone", i);
					}
				      ok = 0;
				      break;
				    }
				}
			    }
			}

		      for (i = 1; i <= lfaces.Size() && ok; i++)
			{
			  static ARRAY<int> lpi(4);

			  if (!fused.Get(i))
			    { 
			      int triin;
			      const Element2d & lfacei = lfaces.Get(i);

			      if (!triboxes.Elem(i).Intersect (rule->fzbox))
				triin = 0;
			      else
				{
				  int li, lj;
				  for (li = 1; li <= lfacei.GetNP(); li++)
				    {
				      int lpii = 0;
				      int pi = lfacei.PNum(li);
				      for (lj = 1; lj <= rule->GetNOldP(); lj++)
					if (pmap.Get(lj) == pi)
					  lpii = lj;
				      lpi.Elem(li) = lpii;
				    }


				  if (lfacei.GetNP() == 3)
				    {
				      triin = rule->IsTriangleInFreeZone 
					(
					 lpoints.Get(lfacei.PNum(1)),
					 lpoints.Get(lfacei.PNum(2)),
					 lpoints.Get(lfacei.PNum(3)), lpi, 1
					 );
				    }
				  else
				    {
				      triin = rule->IsQuadInFreeZone 
					(
					 lpoints.Get(lfacei.PNum(1)),
					 lpoints.Get(lfacei.PNum(2)),
					 lpoints.Get(lfacei.PNum(3)), 
					 lpoints.Get(lfacei.PNum(4)), 
					 lpi, 1
					 );
				    }
				}


			      if (triin == -1)
				{
				  ok = 0;
				}
			      
			      if (triin == 1)
				{
#ifdef TEST_JS
				  ok = 0;

				  if (loktestmode)
				    {
				      (*testout) << "El with " << lfaces.Get(i).GetNP() << " points in freezone: "
						 << lfaces.Get(i).PNum(1) << " - " 
						 << lfaces.Get(i).PNum(2) << " - "
						 << lfaces.Get(i).PNum(3) << " - "
						 << lfaces.Get(i).PNum(4) << endl;
				      for (int lj = 1; lj <= lfaces.Get(i).GetNP(); lj++)
					(*testout) << lpoints.Get(lfaces.Get(i).PNum(lj)) << " ";

				      (*testout) << endl;

				      sprintf (problems.Elem(ri), "triangle (%d, %d, %d) in Freezone",
					       lfaces.Get(i).PNum(1), lfaces.Get(i).PNum(2),
					       lfaces.Get(i).PNum(3));
				    }
#else
				  if (loktestmode)
				    {
				      if (lfacei.GetNP() == 3)
					{
					  (*testout) << "Triangle in freezone: "
						     << lfacei.PNum(1) << " - " 
						     << lfacei.PNum(2) << " - "
						     << lfacei.PNum(3) 
						     << ", or "
						     << lpoints.Get(lfacei.PNum(1)) << " - " 
						     << lpoints.Get(lfacei.PNum(2)) << " - "
						     << lpoints.Get(lfacei.PNum(3)) 
						     << endl;
					  (*testout) << "lpi = " << lpi.Get(1) << ", " 
						     << lpi.Get(2) << ", " << lpi.Get(3) << endl;
					}
				      else
					  (*testout) << "Quad in freezone: "
						     << lfacei.PNum(1) << " - " 
						     << lfacei.PNum(2) << " - "
						     << lfacei.PNum(3) << " - "
						     << lfacei.PNum(4) 
						     << ", or "
						     << lpoints.Get(lfacei.PNum(1)) << " - " 
						     << lpoints.Get(lfacei.PNum(2)) << " - "
						     << lpoints.Get(lfacei.PNum(3)) << " - "
						     << lpoints.Get(lfacei.PNum(4)) 
						     << endl;

				      sprintf (problems.Elem(ri), "triangle (%d, %d, %d) in Freezone",
					       int(lfaces.Get(i).PNum(1)), 
					       int(lfaces.Get(i).PNum(2)),
					       int(lfaces.Get(i).PNum(3)));
				    }	

				  hc = 0;
				  for (k = rule->GetNOldF() + 1; k <= rule->GetNF(); k++)
				    {
				      if (rule->GetPointNr(k, 1) <= rule->GetNOldP() &&
					  rule->GetPointNr(k, 2) <= rule->GetNOldP() &&
					  rule->GetPointNr(k, 3) <= rule->GetNOldP())
					{
					  for (j = 1; j <= 3; j++)
					    if (lfaces.Get(i).PNumMod(j  ) == pmap.Get(rule->GetPointNr(k, 1)) &&
						lfaces.Get(i).PNumMod(j+1) == pmap.Get(rule->GetPointNr(k, 3)) &&
						lfaces.Get(i).PNumMod(j+2) == pmap.Get(rule->GetPointNr(k, 2)))
					      {
						fmapi.Elem(k) = i;
						hc = 1;

						
 // 						(*testout) << "found from other side: " 
//  							   << rule->Name() 
//  							   << " ( " << pmap.Get (rule->GetPointNr(k, 1))
//  							   << " - " << pmap.Get (rule->GetPointNr(k, 2))
//  							   << " - " << pmap.Get (rule->GetPointNr(k, 3)) << " ) "
//  							   << endl;

						strcpy (problems.Elem(ri), "other");
					      }
					}
				    }
				  
				  if (!hc)
				    {
				      if (loktestmode)
					{
					  (*testout) << "Triangle in freezone: "
						     << lfaces.Get(i).PNum(1) << " - " 
						     << lfaces.Get(i).PNum(2) << " - "
						     << lfaces.Get(i).PNum(3) << endl;

					  sprintf (problems.Elem(ri), "triangle (%d, %d, %d) in Freezone",
						   int (lfaces.Get(i).PNum(1)), 
						   int (lfaces.Get(i).PNum(2)),
						   int (lfaces.Get(i).PNum(3)));
					}
				      ok = 0;
				    }
#endif
				}
			    }
			   
			}

		      
		      if (ok)
			{
			  err = 0;
			  for (i = 1; i <= rule->GetNOldP(); i++)
			    {
			      hf = rule->CalcPointDist (i, lpoints.Get(pmap.Get(i)));
			      if (hf > err) err = hf;
			    }
			  
			  
			  if (loktestmode)
			    {
			      (*testout) << "Rule ok" << endl;
			      sprintf (problems.Elem(ri), "Rule ok, err = %f", err);
			    }


			  // MARK(m2);			  
			  //			  newu = rule->GetOldUToNewU() * oldu;

			  // set new points:
			  
			  oldnp = rule->GetNOldP();
			  noldlp = lpoints.Size();
			  noldlf = lfaces.Size();
			  
			  
			  for (i = oldnp + 1; i <= rule->GetNP(); i++)
			    {
			      np = rule->GetPoint(i);
			      np.X() += newu.Elem (3 * (i-oldnp) - 2);
			      np.Y() += newu.Elem (3 * (i-oldnp) - 1);
			      np.Z() += newu.Elem (3 * (i-oldnp));
			      
			      pmap.Elem(i) = lpoints.Append (np);
			    }
			  
			  // Set new Faces:
			  
			  for (i = rule->GetNOldF() + 1; i <= rule->GetNF(); i++)
			    if (!fmapi.Get(i))
			      {
				Element2d nface(rule->GetNP(i));
				for (j = 1; j <= nface.GetNP(); j++)
				  nface.PNum(j) = pmap.Get(rule->GetPointNr (i, j));
				
				lfaces.Append (nface);
			      }
			  
			  
			  // Delete old Faces:

			  for (i = 1; i <= rule->GetNDelF(); i++)
			    delfaces.Append (fmapi.Get(rule->GetDelFace(i)));
			  for (i = rule->GetNOldF()+1; i <= rule->GetNF(); i++)
			    if (fmapi.Get(i))
			      {
				delfaces.Append (fmapi.Get(i));
				fmapi.Elem(i) = 0;
			      }
			  

			  // check orientation
			  for (i = 1; i <= rule->GetNO() && ok; i++)
			    {
			      const fourint * fouri;
			      
			      fouri = &rule->GetOrientation(i);
			      Vec3d v1 (lpoints.Get(pmap.Get(fouri->i1)), 
					lpoints.Get(pmap.Get(fouri->i2)));
			      Vec3d v2 (lpoints.Get(pmap.Get(fouri->i1)), 
					lpoints.Get(pmap.Get(fouri->i3)));
			      Vec3d v3 (lpoints.Get(pmap.Get(fouri->i1)), 
					lpoints.Get(pmap.Get(fouri->i4)));

			      Vec3d n;
			      Cross (v1, v2, n);
			      if (n * v3 > -1e-7)
				{
				  if (loktestmode)
				    {
				      sprintf (problems.Elem(ri), "Orientation wrong");
				      (*testout) << "Orientation wrong" << endl;
				    }
				  ok = 0;
				}
			    }

			  

			  // new points in free-zone ?
			  for (i = rule->GetNOldP() + 1; i <= rule->GetNP() && ok; i++)
			    if (!rule->IsInFreeZone (lpoints.Get(pmap.Get(i))))
			      {
				if (loktestmode)
				  {
				    (*testout) << "Newpoint " << lpoints.Get(pmap.Get(i))
					       << " outside convex hull" << endl;
				    sprintf (problems.Elem(ri), "newpoint outside convex hull");
				  }
				ok = 0;
				
			      }
			  
			  // insert new elements
			  
			  for (i = 1; i <= rule->GetNE(); i++)
			    {
			      elements.Append (rule->GetElement(i));
			      for (j = 1; j <= elements.Get(i).NP(); j++)
				elements.Elem(i).PNum(j) = pmap.Get(elements.Get(i).PNum(j));
			    }
			  

			  // Calculate Element badness
			  
			  teterr = 0;
			  for (i = 1; i <= elements.Size(); i++)
			    {
			      hf = CalcElementBadness (lpoints, elements.Get(i));
			      if (hf > teterr) teterr = hf;
			    }

			  /*
			    // keine gute Erfahrung am 25.1.2000, js
			  if (ok && teterr < 100 &&
			      (rule->TestFlag('b') || tolerance > 10) )
			    {
			      (*mycout) << "Reset teterr " 
				   << rule->Name() 
				   << " err = " << teterr 
				   << endl;
			      teterr = 1;
			    }
			  */

			  // compare edgelength
			  if (rule->TestFlag('l'))
			    {
			      double oldlen = 0;
			      double newlen = 0;

			      for (i = 1; i <= rule->GetNDelF(); i++)
				{
				  const Element2d & face = 
				    rule->GetFace (rule->GetDelFace(i));
				  for (j = 1; j <= 3; j++)
				    {
				      const Point3d & p1 =
					lpoints.Get(pmap.Get(face.PNumMod(j)));
				      const Point3d & p2 =
					lpoints.Get(pmap.Get(face.PNumMod(j+1)));
				      oldlen += Dist(p1, p2);
				    }
				}

			      for (i = rule->GetNOldF()+1; i <= rule->GetNF(); i++)
				{
				  const Element2d & face = rule->GetFace (i);
				  for (j = 1; j <= 3; j++)
				    {
				      const Point3d & p1 =
					lpoints.Get(pmap.Get(face.PNumMod(j)));
				      const Point3d & p2 =
					lpoints.Get(pmap.Get(face.PNumMod(j+1)));
				      newlen += Dist(p1, p2);
				    }
				}

			      if (oldlen < newlen) 
				{
				  ok = 0;
				  if (loktestmode)
				    sprintf (problems.Elem(ri), "oldlen < newlen");
				}
			    }
			  

			  if (loktestmode)
			    (*testout) << "ok = " << int(ok) 
				       << "teterr = " << teterr 
				       << "minteterr = " << minteterr << endl;


			  if (ok && teterr < tolerance)
			    {
			      canuse.Elem(ri) ++;
			      /*
			      (*testout) << "can use rule " << rule->Name() 
					 << ", err = " << teterr << endl;
			      for (i = 1; i <= pmap.Size(); i++)
				(*testout) << pmap.Get(i) << " ";
			      (*testout) << endl;
			      */

			      if (strcmp (problems.Elem(ri), "other") == 0)
				{
				  if (teterr < minother)
				    minother = teterr;
				}
			      else
				{
				  if (teterr < minwithoutother)
				    minwithoutother = teterr;
				}
			    }

			  if (ok && teterr < minteterr)
			    {

			      if (loktestmode)
				(*testout) << "use rule" << endl;

			      found = ri;
			      minteterr = teterr;
			      
			      if (testmode)
				{
				  for (i = 1; i <= rule->GetNOldP(); i++)
				    {
				      (*testout) << "P" << i << ": Ref: "
						 << rule->GetPoint (i) << "  is: "
						 << lpoints.Get(pmap.Get(i)) << endl;
				    }
				}
			      
			      tempnewpoints.SetSize (0);
			      for (i = noldlp+1; i <= lpoints.Size(); i++)
				tempnewpoints.Append (lpoints.Get(i));
			      
			      tempnewfaces.SetSize (0);
			      for (i = noldlf+1; i <= lfaces.Size(); i++)
				tempnewfaces.Append (lfaces.Get(i));

			      tempdelfaces.SetSize (0);
			      for (i = 1; i <= delfaces.Size(); i++)
				tempdelfaces.Append (delfaces.Get(i));
			      
			      tempelements.SetSize (0);
			      for (i = 1; i <= elements.Size(); i++)
				tempelements.Append (elements.Get(i));
			    }
			  
			  lpoints.SetSize (noldlp);
			  lfaces.SetSize (noldlf);
			  delfaces.SetSize (0);
			  elements.SetSize (0);
			}
		      
		      npok = rule->GetNOldP();
		      incnpok = 0;
		    }
		}
	      
	      nfok = rule->GetNOldF();
	      
	      for (j = 1; j <= rule->GetNP (nfok); j++)
		{
		  refpi = rule->GetPointNr (nfok, j);
		  pused.Elem(pmap.Get(refpi))--;
		  
		  if (pused.Get(pmap.Get(refpi)) == 0)
		    {
		      pmap.Set(refpi, 0);
		    }
		}
	      
	    }
	}
      if (loktestmode)
	(*testout) << "end rule" << endl;
    }

  //  (*testout) << "end" << endl;

  // if successfull, reload best choice
  
  if (found)
    {

#ifdef debug
      // if face in advancing front ???
      for (i = 1; i <= tempnewfaces.Size(); i++)
	{
	  hc = 1;
	  for (k = 1; k <= lfaces.Size() && hc; k++)
	    for (j = 1; j <= 3 && hc; j++)
	      if (tempnewfaces.Elem(i).PNumMod(j  ) == lfaces.Get(k).PNum(1) &&
		  tempnewfaces.Elem(i).PNumMod(j+1) == lfaces.Get(k).PNum(3) &&
		  tempnewfaces.Elem(i).PNumMod(j+2) == lfaces.Get(k).PNum(2))
		{
		  tempdelfaces.Append(k);
		  tempnewfaces.Elem(i).PNum(1) = 0;
		  hc = 0;
		  cerr << "Ruler-reload necessary" << endl;
		}
	}
#endif
      
      for (i = 1; i <= tempnewpoints.Size(); i++)
	lpoints.Append (tempnewpoints.Get(i));
      for (i = 1; i <= tempnewfaces.Size(); i++)
	if (tempnewfaces.Get(i).PNum(1))
	  lfaces.Append (tempnewfaces.Get(i));
      for (i = 1; i <= tempdelfaces.Size(); i++)
	delfaces.Append (tempdelfaces.Get(i));
      for (i = 1; i <= tempelements.Size(); i++)
	elements.Append (tempelements.Get(i));
    }
  
  retminerr = minerr;
  return found;
}
}
