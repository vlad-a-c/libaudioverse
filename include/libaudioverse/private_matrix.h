/**Copyright (C) Austin Hicks, 2014
This file is part of Libaudioverse, a library for 3D and environmental audio simulation, and is released under the terms of the Gnu General Public License Version 3 or (at your option) any later version.
A copy of the GPL, as well as other important copyright and licensing information, may be found in the file 'LICENSE' in the root of the Libaudioverse repository.  Should this file be missing or unavailable to you, see <http://www.gnu.org/licenses/>.*/
#pragma once
#include "libaudioverse.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef float LavTransform[4][4];
typedef float LavVector[4];

/**This library uses post multiplication for the transformation application step.  This has the following effect on your code:
Suppose you wish to translate 6 units right and rotate counterclockwise about the origin.  You build the necessary matrices.  In post multiplication, rotate first and then translate.
That is, transformations happen right to left.  This makes this library compatible with OpenGL for the most part (but that's really up to the shader authors) and mathematical docs.

In terms of memory, matrices are layed out across the rows and down; that is, using the standard notion mrc (m11 is the top left, m41 is the bottom left):
{m11, m12, m13, m14, ..., m41, m42, m43, m44}

Vectors are arrays of 4 floats, {x, y, z, w}.
When w=0, a vector represents a direction; when w=1, a vector represents a position.
Another way to put this is that w is a boolean switch which turns off and on the translation component of any transformation.  For most purposes, set w to 1.
Note that this is an oversimplification: w is important in the realm of graphical projection, but those portions of the pipeline are likely to be written on your GPU as shaders, and a full discussion of w is too verbose for this comment.
*/

Lav_PUBLIC_FUNCTION void identityTransform(LavTransform t);
Lav_PUBLIC_FUNCTION void transformApply(LavTransform t, LavVector in, LavVector out); //note: if in==out, this violates the aliasing rule and will break optimizers.
Lav_PUBLIC_FUNCTION void transformMultiply(LavTransform t1, LavTransform t2, LavTransform out);
Lav_PUBLIC_FUNCTION void transformSplitToRotationTranslation(LavTransform t, LavTransform outRot, LavTransform outTrans);
Lav_PUBLIC_FUNCTION void transformInvertOrthoganal(LavTransform t, LavTransform out);
Lav_PUBLIC_FUNCTION float vectorDotProduct(LavVector a, LavVector b);
Lav_PUBLIC_FUNCTION void vectorCrossProduct(LavVector a, LavVector b, LavVector out);
Lav_PUBLIC_FUNCTION void cameraTransform(LavVector at, LavVector up, LavVector position, LavTransform out);

#ifdef __cplusplus
}
#endif