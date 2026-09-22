// ============================================================================
// Blink Engine. Copyright (c) 2026 Dennis C. M. All Rights Reserved.
// Licensed under the Blink Engine Source Access License, see LICENSE.txt
// ============================================================================

#include "Interface/Frame_Set.slang"

/// Computes the aerial perspective LUT of the Sébastien Hillaire's model.
/// Each invocation owns one froxel column and marches it slice by slice, so consecutive slices reuse the transmittance
/// and luminance accumulated so far instead of restarting the ray march. Section 5.4.
[shader("compute")]
[numthreads(8,8,1)]
void
cs_main(uint3 thread_id : SV_DispatchThreadID)
{
}
