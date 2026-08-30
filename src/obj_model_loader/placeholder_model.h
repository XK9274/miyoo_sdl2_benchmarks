#ifndef OBJ_MODEL_LOADER_PLACEHOLDER_MODEL_H
#define OBJ_MODEL_LOADER_PLACEHOLDER_MODEL_H

#include "common/model/mesh.h"

/* Small hardcoded 12-triangle, 6-material cube, no OBJ parsing -- used as a
 * fallback so the suite is always launchable. mesh is (re-)initialized
 * internally. */
void placeholder_model_build(Mesh *mesh);

#endif /* OBJ_MODEL_LOADER_PLACEHOLDER_MODEL_H */
