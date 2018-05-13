//This file is part of SECONDO.

//Copyright (C) 2014, University in Hagen, Department of Computer Science,
//Database Systems for New Applications.

//SECONDO is free software; you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation; either version 2 of the License, or
//(at your option) any later version.

//SECONDO is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.

//You should have received a copy of the GNU General Public License
//along with SECONDO; if not, write to the Free Software
//Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

package mol.datatypes.base;

import static org.junit.Assert.*;

import org.junit.Test;

/**
 * Tests for class 'BaseBool'
 * 
 * @author Markus Fuessel
 */
public class BaseBoolTest {

	@Test
	public void testHashCode_DifferentObjectSameValue_HashCodeShouldBeEqual() {
		BaseBool falseBool1 = new BaseBool(false);
		BaseBool falseBool2 = new BaseBool(false);
		BaseBool trueBool1 = new BaseBool(true);
		BaseBool trueBool2 = new BaseBool(true);

		assertTrue(falseBool1.hashCode() == falseBool2.hashCode());
		assertTrue(trueBool1.hashCode() == trueBool2.hashCode());
	}

	@Test
	public void testHashCode_DifferentValue_HashCodeShouldBeNonEqual() {
		BaseBool falseBool = new BaseBool(false);
		BaseBool trueBool = new BaseBool(true);

		assertFalse(trueBool.hashCode() == falseBool.hashCode());
	}

	@Test
	public void testCompareTo_FalseVsTrue_ShouldBeLowerZero() {
		BaseBool falseBool = new BaseBool(false);
		BaseBool trueBool = new BaseBool(true);

		assertTrue(falseBool.compareTo(trueBool) < 0);

	}

	@Test
	public void testCompareTo_TrueVsFalse_ShouldBeGreaterZero() {
		BaseBool falseBool = new BaseBool(false);
		BaseBool trueBool = new BaseBool(true);

		assertTrue(trueBool.compareTo(falseBool) > 0);

	}

	@Test
	public void testCompareTo_SameBooleanValue_ShouldBeZero() {
		BaseBool falseBool1 = new BaseBool(false);
		BaseBool falseBool2 = new BaseBool(false);
		BaseBool trueBool1 = new BaseBool(true);
		BaseBool trueBool2 = new BaseBool(true);

		assertTrue(falseBool1.compareTo(falseBool2) == 0);
		assertTrue(trueBool1.compareTo(trueBool2) == 0);

	}

	@Test
	public void testEquals_EqualBooleanValue_ShouldBeTrue() {
		BaseBool falseBool1 = new BaseBool(false);
		BaseBool falseBool2 = new BaseBool(false);
		BaseBool trueBool1 = new BaseBool(true);
		BaseBool trueBool2 = new BaseBool(true);

		assertTrue(trueBool1.equals(trueBool2));
		assertTrue(falseBool1.equals(falseBool2));

	}

	@Test
	public void testEquals_NonEqualBooleanValue_ShouldBeFalse() {
		BaseBool falseBool = new BaseBool(false);
		BaseBool trueBool = new BaseBool(true);

		assertFalse(trueBool.equals(falseBool));

	}

}
